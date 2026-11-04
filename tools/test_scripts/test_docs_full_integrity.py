#!/usr/bin/env python3
"""Check the current bilingual documentation links and boot-file examples."""
from pathlib import Path
import importlib.util
import re
import tempfile

root = Path(__file__).resolve().parents[2]
docs = root / 'docs'

ja = {p.relative_to(docs / 'ja') for p in (docs / 'ja').rglob('*.md')}
en = {p.relative_to(docs / 'en') for p in (docs / 'en').rglob('*.md')}
assert ja == en, f'bilingual paths differ: ja-only={ja-en}, en-only={en-ja}'
assert len(ja) > 0

# Link targets are checked relative to each source Markdown file. External links
# are intentionally not fetched; their availability depends on the network.
links = 0
for source in [root / 'README.md', *docs.rglob('*.md')]:
    content = source.read_text(encoding='utf-8')
    for raw in re.findall(r'(?<!!)\[[^\]]*\]\(([^)]+)\)', content):
        target = raw.split(' ', 1)[0].strip('<>')
        if target.startswith(('http:', 'https:', 'mailto:', '#')):
            continue
        filename, _, anchor = target.partition('#')
        dest = (source.parent / filename).resolve()
        assert dest.is_file(), f'broken Markdown link: {source}: {target}'
        links += 1
        if anchor and dest.suffix.lower() == '.md':
            headings = re.findall(r'^#{1,6}\s+(.+)$', dest.read_text(encoding='utf-8'), re.M)
            slugs = []
            used = {}
            for heading in headings:
                heading = re.sub(r'\[([^\]]+)\]\([^)]+\)', r'\1', heading)
                heading = re.sub(r'<[^>]*>', '', heading).lower().strip()
                slug = re.sub(r'[^\w\s-]', '', heading)
                slug = re.sub(r'\s+', '-', slug)
                ordinal = used.get(slug, 0)
                used[slug] = ordinal + 1
                slugs.append(slug + (f'-{ordinal}' if ordinal else ''))
            assert anchor in slugs, f'unknown heading anchor: {source}: {target}'

# Check actual generated initial-file names rather than a manually maintained
# list; it catches naming drift without requiring an ARM toolchain.
spec = importlib.util.spec_from_file_location('generate_initial_fs', root / 'tools/generate_initial_fs.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
with tempfile.TemporaryDirectory() as temp:
    pico = module.generate(root / 'userland/fs', 'pico', Path(temp) / 'pico.inc')
    picow = module.generate(root / 'userland/fs', 'pico_w', Path(temp) / 'picow.inc')
assert 'tmp/bh1750_fbwave.py' in pico and 'tmp/bh1750_fbwave.py' in picow
assert pico['tmp/bh1750_fbwave.py'] == (root / 'userland/examples/python/wave.py').read_bytes()
for language in ('ja', 'en'):
    for relative in ('reference/INITIAL-FILES.md', 'reference/FILESYSTEM.md',
                     'development/apps/python/PYTHON.md'):
        content = (docs / language / relative).read_text(encoding='utf-8')
        assert '/tmp/bh1750_fbwave.py' in content, f'initial sample missing: {language}/{relative}'
        assert '/tmp/wave.py' not in content, f'obsolete initial sample name: {language}/{relative}'
print(f'PASS: {len(ja)} bilingual document pairs; {links} local Markdown links; initial files for Pico/Pico W')
