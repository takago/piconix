/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Restore PicoNIX console input and a usable ANSI terminal display. */
#include "unistd.h"
#include "stdio.h"

int main(void) {
    if (ioctl(0, OS_TTY_SET_CANON, 1) < 0 ||
        ioctl(0, OS_TTY_SET_ECHO, 1) < 0 ||
        ioctl(0, OS_TTY_PERSIST, 0) < 0) {
        puts("reset: console terminal required");
        return 1;
    }
    /* Reset rendition, show cursor, clear screen, and move home. */
    printf("\033[0m\033[?25h\033[2J\033[H");
    return 0;
}
