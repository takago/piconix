"""Check smaller PicoNIX MCP tool surface and embedded documentation."""
from pathlib import Path
import ast, json, re, shutil, subprocess
root=Path(__file__).resolve().parents[2]
s=(root/'userland/apps/mcpd.c').read_text()
lists=[]
for line in s.splitlines():
    if 'rpc_result(id, "' in line and r'\"tools\"' in line:
        blob=ast.literal_eval(line.split('rpc_result(id, ',1)[1].rsplit(');',1)[0])
        data=json.loads(blob)
        if 'tools' in data: lists.append([t['name'] for t in data['tools']])
assert lists == [['get_temperature','write_file','execute_command','get_manual'], ['get_temperature']], lists
assert 'scroll_message(' not in s
assert '"get_wifi_info"' not in s and '"display_message"' not in s
import ast
for line in s.splitlines():
    if 'rpc_result(id, "' in line and r'\"tools\"' in line:
        raw=line.split('rpc_result(id, ',1)[1].rsplit(');',1)[0]
        payload=json.loads(ast.literal_eval(raw))
        for tool in payload.get('tools',[]):
            if tool['name']=='get_manual':
                assert tool['inputSchema']=={'type':'object','properties':{'topic':{'type':'string'}},'additionalProperties':False}
                break
        else:
            continue
        break
else:
    raise AssertionError('get_manual tool missing')
assert 'return send_full_manual(fd,id,topic);' in s and 'Transfer-Encoding: chunked' in s
assert 'send_full_manual(fd,id,topic);' in s
assert 'get_manual' in s and 'read(file,part,sizeof part)' in s
for lang in ['ja','en']:
    guide=(root/f'docs/{lang}/guides/MCP.md').read_text()
    assert 'get_manual()' in guide
print('PASS: MCP tool lists, retired tools, embedded manuals, bilingual documentation')
