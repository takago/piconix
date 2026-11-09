#!/usr/bin/env python3
"""Compile the real PicoNIX PID allocator and test reuse and wraparound."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'kernel/kernel.c').read_text()
start = source.index('NO_TRACE static int allocate_pid(void) {')
brace = source.index('{', start)
depth = 0
end = None
for i in range(brace, len(source)):
    if source[i] == '{':
        depth += 1
    elif source[i] == '}':
        depth -= 1
        if depth == 0:
            end = i + 1
            break
assert end is not None
allocator = source[start:end]
assert 'int pid=allocate_pid();' in source
assert 't->pid=pid;' in source
assert 't->pid=next_pid++' not in source

program = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "piconix_trace.h"
#define NO_TRACE
#define OS_MAX_TASKS 6
#define UNUSED 0
#define READY 1
#define DEAD 3
#define OS_EAGAIN (-11)
typedef struct { int pid, state; } task_t;
static task_t tasks[OS_MAX_TASKS];
static struct { int pid, parent_pid; } exited[OS_MAX_TASKS];
static int next_pid=1;
static int task_active(const task_t *task) {
    return task->state!=UNUSED && task->state!=DEAD;
}
_Static_assert(sizeof(trace_event_t)==32,"trace record size changed");
_Static_assert(sizeof(((trace_event_t *)0)->pid)==1,"trace PID is not uint8_t");
_Static_assert(sizeof(((trace_event_t *)0)->data)==16,"trace preview changed");
'''
program += allocator
program += r'''
int main(void) {
    /* A long-running sequence cycles after 255 without ever returning 0. */
    for(int i=1;i<=1024;i++) {
        int expected=((i-1)%255)+1;
        assert(allocate_pid()==expected);
    }
    /* Wraparound must skip live PIDs and unreaped exit notices. */
    memset(tasks,0,sizeof tasks);
    memset(exited,0,sizeof exited);
    next_pid=255;
    tasks[0].pid=255; tasks[0].state=READY;
    tasks[1].pid=1; tasks[1].state=READY;
    exited[0].pid=2; exited[0].parent_pid=1;
    assert(allocate_pid()==3);
    assert(next_pid==4);
    /* Once reaped, a PID can be reused. */
    exited[0].pid=0;
    next_pid=2;
    assert(allocate_pid()==2);
    /* A dead task's old PID is no longer reserved. */
    tasks[0].state=DEAD;
    next_pid=255;
    assert(allocate_pid()==255);
    puts("PASS: PID cycles through 1..255, skips live tasks and unreaped exits; trace ABI remains 32 bytes");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='piconix-pid-wrap-') as tmp:
    cpp = Path(tmp) / 'pid.c'
    binary = Path(tmp) / 'pid_test'
    cpp.write_text(program)
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',
                    '-I',str(root / 'include'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
