/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "unistd.h"
#include "stdio.h"
int main(int argc, char **argv) {
    (void)argv;
    if(argc!=1) {puts("usage: pwd");return 1;}
    char cwd[128]; if(!getcwd(cwd,sizeof cwd)) {puts("pwd: failed");return 1;} puts(cwd);return 0; }
