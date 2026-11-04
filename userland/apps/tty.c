/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 現在の端末情報を表示する． */

#include "unistd.h"
#include "stdio.h"
/* 現在タスクが接続されているTTYの情報を表示する． */
int main(int argc, char **argv) {
    (void)argv;
    if(argc!=1) {puts("usage: tty");return 1;}int t=get_tty();puts(t==0?"/dev/console":t==1?"/dev/ttyS1":t==2?"/dev/ttyN0":"not a tty");return 0;}
