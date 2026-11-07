#!/usr/bin/env python3
"""Host-side end-to-end encoding test for stack-based PicoNIX cc locals."""
from pathlib import Path
import json
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
includes = ['-I' + str(root / 'userland/apps'), '-I' + str(root / 'userland/lib')]
program = '''int main(void) {
 int a=0; int b=1; int c=2; int d=3; int e=4;
 while(a<3) { a=a+1; e=e+1; }
 if(e>5) { write(1,"OK\\n",3); } else { write(1,"NO\\n",3); }
 return 0;
}'''
with tempfile.TemporaryDirectory() as temp:
    tmp = Path(temp)
    asm_file = tmp / 'program.s'
    compiler = '''#define main cc_app_main
#include "cc.c"
#undef main
int main(void) {
 strcpy(source, SOURCE);
 if(compile()!=0) return 1;
 FILE *f=fopen(OUTPUT,"wb");
 if(!f)return 2;
 fwrite(output,1,used,f);fclose(f);return 0;
}
'''.replace('SOURCE', json.dumps(program)).replace('OUTPUT', json.dumps(str(asm_file)))
    (tmp / 'compiler.c').write_text(compiler)
    subprocess.run(['cc', '-std=c11', '-DPICONIX_HOST_TEST', *includes, str(tmp/'compiler.c'), '-o', str(tmp/'compiler')], check=True)
    subprocess.run([str(tmp/'compiler')], check=True)
    assembly = asm_file.read_text()
    assert 'sub sp, #256' in assembly and 'str r0, [r4, #16]' in assembly
    assembler = '''#define main asm_app_main
#include "asm.c"
#undef main
int main(void) {
 FILE *f=fopen(INPUT,"rb"); if(!f)return 1;
 size_t n=fread(src,1,2048,f);fclose(f);src[n]=0;
 int size=compile(0);
 return size>0 && compile(1)==size ? 0 : 2;
}
'''.replace('INPUT',json.dumps(str(asm_file)))
    (tmp/'assembler.c').write_text(assembler)
    subprocess.run(['cc', '-std=c11', '-DPICONIX_HOST_TEST', *includes, str(tmp/'assembler.c'), '-o', str(tmp/'assembler')], check=True)
    subprocess.run([str(tmp/'assembler')], check=True)
print('PASS: cc stack locals, control flow, and Thumb assembly')
