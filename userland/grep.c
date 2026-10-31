/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Minimal streaming grep: fixed memory, literal substring, no regex. */
#include "unistd.h"
#include "fcntl.h"
#include "string.h"
#include "stdio.h"

static int emit(const char *p, unsigned n) {
    while(n) {
        int k=write(1,p,n);
        if(k<=0) return -1;
        p+=k; n-=(unsigned)k;
    }
    return 0;
}
static int match(const char *line, unsigned n, const char *pat, unsigned m) {
    if(m>n) return 0;
    for(unsigned i=0;i<=n-m;i++) {
        unsigned j=0;
        while(j<m && line[i+j]==pat[j]) j++;
        if(j==m) return 1;
    }
    return 0;
}
int main(int argc,char **argv) {
    if(argc<2 || argc>3) { puts("usage: grep PATTERN [FILE]"); return 2; }
    const char *pat=argv[1];
    unsigned m=(unsigned)strlen(pat);
    int fd=argc==3?open(argv[2],O_RDONLY):0;
    if(fd<0) { puts("grep: cannot open"); return 2; }
    char line[128],buf[32];
    unsigned used=0;
    int overflow=0, found=0, error=0;
    int n;
    while((n=read(fd,buf,sizeof buf))>0) {
        for(int i=0;i<n;i++) {
            char c=buf[i];
            if(c=='\n') {
                if(!overflow && match(line,used,pat,m)) {
                    if(emit(line,used)<0 || emit("\n",1)<0) {error=1;goto done;}
                    found=1;
                }
                used=0;overflow=0;
            } else if(used<sizeof line) line[used++]=c;
            else overflow=1; /* Skip overlong lines rather than print partial matches. */
        }
    }
    if(n<0) error=1;
    if(!error && used && !overflow && match(line,used,pat,m)) {
        if(emit(line,used)<0) error=1;
        else found=1;
    }
done:
    if(argc==3) close(fd);
    if(error) { puts("grep: read/write error"); return 2; }
    return found?0:1;
}
