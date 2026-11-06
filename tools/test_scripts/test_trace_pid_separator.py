#!/usr/bin/env python3
"""Exercise PID separators on displayed rows; SWITCH uses destination PID."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[2]
source=(root/'userland/apps/trace.c').read_text()
start=source.index('static void print_pid_separator(')
end=source.index('/* 表示は Function',start)
function=source[start:end]
assert 'print_pid_separator(shown_pid);' in source
assert 'unsigned shown_pid=kind==TRACE_SWITCH ? e->value : e->pid;' in source
assert 'display_previous_valid=0;' in source
assert 'printf("\\033[4m")' not in source
assert 'printf("%u -> %u\\n",e->pid,e->value)' in source

harness='''
#include <assert.h>
#include <string.h>
static unsigned display_previous_pid;
static int display_previous_valid;
static const char *printed[16];
static int printed_count;
#define TRACE_SEPARATOR "--------"
static int record(const char *s) {
    printed[printed_count++]=s;
    return 0;
}
#define puts(s) record(s)
'''+function+'''
static void reset(void) {
    display_previous_pid=0;
    display_previous_valid=0;
    printed_count=0;
}
int main(void) {
    reset();
    print_pid_separator(0); // first shown event: no separator
    print_pid_separator(0);
    assert(printed_count==0);
    print_pid_separator(12); // PID 0 -> 12
    assert(printed_count==1);
    print_pid_separator(12); // same PID, no separator
    assert(printed_count==1);
    print_pid_separator(0); // PID 12 -> 0
    assert(printed_count==2);
    reset();
    print_pid_separator(12); // first event after reset: no separator
    assert(printed_count==0);
    print_pid_separator(9);
    assert(printed_count==1 && !strcmp(printed[0],TRACE_SEPARATOR));
    return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    src=Path(d)/'test.c'
    exe=Path(d)/'test'
    src.write_text(harness)
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: ASCII PID separators on displayed PID changes, no first separator, reset, SWITCH target PID')
