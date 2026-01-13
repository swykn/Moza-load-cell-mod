#include <Wire.h>
#include "SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h"
#include <Joystick.h>

NAU7802 scale;

// --- HID: Y軸 + ボタン1個（joydevに出やすくするため） ---
// --- HID: Y axis + 1 button (to show up easily as a joydev device) ---
Joystick_ joy(
  JOYSTICK_DEFAULT_REPORT_ID,
  JOYSTICK_TYPE_JOYSTICK,
  1, 0,                 // buttons=1, hat=0
  false, true, false,   // X=false, Y=true, Z=false
  false, false, false,  // Rx,Ry,Rz
  false, false,         // rudder, throttle
  false, false, false   // accelerator, brake, steering
);

// ===== 調整パラメータ =====
// ===== Tuning parameters =====
#define DEBUG_SERIAL 1       // 0:Serial出さない  1:20Hzで出す
                           // 0: disable Serial output, 1: print at 20Hz

// I2CとNAU7802設定
// I2C and NAU7802 settings
const uint8_t  GAIN   = NAU7802_GAIN_128;
const uint8_t  SPS    = NAU7802_SPS_320;

// 0..1023 出力（Sim側でキャリブする前提）
// Output 0..1023 (assumes calibration on the simulator side)
const uint16_t AXIS_MAX = 1023;

// デッドゾーン（0..1023）
// Dead zone (0..1023)
const uint16_t DEADZONE = 8;

// カーブ: 0=リニア, 1=二次（踏み始め穏やか）
// Curve: 0=linear, 1=quadratic (gentler at initial press)
const uint8_t CURVE_MODE = 0;

// ビットシフトIIR（最軽量）
// Bit-shift IIR (lightest-weight)
// kが小さいほど速い。K_UPは踏み込み側、K_DNは戻り側。
// Smaller k = faster response. K_UP for press (rising), K_DN for release (falling).
const uint8_t K_UP = 0;  // 0=完全追従（最速） 1=速い 2=普通
                         // 0=full follow (fastest), 1=fast, 2=normal
const uint8_t K_DN = 3;  // 戻りを安定させる（大きいほど遅い）
                         // Stabilize release (larger = slower)

// 値の向き：踏むと増えるようにする（必要なら1）
// Direction: make value increase when pressed (set to 1 if needed)
const uint8_t INVERT_RAW = 1;

// キャリブ
// Calibration
int32_t raw0   = 0;       // 無荷重ゼロ点（起動時平均で決定）
// raw0: zero point at no load (determined by averaging at startup)
int32_t rawMax = 950000;  // 最大踏力（filtの最大実測に合わせて調整推奨）
// rawMax: max pedal force (recommended to tune to observed max of filt)

// ==== rawMax 自動キャリブ ====
// ==== Auto-calibration for rawMax ====
#define AUTO_CAL_MAX 1

// キャリブ開始判定（軸値）
// Auto-cal start threshold (axis value)
const uint16_t CAL_START_AXIS = 200;   // 0..1023のうち、この値を超えたら「踏んだ」とみなす
                                       // Consider "pressed" when exceeding this value (0..1023)
const uint16_t CAL_MARGIN_PCT = 2;     // 確定rawMaxに上乗せするマージン（%）
                                       // Margin to add on top of finalized rawMax (%)
const uint16_t CAL_WINDOW_MS  = 2000;  // 最大値を集める時間（ms）
                                       // Time window to capture the peak (ms)

static bool calDone = false;
static bool calRunning = false;
static uint32_t calT0 = 0;
static int32_t calMaxFilt = 0;

// 安全ガード（極端な値を防ぐ）
// Safety guards (prevent extreme values)
const int32_t RAWMAX_MIN = 50000;      // 最低レンジ（環境により調整）
                                       // Minimum range (adjust per environment)
const int32_t RAWMAX_MAX = 2000000;    // 上限（あり得ない値で暴走しないように）
                                       // Upper limit (avoid runaway from impossible values)


// ---- 補助関数 ----
// ---- Helper functions ----
static int32_t measureZero(NAU7802 &s, int samples)
{
  int64_t acc = 0;
  int n = 0;
  while (n < samples) {
    if (s.available()) {
      acc += s.getReading();
      n++;
    }
  }
  return (int32_t)(acc / samples);
}

static inline uint16_t clampU16(uint32_t v, uint16_t lo, uint16_t hi)
{
  if (v < lo) return lo;
  if (v > hi) return hi;
  return (uint16_t)v;
}

// 生値（int32）→ 0..1023（線形）
// Raw value (int32) -> 0..1023 (linear)
static inline uint16_t rawToAxis(int32_t v, int32_t z, int32_t m)
{
  // v: raw or filt
  int32_t dv = v - z;
  if (dv <= 0) return 0;

  int32_t span = m - z;
  if (span <= 1) return 0;

  if (dv >= span) return AXIS_MAX;

  // 32bitで安全に：dv * 1023 / span
  // Safe in 32-bit: dv * 1023 / span
  uint32_t out = (uint32_t)dv * (uint32_t)AXIS_MAX;
  out /= (uint32_t)span;
  return (uint16_t)out;
}

// デッドゾーン＋（任意）カーブ
// Dead zone + (optional) curve shaping
static inline uint16_t shapeAxis(uint16_t x)
{
  if (x <= DEADZONE) return 0;

  // デッドゾーン分を除いて再スケール
  // Rescale after removing the dead-zone portion
  uint32_t y = (uint32_t)(x - DEADZONE) * AXIS_MAX;
  y /= (AXIS_MAX - DEADZONE);
  uint16_t r = (uint16_t)y;

  if (CURVE_MODE == 0) {
    return r; // linear
  } else {
    // quadratic: r^2 / 1023
    return (uint16_t)(((uint32_t)r * (uint32_t)r) / AXIS_MAX);
  }
}

void setup()
{
#if DEBUG_SERIAL
  Serial.begin(115200);
  Serial.println("DEBUG MODE!!");
#endif

  Wire.begin();

  while (!scale.begin()) {
#if DEBUG_SERIAL
    Serial.println("NAU7802 not detected");
#endif
    delay(500);
  }

  scale.setGain(GAIN);
  scale.setSampleRate(SPS);
  scale.calibrateAFE();

  // 無荷重で起動してゼロ点取得
  // Start with no load to measure and set the zero point
  raw0 = measureZero(scale, 64);

  // HID初期化
  // Initialize HID
  joy.setYAxisRange(0, AXIS_MAX);
  joy.begin();

#if DEBUG_SERIAL
  Serial.print("raw0=");   Serial.print(raw0);
  Serial.print(" rawMax=");Serial.println(rawMax);
  Serial.println("AutoCal: press brake firmly for about 2 seconds to calibrate max.");
#endif
}

void loop()
{
  if (!scale.available()) return;

  int32_t raw = scale.getReading();
  if (INVERT_RAW) raw = -raw;

  // --- ビットシフトIIR（最軽量） ---
  // --- Bit-shift IIR (lightest-weight) ---
  static int32_t filt = 0;
  int32_t diff = raw - filt;
  if (diff > 0) {
    // 踏み込み（上り）を速く
    // Faster on press (rising edge)
    if (K_UP == 0) filt = raw;
    else filt += (diff >> K_UP);
  } else {
    // 戻り（下り）を遅く
    // Slower on release (falling edge)
    filt += (diff >> K_DN);
  }

  // 0..1023へ
  // Convert to 0..1023
  uint16_t axis = rawToAxis(filt, raw0, rawMax);
  axis = shapeAxis(axis);

#if AUTO_CAL_MAX
  // rawMax確定前だけ実行
  // Run only until rawMax is finalized
  if (!calDone) {
    // キャリブ開始条件：ある程度踏み込んだら開始
    // Start condition: begin once pressed beyond a threshold
    if (!calRunning && axis > CAL_START_AXIS) {
      calRunning = true;
      calT0 = millis();
      calMaxFilt = filt;
#if DEBUG_SERIAL
      Serial.println("AutoCal: started");
#endif
    }

    // キャリブ実行中：最大filtを追跡
    // During auto-cal: track maximum filt
    if (calRunning) {
      if (filt > calMaxFilt) calMaxFilt = filt;

      // 時間経過で確定
      // Finalize after the time window elapses
      if (millis() - calT0 >= CAL_WINDOW_MS) {
        calRunning = false;
        calDone = true;

        // マージン付け（%）
        // Add margin (%)
        int32_t m = calMaxFilt + (calMaxFilt * (int32_t)CAL_MARGIN_PCT) / 100;

        // ガード
        // Guard rails
        if (m < RAWMAX_MIN) m = RAWMAX_MIN;
        if (m > RAWMAX_MAX) m = RAWMAX_MAX;

        rawMax = m;

#if DEBUG_SERIAL
        Serial.print("AutoCal: done rawMax=");
        Serial.println(rawMax);
#endif
      }
    }
  }
#endif

  // HID更新（毎回送る：体感優先）
  // Update HID (send every time for responsiveness)
  joy.setYAxis(axis);

  // ボタン：踏み込みがある程度以上でON（任意）
  // Button: ON when pressed beyond a threshold (optional)
  joy.setButton(0, axis > 20);

#if DEBUG_SERIAL
  // Serialは20Hzのみ。32bit生値は重いので>>8して16bit相当にして出す
  // Serial output at 20Hz only. 32-bit raw is heavy, so shift >>8 to print ~16-bit values.
  static uint32_t lastPrint = 0;
  uint32_t now = millis();
  if (now - lastPrint >= 50) { // 20Hz
    lastPrint += 50;
    int16_t r16 = (int16_t)(raw >> 8);
    int16_t f16 = (int16_t)(filt >> 8);

    Serial.print("r16="); Serial.print(r16);
    Serial.print(" f16="); Serial.print(f16);
    Serial.print(" out="); Serial.println(axis);
  }
#endif
}
