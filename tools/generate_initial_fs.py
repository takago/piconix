#!/usr/bin/env python3
"""Generate Flash-resident initial RAM files from flat, board-scoped directories."""
import argparse
from pathlib import Path

CATEGORIES = ('etc', 'tmp')
BOARDS = ('common', 'pico', 'picow')
ALLOWED = {f'{category}_{board}' for category in CATEGORIES for board in BOARDS}

# 初期RAMファイルの入力データからビルド用のイメージを生成する．
def generate(base, board, output):
    if board not in ('pico', 'pico_w'):
        raise ValueError(f'unsupported board: {board}')
    target = 'picow' if board == 'pico_w' else 'pico'
    if not base.is_dir():
        raise ValueError(f'initial FS directory missing: {base}')
    for entry in sorted(base.iterdir()):
        if entry.name not in ALLOWED or not entry.is_dir():
            raise ValueError(f'unsupported initial FS entry: {entry.name}')
        for item in sorted(entry.iterdir()):
            if not item.is_file() or item.is_symlink():
                raise ValueError(f'only regular files are allowed: {item}')
    chosen = {}
    for suffix in ('common', target):
        for category in CATEGORIES:
            directory = base / f'{category}_{suffix}'
            if not directory.is_dir():
                continue
            for path in sorted(directory.iterdir()):
                key = f'{category}/{path.name}'
                chosen[key] = path.read_bytes()
    if len(chosen) > 11:
        raise ValueError(f'too many initial files: {len(chosen)} > 11')
    for name, data in chosen.items():
        try:
            encoded = name.encode('ascii')
        except UnicodeEncodeError:
            raise ValueError(f'VFS name must be ASCII: {name}')
        if len(encoded) > 31:
            raise ValueError(f'VFS name too long: {name}')
        if len(data) > 2048:
            raise ValueError(f'initial file too large: {name} ({len(data)})')
    files = sorted(chosen.items())
    lines = ['/* Generated file. Do not edit. */',
             'struct piconix_initial_file { const char *name; const unsigned char *data; unsigned size; };']
    for i, (name, data) in enumerate(files):
        arr = ', '.join(f'0x{b:02x}' for b in data) if data else '0x00'
        lines.append(f'static const unsigned char piconix_initial_data_{i}[] = {{{arr}}};')
    lines.append('static const struct piconix_initial_file piconix_initial_files[] = {')
    for i, (name, data) in enumerate(files):
        lines.append(f'    {{"{name}", piconix_initial_data_{i}, {len(data)}u}},')
    lines += ['};', f'static const unsigned piconix_initial_file_count = {len(files)}u;', '']
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n'.join(lines))
    return chosen

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('base', type=Path)
    parser.add_argument('board', choices=['pico', 'pico_w'])
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    try:
        generate(args.base, args.board, args.output)
    except ValueError as exc:
        parser.error(str(exc))
