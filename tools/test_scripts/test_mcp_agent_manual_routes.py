#!/usr/bin/env python3
"""Check LLM-facing ROM manual entrypoints and current MCP tool behavior."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
pages = root / 'manual'
mcp = (root / 'userland/apps/mcpd.c').read_text()
index = (pages / 'README').read_text()
assert len((pages / 'README').read_bytes()) < 4096
for page in ('agent', 'mcp-workflow', 'terminal-operations', 'pty',
             'quickstart', 'safety-recovery', 'vi', 'fd', 'reversi'):
    assert (pages / page).is_file(), page
    assert 0 < len((pages / page).read_bytes()) < 4096, page
    assert page in index or page in ('safety-recovery',), page
terminal = (pages / 'terminal-operations').read_text()
agent = (pages / 'agent').read_text()
for token in ('terminal_read', 'terminal_send', 'ESC', 'CTRL_C',
              'Shell', 'restart', 'reversi', 'vi', 'fd', 'accepted'):
    assert token.lower() in terminal.lower(), token
assert 'terminal-operations' in agent and 'mcp-workflow' in agent
assert 'terminal_start_shell()' in mcp and 'terminal_ensure_shell()' in mcp
assert 'Shell restarted; read terminal before sending input' in mcp
assert 'get_manual' in mcp
for locale in ('ja', 'en'):
    text = (root / f'docs/{locale}/reference/commands/MCP.md').read_text()
    assert 'terminal-operations' in text
    assert 'four `terminal_*`' not in text and '4つの`terminal_*`' not in text
assert '2,400-byte JSON request-body limit' in (root / 'docs/en/reference/commands/MCP.md').read_text()
print('PASS: ROM agent routing, PTY state guidance, page size and bilingual MCP contracts')
