# I²C1とi2cctl — 利用ガイド

*技術リファレンス*

PicoNIXはRP2040のI²C1を100 kHzのマスターモードで使用します．GP26がSDA，GP27がSCLです．ADC0とADC1は利用できず，GP28のADC2と内蔵温度センサは維持されます．I²C0は提供しません．

**配線：** GP26/27の接続方法，プルアップ抵抗と電圧の注意点は[ハードウェアの配線ガイド](../hardware/WIRING.md)にまとめています．

## コマンド

| コマンド | 説明 |
|---|---|
| `i2cctl scan` | 7-bitアドレス0x08〜0x77を探索 |
| `i2cctl read ADDR COUNT` | COUNTバイト受信 |
| `i2cctl write ADDR BYTE...` | バイト列を送信 |
| `i2cctl xfer ADDR READ_COUNT [BYTE...]` | 書き込み後，Repeated STARTで読み込み |

数値は10進数または`0x`付き16進数で指定できます．1回の送信・受信はそれぞれ最大32バイトです．`xfer`では読み取りバイト数を先に指定します．

```sh
i2cctl scan
i2cctl write 0x23 0x01
i2cctl write 0x23 0x10
i2cctl read 0x23 2
i2cctl xfer 0x48 2 0x00
```

## BH1750照度センサ

BH1750（標準アドレス`0x23`）を接続した場合は，`i2cctl scan`で検出を確認し，`bh1750 5`で照度を5回測定できます．Pythonの連続測定例は[Pythonガイド](../../development/apps/python/PYTHON.md)を参照してください．

```sh
i2cctl scan
bh1750 5
```

## C言語からの操作

`/dev/i2c/1`を`O_RDWR`で開き，`include/piconix_i2c.h`の`piconix_i2c_transfer_t`を使って`OS_I2C_TRANSFER`を発行します．読み書きを同時に指定すると，間にRepeated STARTが入ります．`i2cctl scan`は1バイト読み取りによる簡易検出です．読み取りに対応しない機器を検出できない場合があり，一部の機器では読み取りに副作用があります．

```c
#include <stdint.h>
#include "fcntl.h"
#include "unistd.h"
#include "piconix_i2c.h"

int main(void) {
    int fd = open("/dev/i2c/1", O_RDWR);
    if (fd < 0) return 1;
    piconix_i2c_transfer_t t = {0};
    t.address = 0x23;
    t.write_len = 1;
    t.write_data[0] = 0x10;
    int rc = ioctl(fd, OS_I2C_TRANSFER, (int)(uintptr_t)&t);
    close(fd);
    return rc < 0 ? 1 : 0;
}
```

## 注意事項

- `scan`はデバイスへ実際にアクセスします．特殊なデバイスでは副作用があり得ます．
- NACKやタイムアウトは転送失敗として報告されます．`i2cctl`の終了状態を確認してください．
- このインタフェースは7-bitアドレスのマスター転送用です．10-bitアドレス，スレーブモード，SMBus専用操作は提供しません．
- Pythonからは[I²C関数](../../development/apps/python/PYTHON.md)を利用できます．BH1750の読み書きと照度測定は実機で確認済みです．

---

**関連リンク：** [リファレンス目次](../README.md) ｜ [学習ロードマップ](../../ROADMAP.md) ｜ [English](../../../en/reference/devices/I2C.md) ｜ [デバイス](DEVICES.md)
