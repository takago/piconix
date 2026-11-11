#!/usr/bin/env python3
"""Ensure MCP has a persistent boot-time shell with a six-tool surface."""
from pathlib import Path
import ast, json, re
root=Path(__file__).resolve().parents[2]
s=(root/'userland/apps/mcpd.c').read_text()
section=s.split('static int list_tools_stream',1)[1].split('/* mcpd専用PTY',1)[0]
parts=[]
for line in section.splitlines():
    m=re.search(r'const char \*base=(".*");',line) or re.search(r'MCP_ADD\((".*")\);',line)
    if m: parts.append(ast.literal_eval(m.group(1)))
names=[t['name'] for t in json.loads('['+''.join(parts)+']')]
assert names == ['get_temperature','get_manual','write_file','execute_command','terminal_send','terminal_read'],names
assert 'terminal_open_command(' not in s
assert 'if(terminal_start_shell()<0)' in s
assert 'term_master=master;term_pid=pid;' in s
assert 'terminal_ensure_shell()' in s
assert 'Shell restarted; read terminal before sending input' in s
assert 'if(term_master>=0)terminal_close_session();' in s
assert 'term_master<0' not in s.split('if(json_equals(v,v_end,"terminal_send")',1)[1].split('int written=-1;',1)[0]
for locale in ('ja','en'):
    d=(root/f'docs/{locale}/reference/commands/MCP.md').read_text()
    assert '6' in d and 'terminal_start_shell' not in d
for topic in ('agent','pty','reversi','vi','fd','mcp-workflow'):
    m=(root/'manual'/topic).read_text()
    assert 'terminal_open(' not in m and 'terminal_close(' not in m,(topic,m)
print('PASS: six-tool MCP startup shell and client/manual contracts')
