#!/usr/bin/env python3
"""Check current bilingual manuals, Markdown links and common interface claims."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
docs = root / 'docs'
ja = {str(f.relative_to(docs / 'ja')) for f in (docs / 'ja').rglob('*.md')}
en = {str(f.relative_to(docs / 'en')) for f in (docs / 'en').rglob('*.md')}
assert ja and ja == en, f'Japanese/English manual mismatch: {ja ^ en}'

files = [root / 'README.md', *docs.rglob('*.md'),
         *(root / 'userland' / 'examples').rglob('*.md')]
count = 0
for f in files:
    text = f.read_text(encoding='utf-8')
    # Ignore public URLs, mailto, and same-page fragments.
    for target in re.findall(r'!?(?:\[[^\]]*\])\(([^)]+)\)', text):
        path = target.split('#', 1)[0].split(' ', 1)[0].strip('<>')
        if not path or '://' in path or path.startswith(('mailto:', 'data:')):
            continue
        count += 1
        assert (f.parent / path).exists(), f'{f}: broken link {target}'

# 現行のedit/viは/tmpと/etcの両方を受け付ける．教材も実装と一致させる．
editor = (root / 'userland/apps/editor_common.h').read_text(encoding='utf-8')
assert 'strncmp(path,"/tmp/",5)&&strncmp(path,"/etc/",5)' in editor
for f in [root / 'README.md', docs / 'en' / 'reference' / 'network' / 'HTTPD.md',
          docs / 'ja' / 'reference' / 'network' / 'HTTPD.md',
          docs / 'en' / 'reference' / 'commands' / 'EDIT.md', docs / 'ja' / 'reference' / 'commands' / 'EDIT.md']:
    assert 'edit /etc/index.html' in f.read_text(encoding='utf-8'), f
assert 'Horizontal scrolling is not implemented' not in (docs/'en'/'reference' / 'commands'/'EDIT.md').read_text()
assert '横方向のスクロールは未対応' not in (docs/'ja'/'reference' / 'commands'/'EDIT.md').read_text()
for lang in ('en', 'ja'):
    trace = (docs/lang/'reference' / 'commands'/'TRACE.md').read_text()
    assert 'ls -l' in trace
    assert ('device' in trace.lower()) if lang == 'en' else ('デバイス' in trace)
# pingはPico Wのユーザコマンドとして実装され，専用ICMPデバイスを使う．
# Wi-Fi診断文書に旧版の「pingは未実装」という記述を戻さないための回帰検査．
ping_app = (root / 'userland' / 'apps' / 'ping.c').read_text(encoding='utf-8')
assert 'open("/dev/net/icmp",O_RDWR)' in ping_app
for lang in ('en', 'ja'):
    wifi_modes = (docs / lang / 'reference' / 'network' / 'WIFI-MODES.md').read_text(encoding='utf-8')
    link = '[Ping](PING.md)' if lang == 'en' else '[ping](PING.md)'
    assert link in wifi_modes
    assert 'not** a PicoNIX command' not in wifi_modes
    assert 'PicoNIXに`ping`コマンドは実装していません' not in wifi_modes

# The syscall table is the ABI authority. The uptime exercise must never reuse select(40).
syscalls = (root / 'include/piconix_syscall.h').read_text(encoding='utf-8')
assert 'X(SYS_SELECT,        40, "select")' in syscalls
for lang in ('ja', 'en'):
    specification = (docs/lang/'reference'/'SPECIFICATION.md').read_text(encoding='utf-8')
    lab = (docs/lang/'development'/'syscalls'/'SYSCALL-DEVELOPMENT.md').read_text(encoding='utf-8')
    assert '1〜39' not in specification and '1–39' not in specification
    assert 'SYS_GETUPTIME,    41' in lab
    assert (docs/lang/'development'/'apps'/'c'/'MCP-DEVELOPMENT.md').exists()
assert not (docs/'examples'/'uptime_lab.patch').exists(), 'obsolete lab patch must not be restored'

print(f'PASS: {len(ja)} bilingual manual pairs, {count} local links and interface claims')
