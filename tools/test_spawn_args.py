#!/usr/bin/env python3
"""Ensure the SVC spawn path forwards a single string argument for every app."""
from pathlib import Path
source = (Path(__file__).resolve().parents[1] / 'src/kernel.c').read_text()
spawn = source.split('case SYS_SPAWN:', 1)[1].split('case SYS_CONSOLE_READY:', 1)[0]
assert 'else if(c) {' in spawn
assert '(a==4 || a==6 || a==7)' not in spawn
assert 'in_task_range(path+i,1)' in spawn
assert 'spawn_in_slot(slot,a,programs[a].entry,programs[a].name,0,path)' in spawn
assert 'child->script_path[j]=path[j]' in spawn
assert 'a!=3 && b!=0' in spawn  # preserve legacy Omikuji numeric argument
assert 'spawn_in_slot(slot,a,programs[a].entry,programs[a].name,(int)b,NULL)' in spawn
print('PASS: generic spawn string argument forwarding')
