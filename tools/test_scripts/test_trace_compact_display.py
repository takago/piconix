#!/usr/bin/env python3
"""Host regression: adjacent kernel function entry/exit is compacted only when safe."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'userland/apps/trace.c').read_text()
assert source.count('count+=display_event(') == 2  # show/follow and run
assert source.count('display_flush(') >= 3
start = source.index('static int compact_pair(')
end = source.index('/* トレースデバイスから固定長レコード', start)
code = source[start:end]
# Exercise actual matching and look-ahead logic with stand-in display functions.
harness = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#define TRACE_KIND_MASK 0x3fu
#define TRACE_SWITCH 1
#define TRACE_FUNC_ENTER 5
#define TRACE_FUNC_EXIT 6
typedef struct {
    uint32_t tick, value, detail;
    uint8_t kind, pid, program_id;
    int8_t file_id;
    uint8_t data[16];
} trace_event_t;
static trace_event_t display_pending;
static int display_has_pending;
static int singles, compact;
static void print_event(const trace_event_t *e, int merged) { (void)e; if(merged) compact++; else singles++; }
''' + code + r'''
static trace_event_t event(int kind, unsigned ms, unsigned pid, unsigned addr) {
    trace_event_t e={0};
    e.kind=kind; e.tick=ms; e.pid=pid; e.program_id=12; e.value=addr;
    memcpy(e.data,"in_task_range",13);
    return e;
}
static void reset(void) { singles=compact=display_has_pending=0; }
int main(void) {
    trace_event_t a=event(5,616025,24,0x2000), b=event(6,616025,24,0x2000);
    reset();
    assert(display_event(&a,-1)==0);
    assert(display_event(&b,-1)==1);
    assert(display_flush(-1)==0 && singles==0 && compact==1);
    reset(); b.tick++;
    assert(display_event(&a,-1)==0);
    assert(display_event(&b,-1)==2);
    assert(singles==2 && compact==0);
    reset(); b=a; b.kind=6; b.value++;
    display_event(&a,-1); display_event(&b,-1);
    assert(singles==2 && compact==0);
    reset(); b=a; b.kind=6; b.pid++;
    display_event(&a,-1); display_event(&b,-1);
    assert(singles==2 && compact==0);
    reset(); b=a; b.kind=6;
    trace_event_t x=event(1,616025,24,0);
    display_event(&a,-1);display_event(&x,-1);display_event(&b,-1);
    assert(singles==3 && compact==0);
    reset();
    display_event(&a,3);display_event(&b,3);
    assert(singles==0 && compact==0);
    reset();
    display_event(&a,24);
    assert(display_flush(24)==1 && singles==1);
    reset();
    a.pid=0;b.pid=0;
    display_event(&a,-1);display_event(&b,-1);
    assert(compact==1 && singles==0);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    f = Path(tmp)/'test.c'
    exe = Path(tmp)/'test'
    f.write_text(harness)
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(f),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: compact trace display, PID filters, timestamps, intervening events, EOF')
