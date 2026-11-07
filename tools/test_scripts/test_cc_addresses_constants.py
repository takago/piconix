#!/usr/bin/env python3
"""Host-side tests: scalar address-of, one-element arrays and wide constants."""
from pathlib import Path
import json, subprocess, tempfile
root=Path(__file__).resolve().parents[2]
includes=['-I'+str(root/'userland/apps'), '-I'+str(root/'userland/lib')]
programs=[
'''int main(void){char c;c='A';write(1,&c,1);return 0;}''',
'''int main(void){char c;int n;n=read(0,&c,1);if(n==1){write(1,&c,1);}return 0;}''',
'''int main(void){char c[1];c[0]='A';write(1,c,1);return 0;}''',
'''int main(void){char c[1];int n;n=read(0,c,sizeof(c));if(n==1){write(1,c,1);}return 0;}''',
'''int main(void){int n;n=12345;if(n==12345){write(1,"PASS\\n",5);}return 0;}''',
'''void print_number(int n){char digit[1];if(n>=10){print_number(n/10);}digit[0]='0'+n%10;write(1,digit,1);}int main(void){int n;n=12345;print_number(n);write(1,"\\n",1);return 0;}''',
]
with tempfile.TemporaryDirectory() as d:
 t=Path(d)
 (t/'asm_host.c').write_text('''#define main asm_app_main
#include "asm.c"
#undef main
int main(int argc,char **argv){FILE *f=fopen(argv[1],"rb");if(!f)return 3;size_t n=fread(src,1,MAX_SRC+1,f);fclose(f);if(n>MAX_SRC)return 4;src[n]=0;int size=compile(0);if(size<=0||compile(1)!=size)return 1;return 0;}''')
 subprocess.run(['cc','-std=c11','-DPICONIX_HOST_TEST',*includes,str(t/'asm_host.c'),'-o',str(t/'asm_host')],check=True)
 for i,program in enumerate(programs):
  (t/'cc_host.c').write_text('''#define main cc_app_main
#include "cc.c"
#undef main
int main(int argc,char **argv){strcpy(source,PROGRAM);if(compile())return 1;FILE *f=fopen(argv[1],"wb");if(!f)return 3;fwrite(output,1,used,f);fclose(f);return 0;}'''.replace('PROGRAM',json.dumps(program)))
  subprocess.run(['cc','-std=c11','-DPICONIX_HOST_TEST',*includes,str(t/'cc_host.c'),'-o',str(t/'cc_host')],check=True)
  asm=t/'case.s';subprocess.run([str(t/'cc_host'),str(asm)],check=True)
  assert asm.stat().st_size<=2048, f'case {i} exceeds as input limit: {asm.stat().st_size}'
  subprocess.run([str(t/'asm_host'),str(asm)],check=True)
  print(f'PASS case {i}: {asm.stat().st_size} assembly bytes')
