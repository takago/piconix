from pathlib import Path
import subprocess,json,tempfile
root=Path(__file__).resolve().parents[2]
inc=['-I'+str(root/'userland/apps'),'-I'+str(root/'userland/lib')]
tests={
'swap':'''void swap(int *a,int *b){int tmp;tmp=*a;*a=*b;*b=tmp;}int main(void){int x;int y;x=10;y=20;swap(&x,&y);if(x==20){if(y==10){write(1,"PASS\\n",5);}}return 0;}''',
'ptrplus':'''int main(void){int a[3];int *p;a[0]=11;a[1]=22;a[2]=33;p=a;p=p+1;if(*p==22){write(1,"PASS\\n",5);}return 0;}''',
'charptr':'''void change(char *p){*p='Z';}int main(void){char c;c='A';change(&c);write(1,&c,1);return 0;}''',
'ptrindex':'''int main(void){char a[3];char *p;p=a;p[1]='B';if(a[1]=='B'){write(1,"PASS\\n",5);}return 0;}''',
'arrayaddr':'''int main(void){int a[4];int *p;a[2]=77;p=&a[2];if(*p==77){write(1,"PASS\\n",5);}return 0;}''',
'readptr':'''int main(void){char c;char *p;p=&c;read(0,p,1);write(1,p,1);return 0;}''',
}
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp)
 (t/'hostasm.c').write_text('#define main embedded_main\n#include "asm.c"\n#undef main\nint main(int argc,char **argv){FILE *f=fopen(argv[1],"rb");if(!f)return 3;size_t n=fread(src,1,MAX_SRC+1,f);fclose(f);if(n>MAX_SRC)return 4;src[n]=0;int size=compile(0);if(size<=0||compile(1)!=size)return 1;return 0;}')
 subprocess.run(['cc','-std=c11','-DPICONIX_HOST_TEST',*inc,str(t/'hostasm.c'),'-o',str(t/'hostasm')],check=True)
 for name,source in tests.items():
  (t/'hostcc.c').write_text('#define main embedded_main\n#include "cc.c"\n#undef main\nint main(void){strcpy(source,'+json.dumps(source)+');if(compile())return 1;FILE *f=fopen("'+str(t/'out.s')+'","wb");fwrite(output,1,used,f);fclose(f);return 0;}')
  subprocess.run(['cc','-std=c11','-DPICONIX_HOST_TEST',*inc,str(t/'hostcc.c'),'-o',str(t/'hostcc')],check=True)
  a=subprocess.run([str(t/'hostcc')]);
  if a.returncode: print('FAIL compile',name);continue
  size=(t/'out.s').stat().st_size
  a=subprocess.run([str(t/'hostasm'),str(t/'out.s')]);print(name,size,'PASS' if not a.returncode else f'FAIL as {a.returncode}')
