from pathlib import Path
root = Path(__file__).resolve().parents[2]
k = (root / "kernel/kernel.c").read_text()
n = (root / "kernel/net.c").read_text()
assert 'PICONIX_FIXED_DEVICES(DEVICE_ENTRY)' in k
assert 'X("/dev/ttyS1", DEV_UART1)' in (root / 'include/device_names.h').read_text()
assert 'PICONIX_WIFI_DEVICES(DEVICE_ENTRY)' in k
assert 'X("/dev/ttyN0", DEV_WIFI_TTY)' in (root / 'include/device_names.h').read_text()
assert 'X("/dev/net/wifi", DEV_WIFI_CTRL)' in (root / 'include/device_names.h').read_text()
assert 'case VDIR_DEV_NET:' in k
assert 'net_wifi_request_restart()' in k
assert 'sta_manual_restart' in n
for d in ('kernel', 'userland', 'docs'):
    for p in (root / d).rglob('*'):
        if p.is_file() and p.suffix in ('.c', '.h', '.md'):
            assert '/dev/uart1' not in p.read_text(), p
            assert '/dev/wifi' not in p.read_text(), p
print('PASS: TTY names and deferred STA control')
