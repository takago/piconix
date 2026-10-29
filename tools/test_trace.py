#!/usr/bin/env python3
"""ABI and integration sanity checks for the 32-KiB binary trace device."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as d:
    f = Path(d) / 'trace_size.c'
    f.write_text('#include "trace.h"\n_Static_assert(sizeof(trace_event_t)==32,"trace ABI");\nint main(void){return 0;}\n')
    subprocess.run(['cc','-std=c11','-I',str(ROOT/'ram_apps'),'-fsyntax-only',str(f)], check=True)
source = (ROOT/'src/kernel.c').read_text()
assert 'static trace_event_t trace_ring[TRACE_CAPACITY];' in source
assert '#define TRACE_CAPACITY 1024u' in source
audit_source = (ROOT/'tools/audit_firmware.py').read_text()
assert "symbol_sizes.get('trace_ring')" in audit_source
assert 'Trace ring            16384 bytes' not in audit_source
assert 'trace_enabled=0;' in source
assert 'trace_enabled=1;' in source
assert '{"/dev/trace",DEV_TRACE}' in source
assert 'if(dev==DEV_TRACE)' in source
assert 'if(tasks[current].fd[fd]==DEV_TRACE)' in source
assert 'save_and_disable_interrupts()' in source
assert 'if(!trace_enabled ||' in source
assert 'trace_record(TRACE_SWITCH' in source
assert 'trace_record(TRACE_SVC_ENTER' in source
assert 'trace_record(TRACE_SVC_EXIT' in source
assert 'trace_serial++' in source
assert 'TRACE_STATUS' in source and 'TRACE_LOST' in source
trace_app=(ROOT/'ram_apps/trace.c').read_text()
assert 'trace on|off|show [PID]|follow [PID]' in trace_app
assert 'TICKx1K' in trace_app
assert 'console' in trace_app and 'uart1' in trace_app
assert 'print_event(&event)' in trace_app
assert 'register_image(7,' in (ROOT/'src/main.c').read_text()
# Trace is launched via the shared /bin lookup and bounded argv SVC.
assert 'run_external(s,cwd,requested_bg' in (ROOT/'ram_apps/shell.c').read_text()
assert 'spawn(id,argc,(const char *const *)args)' in (ROOT/'ram_apps/shell.c').read_text()
assert 'spawn_in_slot(slot,a,programs[a].entry,programs[a].name,argc,argv)' in source
assert 'test_app(tmp, \'trace\'' in (ROOT/'tools/test_ram_pipeline.py').read_text()
print('PASS: trace ABI and device integration')
