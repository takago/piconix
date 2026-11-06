#!/usr/bin/env python3
"""Trace modes and default CMake instrumentation contract."""
from pathlib import Path
import re
r=Path(__file__).resolve().parents[2]
h=(r/'include/piconix_trace.h').read_text()
k=(r/'kernel/kernel.c').read_text()
t=(r/'userland/apps/trace.c').read_text()
cm=(r/'CMakeLists.txt').read_text()
assert '"Instrument selected kernel functions for trace" ON' in cm
assert '#define TRACE_MODE_SVC 0' in h
assert '#define TRACE_MODE_ALL 1' in h
assert '#define TRACE_MODE_SYSCALL 2' in h
assert re.search(r'if\s*\(trace_mode==TRACE_MODE_SYSCALL\s*&&\s*kind!=TRACE_SVC_ENTER\s*&&\s*kind!=TRACE_SVC_EXIT\)\s*return;', k)
assert 'if(trace_mode==TRACE_MODE_SYSCALL ||' in k
assert 'trace_mode=(uint8_t)arg;' in k
assert 'int mode=trace_mode;' in k
assert 'TRACE_CONTEXT_SET,mode' in t
assert 'TRACE_CONTEXT_SET,previous_mode' in t
assert 'TRACE_ON,mode' in t
assert 'on?TRACE_ON:TRACE_OFF,on_mode' in t
assert 'trace enabled (syscalls only)' in t
assert 'trace on [-s|-a]' in t and 'trace run [-s|-a]' in t
print('PASS: syscall-only recording, mode restoration and default instrumentation')
