/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "unistd.h"
#include "stdio.h"
int main(void) { char cwd[128]; if(!getcwd(cwd,sizeof cwd)) {puts("pwd: failed");return 1;} puts(cwd);return 0; }
