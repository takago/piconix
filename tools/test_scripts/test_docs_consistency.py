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
for f in [root / 'README.md', docs / 'en' / 'development' / 'apps' / 'c' / 'NETWORK-PROGRAMMING.md',
          docs / 'ja' / 'development' / 'apps' / 'c' / 'NETWORK-PROGRAMMING.md']:
    assert 'edit /etc/index.html' in f.read_text(encoding='utf-8'), f
assert 'Horizontal scrolling is not implemented' not in (docs/'en'/'literacy'/'EDIT.md').read_text()
assert '横方向のスクロールは未対応' not in (docs/'ja'/'literacy'/'EDIT.md').read_text()
for lang in ('en', 'ja'):
    trace = (docs/lang/'literacy'/'TRACE.md').read_text()
    assert 'ls -l' in trace
    assert ('device' in trace.lower()) if lang == 'en' else ('デバイス' in trace)
print(f'PASS: {len(ja)} bilingual manual pairs, {count} local links and interface claims')
