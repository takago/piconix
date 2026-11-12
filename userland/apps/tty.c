/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 現在タスクが接続されているTTYの情報を表示する． */

#include "unistd.h"
#include "stdio.h"

/* 引数を受け付けず，現在のタスクが属するTTYのデバイス名を表示する． */
int main(int argc, char **argv) {
    (void)argv;
    if(argc!=1) { puts("usage: tty"); return 1; }
    int tty=get_tty();
    if(tty==0) { puts("/dev/ttyV0"); return 0; }
    if(tty==1) { puts("/dev/ttyS1"); return 0; }
    if(tty==2) { puts("/dev/ttyN0"); return 0; }
    if(tty==3) { puts("/dev/pts/0"); return 0; }
    if(tty==5) { puts("/dev/pts/1"); return 0; }
    if(tty==4) { puts("/dev/ttyV1"); return 0; }
    puts("not a tty");
    return 0;
}
