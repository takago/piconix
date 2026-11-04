/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 外部ユーザプログラムの最小例． */

/* 外部PicoNIXプログラムの最小例． */
#include "stdio.h"

/* PicoNIXの動作確認用メッセージを表示する． */
int main(int argc, char *argv[])
{
    if(argc>2) {puts("usage: hello [name]");return 1;}
    if (argc > 1)
        printf("Hello, %s!\n", argv[1]);
    else
        puts("Hello, PicoNIX!");
    return 0;
}
