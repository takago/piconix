#!/usr/bin/env python3
"""Compile and call the real split TTY/GPIO ioctl helpers with host fakes."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
src = (root/'kernel/kernel.c').read_text()
def extract(signature):
    a = src.index(signature)
    b = src.index('{',a)
    depth=0
    for i in range(b,len(src)):
        if src[i]=='{': depth+=1
        elif src[i]=='}':
            depth-=1
            if not depth: return src[a:i+1]
    raise AssertionError(signature)

helpers = '\n'.join(extract(s) for s in (
    'static int tty_for_device(', 'static int tty_device(',
    'static int device_ioctl_tty(', 'static int device_ioctl_gpio_status('
))
boilerplate = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "piconix.h"
#include "gpio_status.h"
#define NO_TRACE
#define DEV_CONSOLE 0
#define DEV_CDC_VCONSOLE 278
#define CDC_VCONSOLE_TTY 4
#define DEV_UART1 30
#define DEV_WIFI_TTY 97
#define DEV_PTY_SLAVE 275
#define PTY_TTY 3
#define DEV_GPIO_VALUE_BASE 1
#define DEV_GPIO_VALUE_END 29
#define UNUSED 0
#define DEAD 3
#define OS_FD_MAX 16
#define OS_TTY_COUNT 5
typedef struct {int pid,state,fd[OS_FD_MAX];} task_t;
static task_t tasks[3];
static int current;
static int tty_claim_pid[5], foreground_pid[5], tty_shell_pid[5];
static uint8_t tty_canonical[5]={1,1,1,1,1},tty_echo[5]={1,1,1,1,1},tty_onlcr[5]={1,1,1,1,1},tty_interrupt[5]={1,1,1,1,1};
static int tty_mode_owner[5],tty_restore_pid[5];
static struct {int head,tail;} rx[2];
static int resets;
static int tty_owner_pid(int tty) {return tty_claim_pid[tty] ? tty_claim_pid[tty] :
    (foreground_pid[tty] ? foreground_pid[tty] : tty_shell_pid[tty]);}
static void tty_mode_reset(int tty) {(void)tty;resets++;}
typedef int gpio_function_t;
#define GPIO_FUNC_PWM 1
#define GPIO_FUNC_I2C 2
#define GPIO_FUNC_UART 3
#define GPIO_FUNC_NULL 4
#define GPIO_FUNC_SIO 5
#define GPIO_OUT 1
static int fb;
static int fn;
static int fb_active(void) {return fb;}
static int pin_adc(unsigned n) {(void)n;return 1;}
static int pin_reserved(unsigned n) {(void)n;return 0;}
static int gpio_get_function(unsigned pin) {(void)pin;return fn;}
static int gpio_get_dir(unsigned pin) {(void)pin;return GPIO_OUT;}
static int gpio_get(unsigned pin) {(void)pin;return 1;}
static int gpio_is_pulled_up(unsigned pin) {(void)pin;return 1;}
static int gpio_is_pulled_down(unsigned pin) {(void)pin;return 0;}
'''
checks = r'''
int main(void) {
    tasks[0].pid=12; tasks[0].state=1;
    tasks[0].fd[3]=DEV_CONSOLE;
    tasks[0].fd[4]=DEV_GPIO_VALUE_BASE;
    tasks[0].fd[5]=DEV_UART1;
    assert(device_ioctl_tty(DEV_CONSOLE,OS_TTY_GET_CANON,0)==OS_EIO);
    foreground_pid[0]=tasks[0].pid;
    assert(device_ioctl_tty(DEV_CONSOLE,OS_TTY_SET_CANON,0)==0);
    assert(device_ioctl_tty(DEV_CONSOLE,OS_TTY_GET_CANON,0)==0);
    assert(device_ioctl_tty(DEV_CONSOLE,OS_TTY_GET_INTR,0)==1);
    assert(device_ioctl_tty(DEV_CONSOLE,OS_TTY_SET_INTR,0)==0);
    assert(device_ioctl_tty(DEV_CONSOLE,OS_TTY_GET_INTR,0)==0);
    assert(device_ioctl_tty(DEV_CONSOLE,OS_TTY_SET_ECHO,0)==0);
    assert(tty_echo[0]==0 && tty_mode_owner[0]==12);
    assert(device_ioctl_tty(DEV_CONSOLE,OS_TTY_SET_CANON,3)==OS_EINVAL);
    assert(device_ioctl_tty(DEV_GPIO_VALUE_BASE,OS_TTY_GET_CANON,0)==OS_EINVAL);
    assert(device_ioctl_tty(DEV_UART1,OS_TTY_CLAIM,0)==0);
    assert(tty_claim_pid[1]==12 && resets==1);
    assert(device_ioctl_tty(DEV_UART1,OS_TTY_RELEASE,0)==0);
    assert(tty_claim_pid[1]==0 && resets==2);
    assert(device_ioctl_tty(DEV_UART1,OS_TTY_RELEASE,0)==OS_EIO);
#if PICONIX_WIFI
    foreground_pid[2]=12;
    assert(device_ioctl_tty(DEV_WIFI_TTY,OS_TTY_GET_ONLCR,0)==1);
#else
    assert(device_ioctl_tty(DEV_WIFI_TTY,OS_TTY_GET_ONLCR,0)==OS_EINVAL);
#endif
    foreground_pid[3]=12;
    assert(device_ioctl_tty(DEV_PTY_SLAVE,OS_TTY_GET_CANON,0)==1);
    assert(device_ioctl_tty(DEV_PTY_SLAVE,OS_TTY_SET_CANON,0)==0);
    assert(device_ioctl_tty(DEV_PTY_SLAVE,OS_TTY_GET_CANON,0)==0);
    foreground_pid[CDC_VCONSOLE_TTY]=12;
    assert(device_ioctl_tty(DEV_CDC_VCONSOLE,OS_TTY_GET_CANON,0)==1);
    assert(device_ioctl_tty(DEV_CDC_VCONSOLE,OS_TTY_SET_CANON,0)==0);
    assert(tty_canonical[CDC_VCONSOLE_TTY]==0);
    fn=GPIO_FUNC_SIO;
    assert(device_ioctl_gpio_status(4,OS_GPIO_STATUS_BASE+17)==
        (GPIO_STATUS_OUT | (1<<GPIO_STATUS_LEVEL_SHIFT) | (1<<GPIO_STATUS_PULL_SHIFT)));
    fn=GPIO_FUNC_I2C;
    assert(device_ioctl_gpio_status(4,OS_GPIO_STATUS_BASE+26)==GPIO_STATUS_I2C);
    fb=1;
    assert(device_ioctl_gpio_status(4,OS_GPIO_STATUS_BASE+5)==GPIO_STATUS_FB);
    assert(device_ioctl_gpio_status(3,OS_GPIO_STATUS_BASE+5)==OS_EBADF);
    puts("PASS: real TTY/GPIO ioctl helpers");
    return 0;
}
'''
with tempfile.TemporaryDirectory() as directory:
    f = Path(directory)/'test.c'
    f.write_text(boilerplate+helpers+checks)
    for wifi in (0,1):
        exe = Path(directory)/f'test_{wifi}'
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',f'-DPICONIX_WIFI={wifi}',
                        '-I',str(root/'include'),'-o',str(exe),str(f)],check=True)
        subprocess.run([str(exe)],check=True)
