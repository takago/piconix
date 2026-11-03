#!/usr/bin/env python3
"""Execute the real trace writer with a simulated IRQ mask and ring buffer."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[2]
source=(root/'kernel/kernel.c').read_text()
start=source.index('static void trace_record(')
open_brace=source.index('{',start)
level=0
for pos in range(open_brace,len(source)):
    if source[pos]=='{':level+=1
    elif source[pos]=='}':
        level-=1
        if not level:break
writer=source[start:pos+1]
assert 'save_and_disable_interrupts()' in writer
assert 'restore_interrupts(irq_state)' in writer
c_code=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "piconix_trace.h"
#define OS_MAX_TASKS 6
#define TRACE_CAPACITY 512u
#define UNUSED 0
#define READY 1
struct task { int pid; int state; uint8_t program_id,tty; };
static struct task tasks[OS_MAX_TASKS];
static trace_event_t trace_ring[TRACE_CAPACITY];
static uint32_t trace_head,trace_count,trace_serial,ticks;
static uint8_t trace_enabled;
static int irq_mask, save_count, restore_count;
static uint32_t save_and_disable_interrupts(void) {
    uint32_t was_masked=(uint32_t)irq_mask;
    irq_mask=1;save_count++;return was_masked;
}
static void restore_interrupts(uint32_t old) {
    assert(irq_mask==1);irq_mask=(int)old;restore_count++;
}
'''
main=r'''
int main(void) {
    tasks[0].pid=33;tasks[0].tty=2;tasks[0].state=READY;
    trace_enabled=1;ticks=42;
    const uint8_t bytes[]={1,2,3,4};
    trace_record(TRACE_SVC_EXIT,33,7,8,9,-1,bytes,4);
    assert(irq_mask==0 && save_count==1 && restore_count==1);
    assert(trace_count==1 && trace_serial==1 && trace_head==1);
    assert(trace_ring[0].tick==42 && trace_ring[0].value==7);
    assert(TRACE_TTY(trace_ring[0].kind)==2 && trace_ring[0].pid==33);
    assert(memcmp(trace_ring[0].data,bytes,4)==0);
    /* Already running inside an interrupt: retain original mask. */
    irq_mask=1;
    trace_record(TRACE_TICK1K,33,1,2,9,-1,NULL,0);
    assert(irq_mask==1 && save_count==2 && restore_count==2);
    irq_mask=0;
    /* Non-recorded trace viewer must not alter the ring. */
    trace_record(TRACE_SVC_ENTER,33,0,0,7,-1,NULL,0);
    assert(trace_serial==2 && save_count==2);
    for(unsigned i=0;i<TRACE_CAPACITY+3;i++)
        trace_record(TRACE_SVC_ENTER,33,(unsigned)i,0,9,-1,NULL,0);
    assert(trace_count==TRACE_CAPACITY && trace_head==5);
    assert(trace_serial==TRACE_CAPACITY+5);
    assert(save_count==restore_count && irq_mask==0);
    puts("PASS: trace writer IRQ serialization, record ABI, masking, wraparound");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='piconix_trace_writer_') as tmp:
    f=Path(tmp)/'trace_writer.c'
    exe=Path(tmp)/'trace_writer'
    f.write_text(c_code+'\n'+writer+'\n'+main)
    subprocess.run(['cc','-std=c11','-DPICONIX_HOST_TEST','-Wall','-Wextra','-Werror', '-I',str(root/'include'),str(f),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
