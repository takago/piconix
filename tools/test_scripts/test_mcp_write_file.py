from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
s = (ROOT / 'userland/apps/mcpd.c').read_text()
a=s.index('static int decode_file_text(');b=s.index('/* HTTP/1.1のchunked転送．',a)
funcs=s[a:b]
source='''#include <stdio.h>\n#include <string.h>\n#include <unistd.h>\n#include <fcntl.h>\n'''+funcs+r'''
int main(void) {
    char path[32];
    const char *p="\"/tmp/test.c\"";
    if(file_path_argument(p,p+strlen(p),path)!=0 || strcmp(path,"/tmp/test.c"))return 1;
    p="\"/etc/rc\""; if(file_path_argument(p,p+strlen(p),path)==0) return 2;
    p="\"/tmp/../bad\"";if(file_path_argument(p,p+strlen(p),path)==0)return 3;
    const char *txt="\"A\\nB\\\"C\\u0031\"";
    int len=decode_file_text(txt,txt+strlen(txt),-1);
    if(len!=6) return 4;
    int fd=open("/tmp/piconix-write-file-test-output",O_WRONLY|O_CREAT|O_TRUNC,0600);
    if(fd<0)return 5;
    int n=decode_file_text(txt,txt+strlen(txt),fd);close(fd);
    if(n!=6)return 6;
    char buf[32]={0};fd=open("/tmp/piconix-write-file-test-output",O_RDONLY);
    if(read(fd,buf,sizeof buf)!=6)return 7;
    close(fd);
    if(memcmp(buf,"A\nB\"C1",6))return 8;
    const char *bad="\"hello\\u0123\"";
    if(decode_file_text(bad,bad+strlen(bad),-1)>=0)return 9;
    static char longval[2052]; longval[0]='"';
    memset(longval+1,'x',2048);longval[2049]='"';longval[2050]=0;
    if(decode_file_text(longval,longval+2050,-1)!=2048)return 10;
    longval[2049]='x';longval[2050]='"';longval[2051]=0;
    if(decode_file_text(longval,longval+2051,-1)>=0)return 11;
    puts("PASS: JSON string decoding, file write, path restrictions, 2048-byte limit");
    return 0;
}
'''
with tempfile.TemporaryDirectory() as temp:
    c = Path(temp) / 'write_file_test.c'
    exe = Path(temp) / 'write_file_test'
    c.write_text(source)
    subprocess.run(['cc','-Wall','-Wextra','-O2',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
