/* SPDX-License-Identifier: GPL-3.0-or-later */
/* BH1750 continuous high-resolution illuminance monitor for PicoNIX. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "../../include/piconix_i2c.h"
#include <stdint.h>

#define BH1750_ADDR 0x23
#define BH1750_POWER_ON 0x01
#define BH1750_POWER_DOWN 0x00
#define BH1750_CONT_H_RES 0x10

static int transfer(int fd, uint8_t addr, int command, uint8_t *data)
{
    piconix_i2c_transfer_t t = {0};
    t.address = addr;
    if (command >= 0) {
        t.write_len = 1;
        t.write_data[0] = (uint8_t)command;
    } else {
        t.read_len = 2;
    }
    int rc = ioctl(fd, OS_I2C_TRANSFER, (int)(uintptr_t)&t);
    if (rc < 0) return rc;
    if (data) {
        data[0] = t.read_data[0];
        data[1] = t.read_data[1];
    }
    return 0;
}

int main(int argc, char **argv)
{
    int samples = 0; /* 0 = continuous */
    if (argc > 2) {
        puts("usage: bh1750 [count] (0 = continuous)");
        return 1;
    }
    if (argc == 2) {
        char *end;
        long count = strtol(argv[1], &end, 10);
        if (!argv[1][0] || *end || count < 0 || count > 100000) {
            puts("bh1750: count must be 0..100000");
            return 1;
        }
        samples = (int)count;
    }

    int fd = open("/dev/i2c/1", O_RDWR);
    if (fd < 0) {
        puts("bh1750: cannot open /dev/i2c/1");
        return 1;
    }
    if (transfer(fd, BH1750_ADDR, BH1750_POWER_ON, 0) < 0 ||
        transfer(fd, BH1750_ADDR, BH1750_CONT_H_RES, 0) < 0) {
        puts("bh1750: sensor initialization failed");
        close(fd);
        return 1;
    }
    puts("BH1750 (I2C1, address 0x23): illuminance [lx]");
    int status = 0;
    for (int i = 0; samples == 0 || i < samples; ++i) {
        uint8_t data[2];
        msleep(200);
        if (transfer(fd, BH1750_ADDR, -1, data) < 0) {
            puts("bh1750: read failed");
            status = 1;
            break;
        }
        unsigned raw = ((unsigned)data[0] << 8) | data[1];
        /* BH1750: lux = raw / 1.2. Display 3 decimal places. */
        unsigned milli_lux = raw * 2500u / 3u;
        printf("%u.%03u lx\n", milli_lux / 1000u, milli_lux % 1000u);
    }
    (void)transfer(fd, BH1750_ADDR, BH1750_POWER_DOWN, 0);
    close(fd);
    return status;
}
