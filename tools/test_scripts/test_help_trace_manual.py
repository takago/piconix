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
found_manual=False
for line in m.splitlines():
    if 'rpc_result(id, "' in line and r'\"tools\"' in line:
        d=json.loads(ast.literal_eval(line.split('rpc_result(id, ',1)[1].rsplit(');',1)[0]))
        matches=[x for x in d.get('tools',[]) if x['name']=='get_manual']
        if matches:
            found_manual=True
            schema=matches[0]['inputSchema']
            assert schema['type']=='object'
            assert schema['properties']=={'topic': {'type': 'string'}}
            assert 'required' not in schema or not schema['required']
            assert schema.get('additionalProperties') is False
assert found_manual, 'get_manual schema missing'
print('PASS: trace help and full MCP manual contract')
