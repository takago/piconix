# I²C1 and i2cctl — User Guide

[日本語](../ja/I2C.md) | [Devices](DEVICES.md) | [Python](PYTHON.md)

PicoNIX uses the RP2040 I²C1 controller as a 100 kHz master. GP26 is SDA and GP27 is SCL. ADC0 and ADC1 are unavailable; ADC2 on GP28 and the internal temperature sensor remain available. I²C0 is not exposed.

**Wiring:** SDA and SCL need suitable pull-up resistors to 3.3 V. Check whether the module already includes them. Do not apply 5 V signals directly to RP2040 GPIO. Connect grounds. Consider external pull-ups appropriate to the wiring rather than relying solely on weak internal pull-ups.

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

## C application interface

Open `/dev/i2c/1` with `O_RDWR`, prepare `piconix_i2c_transfer_t` from `include/piconix_i2c.h`, and issue `OS_I2C_TRANSFER`. A combined write/read uses a repeated START. A zero-length transfer probes the address.

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
- Python provides [I²C helpers](PYTHON.md). Hardware behavior and the 24 KiB slot fit remain unverified.
