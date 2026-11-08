#!/usr/bin/env python3
"""Static checks for the agent diagnostics manual; hardware not exercised."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
manual=root/'manual'
names=['trace-events', 'trace-investigation', 'trace-select', 'memory-investigation', 'memory-symbols', 'memory-failures', 'devices-discovery', 'devices-pwm', 'devices-i2c', 'devices-gpio', 'devices-framebuffer', 'devices-adc', 'agent-diagnostic-cases']
index=(manual/'README').read_text()
for name in names:
    b=(manual/name).read_bytes()
    assert 100<len(b)<4096 and b'\x00' not in b,name
    assert name in index,name
trace=(manual/'trace-events').read_text()
assert 'Type  Function  Arg  Ret  Info' in trace
assert 'name/' in trace and 'SYSCALL' in trace
assert 'trace show' in (manual/'trace-investigation').read_text()
assert 'one-byte read' in (manual/'devices-i2c').read_text()
assert '24-KiB' in (manual/'memory-investigation').read_text()
# The current ROM format stores the name/offset/length index in manual.bin,
# not in the obsolete generated kernel/man_rom.h C array.
import subprocess
import tempfile
import struct
with tempfile.TemporaryDirectory() as tmp:
    image_path = Path(tmp) / 'manual.bin'
    subprocess.run(['python3', str(root/'tools/make_manual_rom.py'),
                    str(image_path)], check=True, capture_output=True)
    image = image_path.read_bytes()
assert image[:8] == b'PICONIXM'
count = struct.unpack_from('<I', image, 8)[0]
index = {}
for i in range(count):
    name, offset, length = struct.unpack_from('<32sII', image, 12 + i * 40)
    key = name.split(b'\0', 1)[0].decode('ascii')
    index[key] = (offset, length)
for name in names:
    assert name in index, name
    offset, length = index[name]
    assert image[offset:offset + length] == (manual/name).read_bytes(), name
assert not (root/'kernel/man_rom.h').exists()
print('PASS:',len(names),'diagnostics pages indexed in manual.bin')
