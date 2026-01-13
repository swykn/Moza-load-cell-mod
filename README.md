# Moza Load Cell Mod

## English

This project converts the **brake pedal of the Moza CR-P Lite Pedals into a load-cell based system**.  
The goal is to enable force-based braking input and achieve a more realistic driving feel.

---

### Hardware Overview

An **Arduino Pro Micro compatible board** is used as a  
**USB HID joystick controller**.

The load cell used is the **Akizuki Denshi 100 kg load cell (SC301A)**.  
https://akizukidenshi.com/catalog/g/g112036/

The load cell amplifier is **SparkFun Qwiic Scale – NAU7802**.  
It was selected to enable **high-speed sampling at 320 SPS**, which is not possible with the HX711.  
https://www.sparkfun.com/sparkfun-qwiic-scale-nau7802.html

The system uses:
- **5 V logic (Arduino Pro Micro)**
- **3.3 V logic (NAU7802)**

Therefore, **power conversion (5V → 3.3V)** and **I²C level shifting** are required.

---

### Power Conversion (5V → 3.3V)

A dedicated **5V → 3.3V power module** is used.  
The generated **3.3 V is supplied to both the NAU7802 and the LV pin of the level shifter**.

Amazon (Japan):  
https://www.amazon.co.jp/dp/B0BJJBG8C4

**Important**
- NAU7802 is **not 5V tolerant**
- Never connect NAU7802 directly to 5 V
- All GND connections must be common

---

### Level Shifter Used

A **BSS138-based I²C bidirectional level shifter** is used.

Amazon (Japan):  
https://www.amazon.co.jp/dp/B086WWCJGQ

Any equivalent BSS138-based bidirectional I²C level shifter can be used.

---

### Wiring (I²C Level Shifting)

Only **SDA, SCL, VCC, and GND** are required.

- HV side: **5 V (Arduino Pro Micro)**
- LV side: **3.3 V (from power module)**

| Arduino Pro Micro (5V) | Level Shifter (HV) | Level Shifter (LV) | NAU7802 (3.3V) |
|------------------------|-------------------|-------------------|---------------|
| SDA                    | HV1               | LV1               | SDA           |
| SCL                    | HV2               | LV2               | SCL           |
| GND                    | GND               | GND               | GND           |
| —                      | —                 | LV (3.3V)         | 3.3V          |

---

### ⚠ WARNING: HV / LV Connection

**Do NOT swap HV and LV connections.**

- **HV must be 5 V**
- **LV must be 3.3 V**

Reversing HV and LV may:
- Break I²C communication
- Permanently damage the NAU7802
- Damage the level shifter itself

Always double-check before powering on.

---

### Wiring Diagram (SVG)

![Wiring diagram](docs/wiring.svg)

- Open SVG directly: [docs/wiring.svg](docs/wiring.svg)

---

### Photos

| Description | Link |
|------------|------|
| PCB (top) | [docs/pcb_top.jpg](docs/pcb_top.jpg) |
| PCB (bottom) | [docs/pcb_bottom.jpg](docs/pcb_bottom.jpg) |
| With 3D printed parts | [docs/with_3d_parts.jpg](docs/with_3d_parts.jpg) |
| Installed in pedal | [docs/installed_in_pedal.jpg](docs/installed_in_pedal.jpg) |
| Overall finished assembly | [docs/final_overview.jpg](docs/final_overview.jpg) |

<details>
<summary>Show photo previews</summary>

![PCB top](docs/pcb_top.jpg)
![PCB bottom](docs/pcb_bottom.jpg)
![With 3D printed parts](docs/with_3d_parts.jpg)
![Installed in pedal](docs/installed_in_pedal.jpg)
![Final overview](docs/final_overview.jpg)

</details>

---

### Usage

When connected to Windows, the device is recognized as a  
**USB joystick with one axis and one button**.

To calibrate the maximum brake force:
- Press the brake **firmly for about 2 seconds**

Calibration is performed automatically.

---

### Notes and Warnings

The **spring terminals on the SparkFun Qwiic Scale – NAU7802 are unreliable**.  
Even if a wire appears to be properly inserted, electrical contact may fail.

It is strongly recommended to:
- Solder pin headers, or
- Replace the terminals with more reliable connectors

---

## 日本語

このプロジェクトは、**Moza CR-P Lite Pedals のブレーキペダルをロードセル方式に改造する**ものです。  
踏力に応じた入力を可能にし、より実車に近いブレーキフィールを得ることを目的としています。

---

### 構成部品

**Arduino Pro Micro 互換ボード**を  
**USB HID ジョイスティックコントローラ**として使用します。

ロードセルには **秋月電子 100kg ロードセル（SC301A）** を使用しています。  
https://akizukidenshi.com/catalog/g/g112036/

ロードセルアンプには **SparkFun Qwiic Scale – NAU7802** を採用しました。  
**320 SPS の高サンプリング**が可能な点を重視しています。
https://www.sparkfun.com/sparkfun-qwiic-scale-nau7802.html


本構成では、
- Arduino 側は **5V**
- NAU7802 側は **3.3V**

となるため、**電源変換**と**I²C レベル変換**が必要です。

---

### 電源変換（5V → 3.3V）

**5V → 3.3V 電源変換基板**を使用しています。  
生成した **3.3V は NAU7802 とレベル変換モジュールの LV ピンの両方に供給**します。

Amazon（日本）:  
https://www.amazon.co.jp/dp/B0BJJBG8C4

**重要**
- NAU7802 は **3.3V 専用**
- 5V を直接接続しない
- GND は必ず共通にする

---

### 使用したレベル変換モジュール

**BSS138 MOSFET を使用した I²C 双方向レベル変換モジュール**を使用しています。

Amazon（日本）:  
https://www.amazon.co.jp/dp/B086WWCJGQ

同等仕様の BSS138 ベースモジュールで代替可能です。

---

### 配線（I²C レベル変換）

I²C 通信のみを使用します。  
必要な信号は **SDA / SCL / VCC / GND** です。

- HV 側：5V（Arduino）
- LV 側：3.3V（電源基板）

| Arduino Pro Micro (5V) | レベル変換（HV） | レベル変換（LV） | NAU7802 (3.3V) |
|------------------------|------------------|------------------|----------------|
| SDA                    | HV1              | LV1              | SDA            |
| SCL                    | HV2              | LV2              | SCL            |
| GND                    | GND              | GND              | GND            |
| —                      | —                | LV (3.3V)        | 3.3V           |

---

### ⚠ 警告：HV / LV の逆接続

**HV と LV を逆に接続しないでください。**

- **HV は 5V**
- **LV は 3.3V**

逆接続すると、
- I²C 通信不能
- NAU7802 破損
- レベル変換基板破損

の可能性があります。必ず通電前に確認してください。

---

### 配線図（SVG）

![配線図](docs/wiring.svg)

- SVG を直接開く: [docs/wiring.svg](docs/wiring.svg)

---

### 写真

| 内容 | リンク |
|------|------|
| 基板（表） | [docs/pcb_top.jpg](docs/pcb_top.jpg) |
| 基板（裏） | [docs/pcb_bottom.jpg](docs/pcb_bottom.jpg) |
| 3Dプリント部品と組み合わせた状態 | [docs/with_3d_parts.jpg](docs/with_3d_parts.jpg) |
| ペダルに組み込んだ状態 | [docs/installed_in_pedal.jpg](docs/installed_in_pedal.jpg) |
| 完成状態全体 | [docs/final_overview.jpg](docs/final_overview.jpg) |

<details>
<summary>写真プレビュー</summary>

![基板 表](docs/pcb_top.jpg)
![基板 裏](docs/pcb_bottom.jpg)
![3D部品組合せ](docs/with_3d_parts.jpg)
![ペダル組込み](docs/installed_in_pedal.jpg)
![完成全体](docs/final_overview.jpg)

</details>

---

### 使用方法

Windows に接続すると  
**1軸＋1ボタンの USB ジョイスティック**として認識されます。

最大踏力のキャリブレーションは、  
**ブレーキを約2秒間、強めに踏み込む**ことで自動的に行われます。

---

### 注意事項

**SparkFun Qwiic Scale – NAU7802 のスプリングターミナルは接触不良を起こしやすい**です。

- ピンヘッダをはんだ付けする  
- 信頼性の高い端子に交換する  

ことを強く推奨します。
