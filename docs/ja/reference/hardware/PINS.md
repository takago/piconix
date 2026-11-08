# ピン割当とボード端子

*技術リファレンス*

**実装上の正本は`kernel/pins.h`**です．このページは添付ソースの`GPn_MODE`と照合して作成しています．`pins`コマンドでも割当を確認できます．物理端子番号の解釈はPico/Pico Wの基板資料を参照してください．

## 標準ピン用途

| GPIO | 標準モード | 役割・注意 |
|---|---|---|
| GP0～15 | `PIN_GPIO` | LEDマトリクス表示中はGP0～15を占有 |
| GP16・GP17 | `PIN_GPIO` | 押しボタン入力の接続例．通常はプルアップ |
| GP18・GP19 | `PIN_PWM` | PWMスライス1 A/B．周波数は共通 |
| GP20・GP21 | `PIN_UART` | UART1 TX/RX．UART0コンソールは使用しない |
| GP22 | `PIN_GPIO` | 汎用GPIO |
| GP23～25 | `PIN_GPIO`（`pins.h`上） | Pico WではCYW43無線用に予約され，GPIOデバイスから操作できない |
| GP26・GP27 | `PIN_I2C` | I²C1 SDA/SCL．現在はADC0/1として使わない |
| GP28 | `PIN_ADC` | ADC2（外部アナログ入力） |

**Pico Wでの注意：** `pins.h`の`GP23_MODE`〜`GP25_MODE`が`PIN_GPIO`でも，Pico Wビルドでは無線チップが使用するため汎用GPIOとして扱いません．`/dev/gpio/23`〜`25`は公開されず，`pins`では予約状態として表示されます．GP29も無線用の内部ピンです．Pico WのオンボードLEDはGP25ではなくCYW43の`WL_GPIO0`に接続されています．

### 標準ピン配置図（Pico / Pico W）

次の図はUSB端子を上にした**基板表面側**の40ピン配置です．左右の番号はPico本体の物理端子番号で，`R`/`C`はPicoNIXのLED表示座標です．ピン機能の定義は`kernel/pins.h`を正とします．

```text
                      USB
              +-------------------+
       C7  1 |GP0          VBUS| 40
       C6  2 |GP1          VSYS| 39
           3 |GND           GND| 38
       C5  4 |GP2        3V3_EN| 37
       C4  5 |GP3           3V3| 36
       C3  6 |GP4      ADC_VREF| 35
       C2  7 |GP5          GP28| 34 ADC2
           8 |GND           GND| 33
       C1  9 |GP6          GP27| 32 I2C1 SCL
       C0 10 |GP7          GP26| 31 I2C1 SDA
       R0 11 |GP8           RUN| 30
       R1 12 |GP9          GP22| 29 GPIO
          13 |GND           GND| 28
       R2 14 |GP10         GP21| 27 UART1 RX
       R3 15 |GP11         GP20| 26 UART1 TX
       R4 16 |GP12         GP19| 25 PWM B
       R5 17 |GP13         GP18| 24 PWM A
          18 |GND           GND| 23
       R6 19 |GP14         GP17| 22 Switch 2
       R7 20 |GP15         GP16| 21 Switch 1
              +-------------------+
```

Pico WのGP23～25は無線チップが使用するため，PicoNIXの汎用GPIO操作から保護します．PicoではGP25が基板上LED用ですが，Pico WのLEDはCYW43側の`WL_GPIO0`です．

## 設定の変更

`kernel/pins.h`の`GPn_MODE`を編集して再ビルドします．モードは`PIN_GPIO`，`PIN_ADC`，`PIN_PWM`，`PIN_UART`，`PIN_I2C`です．対応していないピン・モードの組み合わせはコンパイル時に拒否します．設定はデバイス一覧，`open()`の可否，周辺機器の初期化に共通して反映されます．

```c
#define GP18_MODE PIN_PWM
#define GP19_MODE PIN_PWM
#define GP26_MODE PIN_I2C
#define GP27_MODE PIN_I2C
#define GP28_MODE PIN_ADC
#define PWM_DEFAULT_FREQ 1000u
#define PWM_DEFAULT_DUTY 0u
```

GP26／27はI²C1専用の固定割り当てで，`PIN_GPIO`や`PIN_ADC`に変更するとビルド時に拒否されます．GP28のADC2は利用できます．USB CDCを主コンソールとし，GP0/1はLEDマトリクス用のGPIO，GP20/21はUART1用です．UART用に予約したピンはGPIOデバイスとして公開されません．

## 接続方法

[配線と電気的注意](WIRING.md)には，LEDマトリクスの端子対応，スイッチ，PWM，UART，I²C，ADCの具体例があります．

---

**関連リンク：** [ハードウェア目次](README.md) ｜ [デバイスの操作](../devices/DEVICES.md) ｜ [English](../../../en/reference/hardware/PINS.md)
