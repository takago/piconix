/* SPDX-License-Identifier: GPL-3.0-or-later */
/* RAMファイルを削除する． */

#include "unistd.h"
#include "stdio.h"
/* 指定したRAMファイルをVFSのunlinkで削除する． */
int main(int argc,char *argv[]) { if(argc!=2) {puts("usage: rm PATH");return 1;} if(unlink(argv[1])<0) {puts("rm: cannot remove");return 1;} return 0; }
