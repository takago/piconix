"""Check smaller PicoNIX MCP tool surface and embedded documentation."""
from pathlib import Path
import ast, json, re, shutil, subprocess
root=Path(__file__).resolve().parents[2]
s=(root/'userland/apps/mcpd.c').read_text()
assert 'list_tools_stream(fd,id)' in s
for name in ('terminal_open','terminal_send','terminal_read','terminal_close','get_manual'):
    assert r'\"name\":\"'+name+r'\"' in s
assert 'scroll_message(' not in s
assert '"get_wifi_info"' not in s and '"display_message"' not in s
assert r'\"topic\":{\"type\":\"string\"' in s
assert 'Call get_manual({}) or topic=index for contents.' in s
assert 'topic=index' in s
assert 'strcmp(topic,"index")' in s and 'strcmp(topic,"INDEX")' in s
assert 'strcpy(topic,"README")' in s
assert 'Unknown topic. Use index.' in s
assert 'return send_full_manual(fd,id,topic);' in s and 'Transfer-Encoding: chunked' in s
assert 'send_full_manual(fd,id,topic);' in s
assert 'get_manual' in s and 'read(file,part,sizeof part)' in s
for lang in ['ja','en']:
    guide=(root/f'docs/{lang}/reference/commands/MCP.md').read_text()
    assert 'get_manual()' in guide
print('PASS: MCP tool lists, retired tools, embedded manuals, bilingual documentation')
