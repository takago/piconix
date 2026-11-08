#!/usr/bin/env python3
"""Pack manual pages into one little-endian XIP image (index + contents).

Layout: 'PICONIXM' magic (8 B), entry count (u32), then count entries of
32-byte NUL-terminated path + u32 absolute image offset + u32 byte length.
Page content starts on a four-byte boundary. All paths are relative to /man/.
"""
from pathlib import Path
import argparse
import struct

MAGIC = b'PICONIXM'
NAME_BYTES = 32
ENTRY = struct.Struct('<32sII')


def pack(root: Path) -> bytes:
    pages = sorted(p for p in root.rglob('*') if p.is_file())
    if not pages:
        raise ValueError('manual directory is empty')
    if len(pages) > 255:
        raise ValueError('too many manual pages (device id capacity: 255)')
    header_size = len(MAGIC) + 4 + ENTRY.size * len(pages)
    image = bytearray(MAGIC + struct.pack('<I', len(pages)))
    content = bytearray()
    for path in pages:
        name = path.relative_to(root).as_posix().encode('ascii')
        data = path.read_bytes()
        if not name or len(name) >= NAME_BYTES or not data or b'\0' in data:
            raise ValueError(f'invalid manual entry: {path}')
        if not all(32 <= char < 127 for char in name):
            raise ValueError(f'non-printable manual path: {path}')
        offset = header_size + len(content)
        image.extend(ENTRY.pack(name, offset, len(data)))
        content.extend(data)
        content.extend(b'\0' * ((-len(content)) % 4))
    image.extend(content)
    return bytes(image)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path, help='build-directory manual.bin')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1] / 'manual'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    result = pack(root)
    if not args.output.exists() or args.output.read_bytes() != result:
        args.output.write_bytes(result)
    print(f'PicoNIX manual: {struct.unpack_from("<I", result, 8)[0]} pages, {len(result)} ROM bytes')


if __name__ == '__main__':
    main()
