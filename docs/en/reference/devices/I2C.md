# I²C1 and i2cctl — User Guide

*Technical reference*

PicoNIX uses the RP2040 I²C1 controller as a 100 kHz master. GP26 is SDA and GP27 is SCL. ADC0 and ADC1 are unavailable; ADC2 on GP28 and the internal temperature sensor remain available. I²C0 is not exposed.

**Wiring:** See [hardware wiring](../hardware/WIRING.md) for GP26/27 connections, pull-up resistors, and electrical precautions.

## Commands

| Command | Meaning |
|---|---|
| `i2cctl scan` | Probe 7-bit addresses 0x08–0x77 |
| `i2cctl read ADDR COUNT` | Read COUNT bytes |
| `i2cctl write ADDR BYTE...` | Write bytes |
| `i2cctl xfer ADDR READ_COUNT [BYTE...]` | Write then read using repeated START |

Numbers may be decimal or `0x`-prefixed hexadecimal. Each write and read phase is limited to 32 bytes. For `xfer`, the read count precedes optional outgoing bytes.

```sh
i2cctl scan
i2cctl write 0x23 0x01
i2cctl write 0x23 0x10
i2cctl read 0x23 2
i2cctl xfer 0x48 2 0x00
```

## BH1750 ambient-light sensor

With a BH1750 (default address `0x23`) connected, use `i2cctl scan` to check detection and `bh1750 5` to take five illuminance measurements. See the [Python guide](../../development/apps/python/PYTHON.md) for a continuous measurement example.

```sh
i2cctl scan
bh1750 5
```

## C application interface

Open `/dev/i2c/1` with `O_RDWR`, prepare `piconix_i2c_transfer_t` from `include/piconix_i2c.h`, and issue `OS_I2C_TRANSFER`. A combined write/read uses a repeated START. `i2cctl scan` uses a one-byte read as a simple probe. Read-incompatible devices may be missed, and some devices may have read side effects.

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

## Notes

- `scan` actively probes devices and can have side effects on unusual peripherals.
- NACK and timeout conditions cause transfer failure; check the command exit status.
- Only 7-bit master transfers are supported; there is no 10-bit addressing, slave mode, or dedicated SMBus protocol.
- Python provides [I²C helpers](../../development/apps/python/PYTHON.md). BH1750 reads, writes, and illuminance measurements have been tested on hardware.

---

**Related links:** [Reference index](../README.md) | [Learning roadmap](../../ROADMAP.md) | [日本語](../../../ja/reference/devices/I2C.md) | [Devices](DEVICES.md)
