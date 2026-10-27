/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Standalone PIC Blink image using the shared user-space syscall API. */
#include "unistd.h"
#include "fcntl.h"
#include "../include/piconix.h"  /* OS_GPIO_OUTPUT */

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    int fd = open("/dev/gpio/25", O_WRONLY);
    if (fd < 0) return 0;
    ioctl(fd, OS_GPIO_OUTPUT, 0);
    for (;;) {
        char on = '1', off = '0';
        write(fd, &on, 1);
        sleep(500);
        write(fd, &off, 1);
        sleep(500);
    }
}
