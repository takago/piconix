"""Test PicoNIX MCP streaming encoder with a native host-side transport stub."""
from pathlib import Path
import json
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
s = (root / 'userland/apps/mcpd.c').read_text()
a = s.index('static int send_chunk(')
b = s.index('/* シェルを起動し，最大4 KiB', a)
code = s[a:b]
assert 'COMMAND_OUTPUT_LIMIT 4096' in s
assert 'send_escaped_chunk(fd,raw,copy)' in s
assert 'Transfer-Encoding: chunked' in s
if compiler := shutil.which('cc') or shutil.which('clang'):
    harness = r'''
#include <stdio.h>
#include <string.h>
static int send_all(int fd, const char *p, unsigned n) {
    (void)fd;
    return fwrite(p,1,n,stdout)==n ? 0 : -1;
}
''' + code + r'''
int main(void) {
    const unsigned char sample[] = {'A','"','\\','\n','\r','\t',0,0x1f,0x80,0xff,'Z'};
    if (send_chunk(1,"[\"",2)<0) return 1;
    if (send_escaped_chunk(1,(const char *)sample,sizeof sample)<0) return 2;
    if (send_chunk(1,"\"]",2)<0) return 3;
    return send_all(1,"0\r\n\r\n",5)<0;
}
'''
    with tempfile.TemporaryDirectory() as tmp:
        c = Path(tmp) / 'test.c'
        binary = Path(tmp) / 'test'
        c.write_text(harness)
        subprocess.run([compiler,'-std=c99','-Wall','-Wextra','-Werror',str(c),'-o',str(binary)],check=True)
        raw = subprocess.check_output([binary])
        chunks=[]
        pos=0
        while True:
            ix=raw.index(b'\r\n',pos)
            n=int(raw[pos:ix],16)
            pos=ix+2
            if not n:
                assert raw[pos:]==b'\r\n'
                break
            chunks.append(raw[pos:pos+n])
            pos+=n
            assert raw[pos:pos+2]==b'\r\n'
            pos+=2
        decoded=json.loads(b''.join(chunks))
        assert decoded==['A"\\\n\r\t\x00\x1f\x80\xffZ'],repr(decoded)
print('PASS: HTTP chunk framing and control/quote/non-ASCII JSON escaping')
