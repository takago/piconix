#!/usr/bin/env python3
"""Regression: SYS_OPEN ENTER carries the checked path, and the viewer uses it."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
kernel = (root / 'kernel/kernel.c').read_text()
viewer = (root / 'userland/apps/trace.c').read_text()
start = kernel.index('/* openの入口でも実パスを記録する．')
end = kernel.index('    switch(num) {', start)
enter = kernel[start:end]
assert 'trace_enabled && num==SYS_OPEN' in enter
assert 'in_task_range(path+enter_preview_len,1)' in enter
assert 'enter_preview, enter_preview_len);' in enter
assert 'trace_record(TRACE_SVC_EXIT' in kernel
assert 'if(call==SYS_OPEN && (int)e->detail>=0)' in viewer
assert 'save_fd(e->pid,e->detail,e->data);' in viewer
assert 'memcpy(arg,e->data,sizeof e->data);' in viewer
assert 'arg[sizeof e->data]=0;' in viewer
assert 'pending_save(e->pid,call,arg);' in viewer
assert 'pending_find(e->pid,call)' in viewer

# Compile and execute the exact entry-side selection/copy code with a valid 24-KiB
# user slot, an invalid pointer, and a path longer than the ABI's 16 data bytes.
code = enter[:enter.index('    switch(num) {')] if '    switch(num) {' in enter else enter
harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define SYS_OPEN 5
#define TRACE_SVC_ENTER 2
static unsigned char slot[24576];
static int trace_enabled=1;
static struct { int pid, program_id; } tasks[1]={{3,4}};
static int in_task_range(const void *p, size_t n) {
    uintptr_t a=(uintptr_t)p, lo=(uintptr_t)slot;
    return a>=lo && a<=lo+sizeof slot && n<=lo+sizeof slot-a;
}
static unsigned char recorded[16];
static unsigned recorded_len, records;
static void trace_record(int kind, unsigned pid, unsigned num, unsigned detail,
                         int program_id, int file_id, const void *p, unsigned len) {
    (void)kind;(void)pid;(void)num;(void)detail;(void)program_id;(void)file_id;
    memset(recorded,0,sizeof recorded);
    if(p) memcpy(recorded,p,len);
    recorded_len=len;
    records++;
}
static void entry(int num, uintptr_t a) {
    int current=0, file_id=-1;
''' + enter + r'''
}
int main(void) {
    strcpy((char *)slot,"/tmp/example.txt.more");
    entry(SYS_OPEN,(uintptr_t)slot);
    assert(recorded_len==16 && !memcmp(recorded,"/tmp/example.txt",16));
    entry(SYS_OPEN,(uintptr_t)(slot+sizeof slot+20));
    assert(recorded_len==0);
    trace_enabled=0;
    entry(SYS_OPEN,(uintptr_t)slot);
    assert(recorded_len==0);
    trace_enabled=1;
    slot[0]=0;
    entry(SYS_OPEN,(uintptr_t)slot);
    assert(recorded_len==0 && records==4);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    src=Path(d)/'test.c'; exe=Path(d)/'test'
    src.write_text(harness)
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: open ENTER validates and records path; viewer uses data without changing EXIT')
