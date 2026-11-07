"""Regression checks for PicoNIX experimental MCP shell bridge."""
from pathlib import Path
import ast
import json
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
MCP = (ROOT / 'userland/apps/mcpd.c').read_text()
SH = (ROOT / 'userland/apps/shell.c').read_text()
assert 'argc==2 && !strcmp(argv[1],"--shell")' in MCP
assert 'if (allow_shell && json_equals(v,v_end,"execute_command"))' in MCP
assert 'COMMAND_OUTPUT_LIMIT 4096' in MCP
assert 'Transfer-Encoding: chunked' in MCP
assert 'if(status>=0) http_reply(fd, status, response);' in MCP
assert 'send_escaped_chunk(fd,raw,copy)' in MCP
assert 'COMMAND_TIMEOUT_MS 3000' in MCP
assert 'if(argc==3) { batch_mode=1; return run_one_command(argv[2],cwd); }' in SH
assert 'if(!batch_mode) set_foreground(pid[i]);' in SH
lists = []
for line in MCP.splitlines():
    if 'rpc_result(id, "' in line and '"tools' in line:
        quoted = line.split('rpc_result(id, ', 1)[1].rsplit(');', 1)[0]
        obj = json.loads(ast.literal_eval(quoted))
        if 'tools' in obj:
            lists.append([tool['name'] for tool in obj['tools']])
assert len(lists) == 2
assert lists[0] == ['get_temperature', 'get_wifi_info', 'display_message', 'write_file', 'execute_command']
assert lists[1] == ['get_temperature', 'get_wifi_info', 'display_message']
for lang in ('en', 'ja'):
    assert (ROOT / f'docs/{lang}/guides/AGENT-GUIDE.md').exists()
if clang := shutil.which('clang'):
    subprocess.run([clang, '--target=arm-none-eabi', '-mcpu=cortex-m0plus', '-mthumb', '-Oz',
                    '-fPIC', '-ffreestanding', '-fsyntax-only', '-Iinclude', '-Iuserland/lib',
                    'userland/apps/shell.c', 'userland/apps/mcpd.c'],
                   cwd=ROOT, check=True)
print('PASS: opt-in command tool, MCP JSON metadata, shell batch handling and ARM syntax')
