#!/usr/bin/env python3
"""Ensure the raw framebuffer has exactly one public name: /dev/fb."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (root / 'include/device_names.h').read_text()
remote = (root / 'kernel/net_service_remote_devctl.c').read_text()
client = (root / 'host/piconixctl.py').read_text()

assert 'X("/dev/fb", DEV_FB)' in header
assert 'X("/dev/fbchar", DEV_FBCHAR)' in header
assert 'X("/dev/gpio/fb", DEV_FB)' not in header
assert 'if(!strcmp(path,"/dev/fb"))' in remote
assert 'if(!strncmp(path,"/dev/gpio/",10))' in remote
assert 'args.path == "/dev/fb"' in client

app_paths = (
    'userland/apps/fbctl.c', 'userland/apps/fbed.c',
    'userland/apps/scroll.c', 'userland/apps/kitesys.c',
    'userland/apps/httpd.c', 'host/demo_udp.py',
    'userland/examples/python/fbwave.py',
    'userland/fs/tmp_common/fbwave.py',
)
for relative in app_paths:
    text = (root / relative).read_text()
    assert '/dev/fb' in text, relative
    assert '/dev/gpio/fb' not in text, relative

for relative in (
    'manual/devices-framebuffer', 'manual/devices',
    'manual/led-patterns', 'manual/gpio-workflow',
    'docs/en/reference/devices/DEVICES.md',
    'docs/ja/reference/devices/DEVICES.md',
    'docs/en/reference/network/REMOTE-DEVICE.md',
    'docs/ja/reference/network/REMOTE-DEVICE.md',
):
    text = (root / relative).read_text()
    assert '/dev/fb' in text, relative
    if '/dev/gpio/fb' in text:
        assert relative.endswith('reference/devices/DEVICES.md'), relative

for path in root.rglob('*'):
    if not path.is_file() or path.suffix in ('.png', '.bin', '.zip', '.pyc'):
        continue
    relative = path.relative_to(root).as_posix()
    if relative.startswith('docs/DEVELOPMENT_DIARY') or relative.endswith('reference/devices/DEVICES.md') or relative == 'tools/test_scripts/test_framebuffer_path_migration.py':
        continue
    try:
        contents = path.read_text()
    except UnicodeDecodeError:
        continue
    assert '/dev/gpio/fb' not in contents, relative
print('PASS: framebuffer VFS, UDP, applications, samples, manuals and docs moved')
