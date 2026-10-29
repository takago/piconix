/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Minimal external PicoNIX application. */
#include "stdio.h"

int main(int argc, char *argv[])
{
    if (argc > 1)
        printf("Hello, %s!\n", argv[1]);
    else
        puts("Hello, PicoNIX!");
    return 0;
}
