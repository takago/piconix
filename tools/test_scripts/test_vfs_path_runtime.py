#!/usr/bin/env python3
"""Compile the *real* GPIO parser and path normalizer on the host."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / "kernel/kernel.c").read_text()


def extract(signature):
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for i in range(brace, len(source)):
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                return source[start:i + 1]
    raise AssertionError(f"Missing function terminator: {signature}")


parser = extract("static int vfs_gpio_id(")
normalizer = extract("static int absolute_path(")
assert "dev=vfs_gpio_id(p+10);" in source, "open() must share GPIO parser"
assert "if(id<0 && dir==VDIR_GPIO) id=vfs_gpio_id(name);" in source
assert "dev>=DEV_GPIO_VALUE_BASE && dev<=DEV_GPIO_VALUE_END" in source
assert "return OS_EBADF;" in source[source.index("/* I2C uses ioctl()"):
                                          source.index("static int device_ioctl")]

code = r'''
#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "pins.h"
enum {
    DEV_GPIO_VALUE_BASE=1, DEV_GPIO_DIR_BASE=36, DEV_GPIO_PULL_BASE=65,
    DEV_GPIO_MAX_PIN=28
};
typedef struct { char cwd[128]; } test_task_t;
static test_task_t tasks[1];
static int current;
static const char *task_begin;
static size_t task_size;
static int in_task_range(const void *p, size_t n) {
    uintptr_t a=(uintptr_t)p, lo=(uintptr_t)task_begin;
    return a>=lo && a-lo<=task_size && n<=task_size-(a-lo);
}
''' + parser + '\n' + normalizer + r'''
static void check_path(const char *path, const char *cwd, const char *expected) {
    char output[128];
    strcpy(tasks[0].cwd,cwd);
    task_begin=path;
    task_size=strlen(path)+1;
    assert(absolute_path(path,output)==0);
    assert(strcmp(output,expected)==0);
}
int main(void) {
    assert(vfs_gpio_id("0")==1);
    assert(vfs_gpio_id("0.dir")==36);
    assert(vfs_gpio_id("15.pull")==80);
    assert(vfs_gpio_id("25")==26);
    assert(vfs_gpio_id("18")==-1); /* PWM */
    assert(vfs_gpio_id("20")==-1); /* UART1 */
    assert(vfs_gpio_id("26")==-1); /* I2C1 */
    assert(vfs_gpio_id("28")==-1); /* ADC */
    assert(vfs_gpio_id("00")==-1);
    assert(vfs_gpio_id("01")==-1);
    assert(vfs_gpio_id("29")==-1);
    assert(vfs_gpio_id("0foo")==-1);
    assert(vfs_gpio_id("0.dirextra")==-1);
    assert(vfs_gpio_id("9999999999999999999")==-1);
    assert(vfs_gpio_id("fb")==-1); /* static registry handles framebuffer */
    check_path("./1/../25.pull", "/dev/gpio", "/dev/gpio/25.pull");
    check_path("/tmp/./test//", "/", "/tmp/test");
    check_path("../../etc/hello", "/dev/gpio", "/etc/hello");
    check_path("/", "/tmp", "/");
    check_path("../..", "/tmp", "/");
    const long page_size=sysconf(_SC_PAGESIZE);
    assert(page_size>=4096);
    char *memory=mmap(NULL,(size_t)page_size*2,PROT_READ|PROT_WRITE,
                      MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(memory!=MAP_FAILED);
    assert(mprotect(memory+page_size,(size_t)page_size,PROT_NONE)==0);
    char *last=memory+page_size-1;
    *last='x'; /* no terminator in this task slot; next page is unreadable */
    task_begin=last;
    task_size=1;
    strcpy(tasks[0].cwd,"/");
    char output[128];
    assert(absolute_path(last,output)==-1);
    assert(munmap(memory,(size_t)page_size*2)==0);
    puts("PASS: runtime VFS GPIO parser and bounded path normalization");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="piconix-vfs-") as tmp:
    src = Path(tmp) / "vfs_path_harness.c"
    exe = Path(tmp) / "vfs_path_harness"
    src.write_text(code)
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O2",
                    "-I", str(root / "kernel"), str(src), "-o", str(exe)],
                   check=True)
    subprocess.run([str(exe)], check=True)
