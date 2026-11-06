/* SPDX-License-Identifier: GPL-3.0-or-later */
/* GPIOの点滅を使う小さなプログラム例． */

/* 共通ユーザシステムコールAPIを使う独立したPICの点滅プログラム． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "../../include/piconix.h"  /* GPIOを出力に設定する． */

int main(int argc, char *argv[]) {
    (void)argv;
    if(argc!=1) {puts("usage: blink");return 1;}
    int fd = open("/dev/gpio/25", O_WRONLY);
    if (fd < 0)
        return 0;
    ioctl(fd, OS_GPIO_OUTPUT, 0);
    printf("blink: GPIO25 ready\r\n");
    for (;;) {
        char on = '1', off = '0';
        write(fd, &on, 1);
        msleep(500);
        write(fd, &off, 1);
        msleep(500);
    }
}
