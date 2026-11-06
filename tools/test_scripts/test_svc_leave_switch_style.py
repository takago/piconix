#!/usr/bin/env python3
"""Check SVC boundary removal and ASCII separators for displayed PID changes."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
asm = (root / 'kernel/context.S').read_text()
kernel = (root / 'kernel/kernel.c').read_text()
trace = (root / 'userland/apps/trace.c').read_text()
abi = (root / 'include/piconix_trace.h').read_text()
section = asm.split('isr_svcall:', 1)[1].split('.size isr_svcall', 1)[0]
assert 'bl os_svc_dispatch' in section
assert 'bl os_trace_svc_leave' not in section
assert 'os_trace_svc_leave' not in kernel
assert 'TRACE_SVC_LEAVE' not in kernel + trace + abi
assert 'printf("\\033[4m")' not in trace
assert 'printf("%u -> %u\\n",e->pid,e->value)' in trace
assert 'print_pid_separator(shown_pid);' in trace
assert 'display_previous_pid!=pid' in trace
assert 'display_previous_valid=0;' in trace
assert 'puts(TRACE_SEPARATOR);' in trace
# The old syscall depth reset broke nesting inside os_svc_dispatch().
assert 'unsigned syscall_depth=*depth;' in trace
assert 'function_label(label,syscall_name(call),enter ? syscall_depth : *depth,enter);' in trace
assert 'if(!enter) *depth=0;' not in trace
for key, number in [('TRACE_SWITCH', 1), ('TRACE_SVC_ENTER', 2),
                    ('TRACE_SVC_EXIT', 3), ('TRACE_TICK1K', 4),
                    ('TRACE_FUNC_ENTER', 5), ('TRACE_FUNC_EXIT', 6)]:
    assert f'#define {key} {number}' in abi
print('PASS: no SVC boundary events; PID change separator; nested syscall display; stable ABI')
