#!/usr/bin/env python3
"""Check cp embedding, lookup, help and RAM-file safeguards."""
from pathlib import Path
r=Path(__file__).resolve().parents[2]
main=(r/'kernel/main.c').read_text()
cmake=(r/'CMakeLists.txt').read_text()
helptext=(r/'userland/apps/help.c').read_text()
cp=(r/'userland/apps/cp.c').read_text()
assert 'register_image(48,"cp",' in main
assert 'piconix_cp_image_start' in main
assert 'ps cat cp grep' in cmake
assert 'usage: cp SRC DST' in helptext and '  cp         Copy' in helptext
assert 'O_CREAT|O_TRUNC' in cp and '!strcmp(src,dst)' in cp
assert 'close(in)' in cp and 'close(out)' in cp
assert '`cp`' in (r/'README.md').read_text()
print('PASS: cp registration, docs and guards')
