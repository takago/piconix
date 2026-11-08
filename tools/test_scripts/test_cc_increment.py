#!/usr/bin/env python3
"""Compile supported ++/-- C forms with real cc parser and verify emitted assembly."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
source = (root/'userland/apps/cc.c').read_text()
for h in ('unistd','fcntl','stdio','string'):
    source=source.replace(f'#include "{h}.h"',f'#include <{h}.h>')
with tempfile.TemporaryDirectory() as d:
    p=Path(d)
    (p/'cc_host.c').write_text(source)
    subprocess.run(['cc','-std=c11','-O0',str(p/'cc_host.c'),'-o',str(p/'compiler')],check=True)
    # Assemble the emitted Thumb-1 text with PicoNIX's own parser.
    assembler = '#define main asm_app_main\n#include "asm.c"\n#undef main\nint main(int argc,char **argv){FILE *f=fopen(argv[1],"rb");if(!f)return 3;size_t n=fread(src,1,MAX_SRC+1,f);fclose(f);if(n>MAX_SRC)return 4;src[n]=0;int size=compile(0);return size>0&&compile(1)==size?0:1;}\n'
    (p/'as_host.c').write_text(assembler)
    subprocess.run(['cc','-std=c11','-DPICONIX_HOST_TEST', '-I'+str(root/'userland/apps'), '-I'+str(root/'userland/lib'), str(p/'as_host.c'),'-o',str(p/'assembler')],check=True)
    def check(program,accepted):
        f=p/'test.c'; f.write_text(program)
        result=subprocess.run([str(p/'compiler'),str(f)],text=True,capture_output=True)
        assert (result.returncode==0)==accepted, (program,result.stdout,result.stderr)
        if accepted:
            asm=(p/'test.s').read_text()
            assert 'adds r0, #1' in asm or 'subs r0, #1' in asm, asm
            subprocess.run([str(p/'assembler'),str(p/'test.s')],check=True)
            (p/'test.s').unlink()
    # all four statement forms, expression forms in initializers/condition
    check('int main(){int i=1;i++;++i;i--;--i;return i;}',True)
    check('int main(){int i=1;int a=i++;int b=++i;return a+b;}',True)
    check('int main(){int i=1;int a=i--;int b=--i;return a+b;}',True)
    check('int main(){int i=0;while(i++<3){i=i+1;}return i;}',True)
    check('int main(){char a=1;a++;return 0;}',False)
    check('int main(){int a=1;int *p=&a;p++;return a;}',False)
    check('int main(){int a[2];a[0]++;return 0;}',False)
print('PASS: integer ++/-- parsing and rejected lvalues')
