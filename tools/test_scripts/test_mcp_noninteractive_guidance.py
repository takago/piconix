"""Ensure MCP agents are told not to start terminal-only applications."""
from pathlib import Path
import ast, json
root = Path(__file__).resolve().parents[2]
s = (root/'userland/apps/mcpd.c').read_text()
assert 'Run shell command (noninteractive); do not stop mcpd' in s
for name in ( 'terminal_read', 'terminal_send', 'execute_command'):
    assert name in s, name
assert 'Run shell command (noninteractive); do not stop mcpd' in s
assert 'terminal_start_shell' in s and 'terminal_read' in s
for locale in ('ja','en'):
    guide = (root/f'docs/{locale}/reference/commands/MCP.md').read_text()
    for name in ('mem','watch','vi','edit','fd','pong'):
        assert f'`{name}`' in guide
print('PASS: MCP non-interactive tool guidance and PTY documentation')
