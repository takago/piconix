#!/usr/bin/env python3
"""Ensure ls inspects file operands without opening device nodes."""
from pathlib import Path
src=(Path(__file__).resolve().parents[2]/'userland/apps/ls.c').read_text()
probe=src.index('is_file=file_entry(path);')
open_device=src.index('int fd=open(path,0);',src.index('static void ls('))
assert probe < open_device, 'file metadata must be checked before opening a device'
assert 'if(e.is_dir) result=0;' in src
assert 'else { entry(parent,e.name,0,e.device_id); result=1; }' in src
assert 'if(is_file==1)return;' in src
print('PASS: ls file operands use parent getdents without device open')
