"""Check help matches implemented trace modes and full manual contract."""
from pathlib import Path
import ast, json
root=Path(__file__).resolve().parents[2]
h=(root/'userland/apps/help.c').read_text()
t=(root/'userland/apps/trace.c').read_text()
m=(root/'userland/apps/mcpd.c').read_text()
for token in ('trace on [-s|-a]', 'run [-s|-a]', 'show [PID]',
              'follow [PID]', 'Trace buffer overwrites'):
    assert token in h, token
for token in ('TRACE_MODE_SYSCALL','TRACE_MODE_ALL','trace_drain','TRACE_OFF'):
    assert token in t, token
assert 'send_full_manual(fd,id,topic)' in m
assert 'send_escaped_chunk(fd,part' in m
assert 'Transfer-Encoding: chunked' in m
# tools/listは単一のrpc_result文字列から，逐次送信するMCP_ADDへ移行済み．
# 送信されるJSON片を解析し，現在の公開スキーマを検証する．
import re
fragments=re.findall(r'MCP_ADD\("((?:\\.|[^"\\])*)"\);',m)
fragments=[ast.literal_eval('"'+part+'"') for part in fragments]
manual=[json.loads(fragment[1:]) for fragment in fragments if fragment.startswith(',')]
matches=[entry for entry in manual if entry.get('name')=='get_manual']
assert len(matches)==1, 'get_manual schema missing'
schema=matches[0]['inputSchema']
assert schema['type']=='object'
assert isinstance(schema.get('properties'), dict)
assert isinstance(schema['properties'].get('topic'), dict)
assert schema['properties']['topic']['type']=='string'
assert 'required' not in schema or not schema['required']
print('PASS: trace help and full MCP manual contract')
