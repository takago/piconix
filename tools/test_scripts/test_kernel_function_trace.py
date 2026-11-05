#!/usr/bin/env python3
"""Check the opt-in kernel function tracing ABI and build wiring."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
cmake = (root / 'CMakeLists.txt').read_text()
kernel = (root / 'kernel/kernel.c').read_text()
trace = (root / 'userland/apps/trace.c').read_text()
abi = (root / 'include/piconix_trace.h').read_text()
for key in ('PICONIX_KERNEL_FUNCTION_TRACE', '-finstrument-functions'):
    assert key in cmake, key
assert 'PICONIX_KERNEL_FUNCTION_TRACE "Instrument selected kernel calls for trace kernel" OFF' in cmake
for key in ('TRACE_FUNC_ON', 'TRACE_FUNC_OFF', 'TRACE_FUNC_ENTER', 'TRACE_FUNC_EXIT'):
    assert key in abi and key in kernel and key in trace, key
assert 'no_instrument_function)) trace_record' in kernel
assert 'no_instrument_function))\ntrace_kernel_func' in kernel
assert 'ipsr!=11u' in kernel
# Expanded coverage is pinned to the real kernel function definitions.
for fn in ('vfs_fixed_path_id','vfs_directory_id','free_fd','fd_set',
           'in_task_range','tmp_init','tmp_name','tmp_lookup',
           'device_rw_rand','rand32','tty_rx_ready','tty_dequeue',
           'proc_device','device_rw_proc','device_ioctl_tty'):
    assert f'TRACE_MATCH({fn});' in kernel, fn
    assert fn + '(' in kernel, fn
    for lang in ('ja','en'):
        assert f'`{fn}`' in (root / f'docs/{lang}/guides/TRACE.md').read_text(), (lang,fn)
assert 'if(!name) return;' in kernel
assert 'trace_run(fd,argc,argv,!strcmp(argv[1],"kernel"))' in trace
for lang in ('ja','en'):
    doc = (root / f'docs/{lang}/guides/TRACE.md').read_text()
    assert 'trace kernel cat /tmp/test.txt' in doc
    assert 'PICONIX_KERNEL_FUNCTION_TRACE=ON' in doc
    for event in ('read', 'read/', 'vfs_rw', 'vfs_rw/'):
        assert f'`{event}`' in doc, (lang,event)
assert 'static unsigned *trace_depth(unsigned pid)' in trace
assert 'function_label(label,func,func_depth,func_enter)' in trace
assert "if(!entering) out[pos++]='/';" in trace
assert "out[pos++]=entering ? '+' : '-'" not in trace
assert "out[pos++]='<'" not in trace
assert 'col("KERNEL",9);col(label,34);col("--",16);col("--",3);puts("--")' in trace
assert 'function_label(label,syscall_name(call),0,enter)' in trace
assert 'memset(trace_depths,0,sizeof trace_depths)' in trace
for lang in ('ja','en'):
    doc = (root / f'docs/{lang}/guides/TRACE.md').read_text()
    assert ('字下げ' if lang=='ja' else 'indenting nested kernel functions') in doc
print('PASS: optional kernel function tracing ABI, guards, CLI and bilingual documentation')

assert 'Type     Function                           Arg              Ret Info' in trace
assert 'col("SWITCH",9);col("SWITCH",34)' in trace
assert 'if(enter) *depth=1;' in trace
assert 'if(!enter) *depth=0;' in trace
assert 'col("SYSCALL",9)' in trace
assert 'col("KERNEL",9)' in trace
assert 'col("NA"' not in trace

for event_type in ('SYSCALL', 'KERNEL', 'SWITCH', 'TIMER', 'OTHER'):
    assert f'col("{event_type}",9)' in trace, event_type
for lang in ('ja', 'en'):
    doc = (root / f'docs/{lang}/guides/TRACE.md').read_text()
    for label in ('SYSCALL', 'KERNEL', 'SWITCH', 'TIMER', 'OTHER'):
        assert f'`{label}`' in doc, (lang,label)

# Display depth filtering never affects event recording or stack tracking.
assert 'trace_max_depth=255' in trace
assert 'if(func_depth>trace_max_depth) return;' in trace
assert 'if(func_enter && *depth<255) ++*depth;' in trace
assert 'parse_depth(argv[3],&trace_max_depth)' in trace
assert 'command_index=4;' in trace
for lang in ('ja','en'):
    doc=(root / f'docs/{lang}/guides/TRACE.md').read_text()
    for cmd in ('trace kernel -d 0 hello','trace kernel -d 1 hello','trace kernel -d 2 hello'):
        assert cmd in doc

# A hidden kernel event must be skipped before any common row columns are printed.
start = trace.index('static void print_event(')
end = trace.index('static int trace_drain(', start)
printer = trace[start:end]
assert printer.index('if(func_depth>trace_max_depth) return;') < printer.index('col(time,7);')
assert printer.count('if(func_depth>trace_max_depth) return;') == 1
assert 'if(!func_enter && *depth) --*depth;' in printer

# Consecutive ioctls by one process must never reuse the previous FD target.
assert 'static void pending_forget(unsigned pid, unsigned call)' in trace
assert 'if(!enter) pending_forget(e->pid,call);' in trace
start = trace.index('static void pending_save(')
end = trace.index('static void reset_view_state(', start)
assert 'strcpy(pending[i].target,target);' in trace[start:end]
assert 'if(pending[i].pid==pid && pending[i].call==call)' in trace[start:end]
print('PASS: syscall target history is refreshed and cleared after return')
