"""Check help matches implemented trace modes and full manual contract."""
from pathlib import Path
import ast, json
root=Path(__file__).resolve().parents[2]
h=(root/'userland/apps/help.c').read_text()
t=(root/'userland/apps/trace.c').read_text()
m=(root/'userland/apps/mcpd.c').read_text()
for token in ('trace run [-s|-a]', 'trace on [-s|-a]', 'trace follow [PID]',
              'trace show [PID]', 'Trace buffer overwrites'):
    assert token in h, token
for token in ('TRACE_MODE_SYSCALL','TRACE_MODE_ALL','trace_drain','TRACE_OFF'):
    assert token in t, token
assert 'send_full_manual(fd,id)' in m
assert 'send_escaped_chunk(fd,body' in m
assert 'return -1; /* chunked HTTP' in m
for line in m.splitlines():
    if 'rpc_result(id, "' in line and r'\"tools\"' in line:
        d=json.loads(ast.literal_eval(line.split('rpc_result(id, ',1)[1].rsplit(');',1)[0]))
        matches=[x for x in d.get('tools',[]) if x['name']=='get_manual']
        if matches:
            assert matches[0]['inputSchema']['properties']=={}
            assert 'required' not in matches[0]['inputSchema']
print('PASS: trace help and full MCP manual contract')
