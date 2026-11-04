/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 端末表示と入力モードを復旧する． */

/* PicoNIXのコンソール入力とANSI端末表示を復旧する． */
#include "unistd.h"
#include "stdio.h"

/* TTYのカノニカル入力・エコーとANSI表示を復元する．OS自体の再起動は行わない． */
int main(int argc, char **argv) {
    (void)argv;
    if(argc!=1) {puts("usage: reset");return 1;}
    if (ioctl(0, OS_TTY_SET_CANON, 1) < 0 ||
        ioctl(0, OS_TTY_SET_ECHO, 1) < 0 ||
        ioctl(0, OS_TTY_PERSIST, 0) < 0) {
        puts("reset: console terminal required");
        return 1;
    }
    /* 別画面表示とスクロール設定を解除し，通常画面へ復元する． */
    printf("\033[?47l\033[?1047l\033[?1049l\033[?1007l"
           "\033[r\033[0m\033[?25h\033[2J\033[H");
    return 0;
}
