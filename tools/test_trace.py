#!/usr/bin/env python3
"""ABI and integration sanity checks for the 16-KiB binary trace device."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as d:
    f = Path(d) / 'trace_size.c'
    f.write_text('#include "trace.h"\n_Static_assert(sizeof(trace_event_t)==16,"trace ABI");\nint main(void){return 0;}\n')
    subprocess.run(['cc','-std=c11','-I',str(ROOT/'ram_apps'),'-fsyntax-only',str(f)], check=True)
source = (ROOT/'src/kernel.c').read_text()
assert 'static trace_event_t trace_ring[TRACE_CAPACITY];' in source
assert '#define TRACE_CAPACITY 1024u' in source
assert 'trace_enabled=0;' in source
assert 'trace_enabled=1;' in source
assert '{"/dev/trace",DEV_TRACE}' in source
assert 'if(dev==DEV_TRACE)' in source
assert 'if(tasks[current].fd[fd]==DEV_TRACE)' in source
assert 'save_and_disable_interrupts()' in source
assert 'if(!trace_enabled) return;' in source
assert 'trace_record(TRACE_SWITCH' in source
assert 'trace_record(TRACE_SVC_ENTER' in source
assert 'trace_record(TRACE_SVC_EXIT' in source
assert 'register_image(7,' in (ROOT/'src/main.c').read_text()
# Shell passes trace's subcommand via spawn(7,...,arg); the kernel must forward it as argv[1].
assert 'spawn(7,0,arg)' in (ROOT/'ram_apps/shell.c').read_text()
assert 'else if(c) {' in source
assert '(a==4 || a==6 || a==7) && c' not in source
assert 'spawn_in_slot(slot,a,programs[a].entry,programs[a].name,0,path)' in source
assert 'test_app(tmp, \'trace\'' in (ROOT/'tools/test_ram_pipeline.py').read_text()
print('PASS: trace ABI and device integration')
