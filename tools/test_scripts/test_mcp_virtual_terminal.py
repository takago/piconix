#!/usr/bin/env python3
"""Check MCP terminal tools, streamed schema, virtual screen and key devices."""
from pathlib import Path
import ast
import json
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
m = (root / 'userland/apps/mcpd.c').read_text()
k = (root / 'kernel/kernel.c').read_text()
dev = (root / 'include/device_names.h').read_text()

# Streamed tools/list is assembled from independently transmitted JSON fragments.
section = m[m.index('static int list_tools_stream'):m.index('/* mcpd専用PTY')]
fragments = []
for line in section.splitlines():
    found = re.search(r'const char \*base=(".*");', line)
    if found:
        fragments.append(ast.literal_eval(found.group(1)))
    found = re.search(r'MCP_ADD\((".*")\);', line)
    if found:
        fragments.append(ast.literal_eval(found.group(1)))
# MCP 2026-07-28 ListToolsResult requires cacheScope and ttlMs.
# Reconstruct the streamed response envelope as well as the tool entries.
header = re.search(r'const char \*headers=.*?;', section)
prefix = re.search(r'int n=snprintf\(prefix,sizeof prefix,(".*?"),id\);', section)
assert header and prefix
prefix_json = ast.literal_eval(prefix.group(1)) % '1'
full_response = json.loads(prefix_json + ''.join(fragments) + ']}}')
result = full_response['result']
assert result['resultType'] == 'complete'
assert result['cacheScope'] == 'private'
assert isinstance(result['ttlMs'], int) and result['ttlMs'] >= 0
tools = json.loads('[' + ''.join(fragments) + ']')
names = [tool['name'] for tool in tools]
assert len(names) == len(set(names)) == 6
assert 'terminal_open' not in names and 'terminal_close' not in names
assert {'get_manual', 'execute_command', 'terminal_send',
        'terminal_read'} <= set(names)
for item in tools:
    assert item['inputSchema']['type'] == 'object'

assert 'VT_COLS 80u' in k and 'VT_ROWS 24u' in k
assert 'PTY_OUT_BYTES 1024u' in k
assert 'pty_screen_byte(slot,c);' in k
assert 'pty_screen_read(fd,p,n)' in k
assert 'pty_key_write(pty_slot(dev),p,n)' in k
assert 'terminal_screen_stream(fd,id)' in m
assert 'term_last_hash' not in m
assert m.count('int screen=open(path,O_RDONLY)') == 1
assert 'protected_target' in k and 'mcp_origin' in k
for path in ('/dev/pts/0.screen', '/dev/pts/0.key', '/dev/pts/1.screen', '/dev/pts/1.key'):
    assert path in dev
for locale in ('ja', 'en'):
    for name in ('PTY.md',):
        assert '0.screen' in (root / f'docs/{locale}/reference/devices/{name}').read_text()

if clang := shutil.which('clang'):
    with tempfile.TemporaryDirectory() as td:
        obj = Path(td) / 'mcpd.o'
        subprocess.run([clang, '--target=arm-none-eabi', '-mcpu=cortex-m0plus',
                        '-mthumb', '-Oz', '-fPIC', '-ffreestanding', '-fno-jump-tables',
                        '-Iinclude', '-Iuserland/lib', '-c', 'userland/apps/mcpd.c',
                        '-o', str(obj)], cwd=root, check=True)
        if objdump := shutil.which('llvm-objdump'):
            reloc = subprocess.check_output([objdump, '-r', str(obj)], text=True)
            assert 'R_ARM_ABS32' not in reloc, 'mcpd contains absolute-address relocation'
print('PASS: MCP terminal schemas, screen/key paths, memory-safe streaming and ARM PIC relocations')
