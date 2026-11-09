"""Ensure MCP agents are told not to start terminal-only applications."""
from pathlib import Path
import ast, json
root = Path(__file__).resolve().parents[2]
s = (root/'userland/apps/mcpd.c').read_text()
assert 'Run noninteractive shell command' in s
for name in ('terminal_open', 'terminal_read', 'terminal_send', 'execute_command'):
    assert name in s, name
assert 'Run noninteractive shell command' in s
assert 'only for vi/fd/sh' in s
for locale in ('ja','en'):
    guide = (root/f'docs/{locale}/reference/commands/MCP.md').read_text()
    for name in ('mem','watch','vi','edit','fd','pong'):
        assert f'`{name}`' in guide
print('PASS: MCP non-interactive guidance is in tool description, built-in manual, and agent guides')
