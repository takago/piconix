#!/usr/bin/env python3
"""Prevent shell option-as-path and missing embedded-command regressions."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
shell = (root / 'userland/apps/shell.c').read_text()
main = (root / 'kernel/main.c').read_text()
cmake = (root / 'CMakeLists.txt').read_text()

# Neither foreground nor pipeline execution interprets command options.
assert 'int path_arg=' not in shell
assert 'args[path_arg]=path;' not in shell
assert 'args[argc++]="31"' not in shell
# コメントの言語に依存せず，通常起動とパイプ起動の両方でargvを渡すことを検査する．
assert 'spawn_foreground(id,argc,(const char *const *)args)' in shell
assert 'return spawn(id,argc,(const char *const *)args);' in shell
assert 'static int resolve(' in shell  # still needed for redirection and cd
assert 'absolute_path(' in (root / 'kernel/kernel.c').read_text()

# CMake embedding alone does not expose a program as a command.
for name, ident in [('ping', 45), ('tcpdump', 46)]:
    assert re.search(r'\b'+name+r'\b', cmake)
    assert f'piconix_{name}_image_start' in main
    assert f'register_image({ident},"{name}",' in main
print('PASS: command-owned option parsing and network command registration')
