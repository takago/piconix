#!/usr/bin/env python3
"""ABI and integration sanity checks for the 16-KiB binary trace device."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as d:
    f = Path(d) / 'trace_size.c'
    f.write_text('#include "piconix_trace.h"\n_Static_assert(sizeof(trace_event_t)==32,"trace ABI");\nint main(void){return 0;}\n')
    subprocess.run(['cc','-std=c11','-I',str(ROOT/'include'),'-fsyntax-only',str(f)], check=True)
source = (ROOT/'kernel/kernel.c').read_text()
v = source
assert 'extern trace_event_t trace_ring[TRACE_CAPACITY];' in source
assert '.set trace_ring, 0x20038000' in source
assert '#define TRACE_CAPACITY 512u' in source
audit_source = (ROOT/'tools/audit_firmware.py').read_text()
assert "symbols.get('trace_ring') != 0x20038000" in audit_source
assert '0x20038000..0x2003bfff, reserved gap' in audit_source
assert 'trace_enabled=0;' in source
assert 'trace_enabled=1;' in source
assert 'PICONIX_FIXED_DEVICES(DEVICE_ENTRY)' in v
assert 'if(dev==DEV_TRACE)' in source
assert 'if(tasks[current].fd[fd]==DEV_TRACE)' in source
assert 'save_and_disable_interrupts()' in source
assert 'if(!trace_enabled ||' in source
assert 'trace_record(TRACE_SWITCH' in source
assert 'trace_record(TRACE_SVC_ENTER' in source
assert 'trace_record(TRACE_SVC_EXIT' in source
assert 'trace_serial++' in source
assert 'TRACE_STATUS' in source and 'TRACE_LOST' in source
trace_app=(ROOT/'userland/apps/trace.c').read_text()
assert 'trace on [-a]|off|show [PID]|follow [PID]' in trace_app
assert 'TICKx1K' in trace_app
assert 'is_tmp ? spawn_tmp(image) : spawn(' in trace_app
assert 'getcwd(cwd,sizeof cwd)' in trace_app
assert '#include "../../include/piconix_syscall.h"' in trace_app
assert 'PICONIX_SYSCALL_TABLE(PICONIX_TRACE_SYSCALL_CASE)' in trace_app
assert 'console' in trace_app and 'ttyS1' in trace_app and 'ttyN0' in trace_app
assert 'display_event(&event,filter_pid)' in trace_app
assert 'register_image(7,' in (ROOT/'kernel/main.c').read_text()
# Trace is launched via the shared /bin lookup and bounded argv SVC.
assert 'run_external(s,cwd,requested_bg' in (ROOT/'userland/apps/shell.c').read_text()
assert 'spawn(id,argc,(const char *const *)args)' in (ROOT/'userland/apps/shell.c').read_text()
assert 'spawn_in_slot(slot,a,programs[a].entry,programs[a].name,argc,argv,NULL,0)' in source
assert 'test_app(tmp, \'trace\'' in (ROOT/'tools/test_scripts/test_ram_pipeline.py').read_text()
print('PASS: trace ABI and device integration')
