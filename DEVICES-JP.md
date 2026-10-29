# PicoNIX デバイスガイド

[English](DEVICES-EN.md) | [README](README-JP.md)

PicoNIXの`/dev`は仮想デバイスファイルです．RAM上にファイルを作るのではなく，`open()`，`read()`，`write()`などを通じてカーネルが周辺機器を操作します．

## デバイス一覧

| デバイス | 内容 |
|---|---|
| `/dev/console` | 主コンソール（USB CDCまたはUART0構成） |
| `/dev/uart1` | UART1端末 |
| `/dev/gpio/` | `pins.h`でGPIO指定されたピン |
| `/dev/adc/` | `pins.h`でADC指定された入力および内部温度センサー |
| `/dev/pwm/` | `pins.h`でPWM指定された出力と共通周波数 |
| `/dev/mem` | 許可されたRAM・Flash領域の読み出し |
| `/dev/trace` | トレースイベントのバイナリ読み出し |

## GPIO

GPIOとして使用できるピンは`src/pins.h`の`GPn_MODE`で決まります．`ls /dev/gpio`で一覧を確認できます．GPIOの値・方向・プルアップ／プルダウンは既存のGPIOデバイスインターフェースから操作します．ADC・PWM・UARTに割り当てたピンはGPIO一覧に現れず，GPIOとしての操作も拒否します．

## ADC・温度センサー

既定の割り当ては次のとおりです．

| デバイス | 入力 | 出力 |
|---|---|---|
| `/dev/adc/0` | GP26 | 12ビットADC値（0～4095） |
| `/dev/adc/1` | GP27 | 12ビットADC値（0～4095） |
| `/dev/adc/2` | GP28 | 12ビットADC値（0～4095） |
| `/dev/adc/temp` | RP2040内蔵温度センサー | 摂氏の概算値 |

```sh
cat /dev/adc/0
cat /dev/adc/temp
watch 1 cat /dev/adc/temp
```

ADC値はファイルを開くときに取得し，十進テキストとして読み出します．温度は基準電圧3.3 Vを仮定した概算値で，精密な温度計測には向きません．GPIO26～28のモードをGPIOに変更すると，対応するADCデバイスは公開されません．内部温度センサーはピン設定とは独立です．

## PWM

既定ではGP18とGP19をPWM出力に割り当てます．両方とも同じPWMスライスを使用するため，**周波数は共通**ですがDuty比は個別に設定できます．起動時は**1 kHz，両方ともDuty 0%**です．

PWMスライス1のチャネルA（`/dev/pwm/1/a`）はGPIO18，チャネルB（`/dev/pwm/1/b`）はGPIO19に対応します．両チャネルは周波数を共有します．

```sh
cat /dev/pwm/1/freq
cat /dev/pwm/1/a
cat /dev/pwm/1/b
echo 2000 > /dev/pwm/1/freq
echo 50 > /dev/pwm/1/a
echo 0 > /dev/pwm/1/a
```

`/dev/pwm/1/freq`は8～100000 Hz，`/dev/pwm/1/a`と`/dev/pwm/1/b`は0～100%の整数をテキストで読み書きします．ハードウェアの分周比とカウンタ範囲によって，設定周波数には丸め誤差が生じ得ます．PWM出力に割り当てていないピンのデバイスは公開されません．

## ピン割り当ての変更

`src/pins.h`の`GPn_MODE`を編集して再ビルドします．モードは`PIN_GPIO`，`PIN_ADC`，`PIN_PWM`，`PIN_UART`です．対応していないピン・モードの組み合わせはコンパイル時に拒否します．設定はデバイス一覧，`open()`の可否，周辺機器の初期化に共通して反映されます．

```c
#define GP18_MODE PIN_PWM
#define GP19_MODE PIN_PWM
#define GP26_MODE PIN_ADC
#define GP27_MODE PIN_ADC
#define GP28_MODE PIN_ADC
#define PWM_DEFAULT_FREQ 1000u
#define PWM_DEFAULT_DUTY 0u
```

すべての利用可能な汎用ピンをGPIOとして使いたい場合は，PWM・ADCに割り当てたピンを`PIN_GPIO`に変更します．標準のUSB CDCコンソール構成ではGP0/1をGPIOとして使用でき，GP20/21はUART1用です．UART0コンソール構成に変更する場合は，GP0/1を`PIN_UART`に変更してください．UART用に予約したピンはGPIOデバイスとして公開されません．

## 標準構成の接続例

| ピン | 用途 | 注意 |
|---|---|---|
| GP0～7 | 8×8 LEDマトリクスの列（カソード） | LOWで列を選択 |
| GP8～15 | 8×8 LEDマトリクスの行（アノード） | HIGHで行を選択 |
| GP16／17 | 押しボタン | GNDへ接続，内部プルアップ |
| GP18／19 | PWM | 周波数は共通，GP18はブザーなど |
| GP20／21 | UART1 TX／RX | 115200 bps，8N1 |
| GP26～28 | ADC0～2 | 0～3.3 Vの入力 |

UART機器はGNDを共通にし，3.3 Vロジックを使用してください．RS-232の電圧を直接接続しないでください．LEDマトリクスは電流制限抵抗と適切な駆動回路が必要であり，GPIOへの直接接続を想定した回路ではありません．`pins`で現状のピン割り当てを確認できます．

## UART・コンソール

主コンソールは標準ではUSB CDCです．UART0コンソール構成では`pins.h`でGP0/1を`PIN_UART`に変更する必要があります．UART1は`/dev/uart1`で利用し，`getty /dev/uart1 sh &`で別のShellを起動できます．詳細は[UART-JP.md](UART-JP.md)を参照してください．

## 制限と注意

- ADCとGPIO，PWMとGPIOは同じピンで同時には使用できません．
- GP18とGP19のPWM周波数は独立に変更できません．
- `/dev/mem`は許可されたメモリ領域の読み出し専用です．
- `/dev/trace`はバイナリイベントを返します．利用方法は[TRACE-JP.md](TRACE-JP.md)を参照してください．
- ドライバは`src/gpio.c`，`src/adc.c`，`src/pwm.c`に分離しています．FD管理は`src/kernel.c`が担当します．
