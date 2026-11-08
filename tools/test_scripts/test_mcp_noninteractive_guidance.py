"""Ensure MCP agents are told not to start terminal-only applications."""
from pathlib import Path
import ast, json
root = Path(__file__).resolve().parents[2]
s = (root/'userland/apps/mcpd.c').read_text()
assert 'Non-interactive commands ONLY' in s
for name in ('mem', 'watch', 'vi', 'edit', 'fd', 'pong', 'gpio', 'fbed'):
    assert name in s, name
for line in s.splitlines():
    if 'rpc_result(id, "' in line and r'\"tools\"' in line:
        payload = json.loads(ast.literal_eval(line.split('rpc_result(id, ',1)[1].rsplit(');',1)[0]))
        tool = next((t for t in payload.get('tools', []) if t['name']=='execute_command'), None)
        if tool:
            d=tool['description']
            assert 'non-interactive' in d and 'mem' in d and 'watch' in d
            break
else:
    raise AssertionError('execute_command not advertised')
for locale in ('ja','en'):
    guide = (root/f'docs/{locale}/guides/AGENT-GUIDE.md').read_text()
    for name in ('mem','watch','vi','edit','fd','pong'):
        assert f'`{name}`' in guide
print('PASS: MCP non-interactive guidance is in tool description, built-in manual, and agent guides')
