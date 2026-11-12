#!/usr/bin/env python3
"""Run the actual lazy-shell switch routine against a small host-side scheduler stub."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[2]
s=(root/'kernel/kernel.c').read_text()
source=s[s.index('/* Ctrl+T toggles between independent CDC TTY0'):s.index('/* アイドル側からUSB／Telnet入力')]
harness=r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#define NO_TRACE
#define CDC_VCONSOLE_TTY 4
#define DEV_CDC_VCONSOLE 278
#define OS_EAGAIN (-11)
typedef struct {int pid,tty,parent_pid,fd[3];char cwd[16];} task_t;
typedef struct {void *entry;const char *name;} program_t;
static task_t tasks[6];
static program_t programs[3];
static int tty_shell_pid[5]={7,0,0,0,0};
static int free_slot=2,spawn_calls,repaint_calls,beeps;
static unsigned char cdc_active;
static uint32_t save_and_disable_interrupts(void) {return 0;}
static void restore_interrupts(uint32_t value) {(void)value;}
static int free_program_slot(void) {return free_slot;}
static int spawn_in_slot(int slot,int program_id,void *entry,const char *name,int argc,
                         const char *const *args,const uint8_t *raw,unsigned raw_size) {
    (void)program_id;(void)entry;(void)name;(void)argc;(void)args;(void)raw;(void)raw_size;
    spawn_calls++;
    tasks[slot].pid=42;
    return 42;
}
static void fd_set(task_t *t,int fd,int dev,int pos,int flags,int other) {
    (void)pos;(void)flags;(void)other;
    t->fd[fd]=dev;
}
static void tty_put(char c) {if(c=='\a')beeps++;}
static void cdc_vconsole_begin_repaint(void) {repaint_calls++;}
'''
main=r'''
int main(void) {
    free_slot=-1;
    cdc_vconsole_switch();
    assert(cdc_active==0 && beeps==1 && repaint_calls==0);
    free_slot=2;
    cdc_vconsole_switch();
    assert(cdc_active==1 && spawn_calls==1 && repaint_calls==1);
    assert(tty_shell_pid[0]==7 && tty_shell_pid[4]==42);
    assert(tasks[2].tty==4 && tasks[2].parent_pid==0);
    assert(tasks[2].fd[0]==278 && tasks[2].fd[1]==278 && tasks[2].fd[2]==278);
    assert(!strcmp(tasks[2].cwd,"/"));
    cdc_vconsole_switch();
    assert(cdc_active==0 && spawn_calls==1);
    cdc_vconsole_switch();
    assert(cdc_active==1 && spawn_calls==1 && repaint_calls==3);
    puts("PASS: second shell is lazy, owns TTY4, and unavailable slots leave CDC0 active");
    return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
 c=Path(tmp)/'switch.c'; exe=Path(tmp)/'switch'
 c.write_text(harness+source+main)
 subprocess.run(['clang','-std=c11','-O2','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
