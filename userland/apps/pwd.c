/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 現在の作業ディレクトリを表示する． */

#include "unistd.h"
#include "stdio.h"
/* 現在の作業ディレクトリを表示する． */
int main(int argc, char **argv) {
    (void)argv;
    if(argc!=1) {puts("usage: pwd");return 1;}
    char cwd[128]; if(!getcwd(cwd,sizeof cwd)) {puts("pwd: failed");return 1;} puts(cwd);return 0; }
