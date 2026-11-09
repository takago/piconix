#!/usr/bin/env python3
"""Static checks for the standard PTY console-to-child relay."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
s = (root / 'userland/apps/pty_test.c').read_text()
assert 'argc>1 ? argc-1 : 1' in s
assert 'program_id(child_args[0])' in s
assert 'spawn_foreground(program,child_argc,child_args)' in s
assert 'FD_SET(master,&rd)' in s and 'FD_SET(0,&rd)' in s
assert 'FD_SET(master,&wr)' in s
assert 'read(0,input' in s and 'write(master,input+in_pos' in s
assert 'read(master,output' in s and 'output_all(1,output,n)' in s
assert 'OS_TTY_SET_CANON' in s and 'OS_TTY_SET_ECHO' in s
assert 'F_SETFL,old_flags' in s and 'F_SETFL,old_flags|O_NONBLOCK' in s
assert 'waitpid(child,NULL,WNOHANG)' in s and 'kill(child)' in s
for lang in ('ja','en'):
    doc = (root / f'docs/{lang}/reference/devices/PTY.md').read_text()
    assert 'pty_test fd' in doc and 'pty_test vi /tmp/test.txt' in doc
assert 'pty_test fd' in (root/'manual/pty').read_text()
print('PASS: PTY interactive relay source, docs and manual are consistent')
