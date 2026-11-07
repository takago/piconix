#!/usr/bin/env python3
"""Host-side code generation and assembler regression for user-defined functions."""
from pathlib import Path
import subprocess, tempfile, json
root=Path(__file__).resolve().parents[2]
includes=['-I'+str(root/'userland/apps'),'-I'+str(root/'userland/lib')]
programs=[
'''int add(int a,int b) {return a+b;} int main(void){int n=add(10,20);return n;}''',
'''int twice(int x){return x+x;} int square(int x){return x*x;} int main(void){return twice(square(3));}''',
'''int fact(int n){if(n<=1){return 1;}return n*fact(n-1);} int main(void){return fact(4);}''',
'''void noop(void){return;} int main(void){noop();return 0;}''',
'''int sum4(int a,int b,int c,int d){return a+b+c+d;} int main(void){return sum4(1,2,3,4);}''',
'''int add(int a,int b){return a+b;} int square(int x){return x*x;} int calc(int a,int b){int sum;sum=add(a,b);return square(sum);} int main(void){int result;result=calc(3,4);if(result==49){write(1,"PASS\\n",5);}else{write(1,"FAIL\\n",5);}return 0;}''',
]
with tempfile.TemporaryDirectory() as d:
 t=Path(d)
 (t/'as_host.c').write_text('''#define main asm_app_main
#include "asm.c"
#undef main
int main(int argc,char **argv){FILE *f=fopen(argv[1],"rb");if(!f)return 3;size_t n=fread(src,1,MAX_SRC+1,f);fclose(f);if(n>MAX_SRC)return 4;src[n]=0;int size=compile(0);if(size<=0||compile(1)!=size)return 1;return 0;}''')
 subprocess.run(['cc','-std=c11','-DPICONIX_HOST_TEST',*includes,str(t/'as_host.c'),'-o',str(t/'as_host')],check=True)
 for i,program in enumerate(programs):
  (t/'cc_host.c').write_text('''#define main cc_app_main
#include "cc.c"
#undef main
int main(int argc,char **argv){strcpy(source,PROGRAM);if(compile())return 1;FILE *f=fopen(argv[1],"wb");if(!f)return 3;fwrite(output,1,used,f);fclose(f);return 0;}'''.replace('PROGRAM',json.dumps(program)))
  subprocess.run(['cc','-std=c11','-DPICONIX_HOST_TEST',*includes,str(t/'cc_host.c'),'-o',str(t/'cc_host')],check=True)
  asm=t/'test.s';subprocess.run([str(t/'cc_host'),str(asm)],check=True)
  assert 'push {r4, lr}' in asm.read_text()
  assert 'addsp r4, #0' in asm.read_text()
  assert 'pop {r4, pc}' in asm.read_text()
  subprocess.run([str(t/'as_host'),str(asm)],check=True)
 print('PASS: 6 function-definition/call programs assemble (execution not verified)')
