/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "unistd.h"
#include "stdio.h"
int main(void) {int t=get_tty();puts(t==0?"/dev/console":t==1?"/dev/ttyS1":t==2?"/dev/ttyN0":"not a tty");return 0;}
