#!/usr/bin/env python3
"""Source-level contract for unified optional function tracing."""
from pathlib import Path
import re
p=Path(__file__).resolve().parents[2]
cm=(p/'CMakeLists.txt').read_text()
k=(p/'kernel/kernel.c').read_text()
t=(p/'userland/apps/trace.c').read_text()
a=(p/'include/piconix_trace.h').read_text()
assert 'PICONIX_KERNEL_FUNCTION_TRACE' in cm and '-finstrument-functions' in cm
for name in ('TRACE_CONTEXT_SET','TRACE_CONTEXT_GET','TRACE_FUNC_ENTER','TRACE_FUNC_EXIT'):
    assert name in a and name in k+t
assert 'TRACE_FUNC_ON' not in a and 'trace kernel' not in t
assert 'trace_mode==TRACE_MODE_SYSCALL' in k and 'trace_mode!=TRACE_MODE_ALL && ipsr!=11u' in k
assert 'int svc=ipsr==11u && current>=0' in k
assert 'trace_record(kind,t?(uint8_t)t->pid:0' in k
assert re.search(r'if\s*\(!trace_enabled\)\s*return\s*;', k)
assert 'trace_mode=(uint8_t)arg;' in k
assert 'save_and_disable_interrupts()' in k
assert 'trace_run(fd,argc,argv)' in t
assert 'TRACE_CONTEXT_SET,mode' in t and 'TRACE_CONTEXT_SET,previous_mode' in t
assert 'on?TRACE_ON:TRACE_OFF,on_mode' in t
assert 'if(!shown_pid && (kind==TRACE_FUNC_ENTER || kind==TRACE_FUNC_EXIT))' in t
assert 'function_label(label,func,func_depth,func_enter)' in t
assert 'trace_depth(e->pid)' in t
for lang in ('ja','en'):
    d=(p/f'docs/{lang}/reference/commands/TRACE.md').read_text()
    assert 'trace on -a' in d and 'trace run -a' in d
    # Reference documentation describes only the currently available command syntax.
    assert 'trace kernel' not in d
print('PASS: unified trace source and bilingual guide contract')
