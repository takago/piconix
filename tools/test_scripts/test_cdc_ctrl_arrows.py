#!/usr/bin/env python3
"""Exercise the real CDC Ctrl+arrow decoder with a minimal host TTY stub."""
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[2]
s=(root/'kernel/kernel.c').read_text()
part=s[s.index('/* Cycle V0 single'):s.index('/* Ctrl+T toggles')]
harness=r'''
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#define NO_TRACE
#define CDC_VCONSOLE_TTY 4
static unsigned char cdc_active,cdc_dual_mode;
static int switch_calls,repaints,tty_len;
static unsigned char tty_received[64], tty_target[64];
static uint32_t now;
static uint32_t time_us_32(void) {return now;}
static void tty_input_char(int tty,uint8_t c) {assert(tty_len<64);tty_target[tty_len]=(unsigned char)tty;tty_received[tty_len++]=c;}
static void cdc_vconsole_switch(void) {cdc_active^=1;switch_calls++;}
static void cdc_vconsole_begin_repaint(void) {repaints++;}
'''
main=r'''
static void feed(const char *s) {for(;*s;s++){int received=0;cdc_key_input((uint8_t)*s,&received);}}
int main(void) {
 /* Ctrl+Right cycles V0 -> V1 -> Dual -> V0. */
 feed("\033[1;5C");assert(cdc_active==1 && !cdc_dual_mode && switch_calls==1);
 feed("\033[1;5C");assert(cdc_active==1 && cdc_dual_mode && repaints==1);
 feed("\033[1;5A");assert(cdc_active==0 && cdc_dual_mode && switch_calls==2);
 feed("\033[1;5B");assert(cdc_active==1 && cdc_dual_mode && switch_calls==3);
 feed("\033[1;5C");assert(cdc_active==0 && !cdc_dual_mode && switch_calls==4);
 /* In single view Up/Down must not change input focus. */
 feed("\033[1;5B");assert(cdc_active==0 && switch_calls==4);
 feed("\033[1;5A");assert(cdc_active==0 && switch_calls==4);
 /* Ctrl+Left cycles the other way: V0 -> Dual -> V1 -> V0. */
 feed("\033[1;5D");assert(cdc_dual_mode && cdc_active==0);
 feed("\033[1;5D");assert(!cdc_dual_mode && cdc_active==1);
 feed("\033[1;5D");assert(!cdc_dual_mode && cdc_active==0);
 /* Unchanged modes / plain arrows / ESC must keep working. */
 feed("\033[A");assert(tty_len==3 && tty_received[0]==27 && tty_received[1]=='[' && tty_received[2]=='A');
 tty_len=0;
 feed("\033");assert(cdc_key_len==1);now=120000;int received=0;cdc_key_flush(&received);
 assert(received==1 && tty_len==1 && tty_received[0]==27 && cdc_key_len==0);
 tty_len=0;now=0;
 feed("\033[1;");feed("x");assert(tty_len==5 && tty_received[4]=='x');
 tty_len=0;
 feed("\024");assert(cdc_active==1 && !cdc_dual_mode);
 feed("\007");assert(cdc_dual_mode);
 puts("PASS: cyclic Ctrl+Left/Right, dual focus, legacy keys, ANSI and ESC");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
 path=Path(d)/'test.c'; exe=Path(d)/'test'
 path.write_text(harness+part+main)
 subprocess.run(['clang','-std=c11','-Wall','-Wextra','-Werror',str(path),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
