#!/usr/bin/env python3
"""Compile C expressions and assemble them with the actual PicoNIX as parser."""
from pathlib import Path
import json, subprocess, tempfile
root=Path(__file__).resolve().parents[2]
includes=['-I'+str(root/'userland/apps'),'-I'+str(root/'userland/lib')]
programs=[
'''int main(void) { int a=10; int b=3; int c; c=(a+b*2)/4; if(c==4){write(1,"OK\\n",3);} return 0; }''',
'''int main(void) { int x=9; int y=4; int z; z=x%y; if(z==1){write(1,"MOD\\n",4);} return 0; }''',
'''int main(void) { int x=3; int y=2; int z; z=-(x+y)*2; if(z!=0){write(1,"NEG\\n",4);} return 0; }''',
]
with tempfile.TemporaryDirectory() as d:
 t=Path(d)
 for i,program in enumerate(programs):
  asm=t/f'{i}.s'
  c='''#define main cc_app_main
#include "cc.c"
#undef main
int main(void) {strcpy(source, SOURCE);if(compile())return 1;FILE *f=fopen(OUTPUT,"wb");if(!f)return 2;fwrite(output,1,used,f);fclose(f);return 0;}
'''.replace('SOURCE',json.dumps(program)).replace('OUTPUT',json.dumps(str(asm)))
  (t/'compiler.c').write_text(c)
  subprocess.run(['cc','-std=c11','-DPICONIX_HOST_TEST',*includes,str(t/'compiler.c'),'-o',str(t/'compiler')],check=True)
  subprocess.run([str(t/'compiler')],check=True)
  a='''#define main asm_app_main
#include "asm.c"
#undef main
int main(void) {FILE *f=fopen(INPUT,"rb");if(!f)return 1;size_t n=fread(src,1,2048,f);fclose(f);src[n]=0;int size=compile(0);return size>0 && compile(1)==size ? 0 : 2;}
'''.replace('INPUT',json.dumps(str(asm)))
  (t/'assembler.c').write_text(a)
  subprocess.run(['cc','-std=c11','-DPICONIX_HOST_TEST',*includes,str(t/'assembler.c'),'-o',str(t/'assembler')],check=True)
  subprocess.run([str(t/'assembler')],check=True)
  print('PASS:',i,'assembly bytes',asm.stat().st_size)
