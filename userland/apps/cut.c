/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 区切り文字で分割したテキストの指定列を標準出力へ転送する． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"

#define CUT_FIELDS 64
#define CUT_LINE 512

/* 64列までの番号指定または閉区間を選択表へ格納する． */
static int fields_parse(const char *s, unsigned char selected[CUT_FIELDS]) {
    if(!s || !*s) return -1;
    while(*s) {
        unsigned first=0,last;
        if(*s<'0' || *s>'9') return -1;
        while(*s>='0' && *s<='9') {
            first=first*10u+(unsigned)(*s++-'0');
            if(first>CUT_FIELDS) return -1;
        }
        if(first==0) return -1;
        last=first;
        if(*s=='-') {
            s++;
            if(*s<'0' || *s>'9') return -1;
            last=0;
            while(*s>='0' && *s<='9') {
                last=last*10u+(unsigned)(*s++-'0');
                if(last>CUT_FIELDS) return -1;
            }
            if(last<first) return -1;
        }
        for(unsigned i=first;i<=last;i++) selected[i-1]=1;
        if(!*s) break;
        if(*s++!=',' || !*s) return -1;
    }
    return 0;
}

/* 部分書き込みを扱い，選択したバイト列を確実に転送する． */
static int emit(const char *s,unsigned n) {
    while(n) {
        int k=write(1,s,n);
        if(k==OS_EAGAIN || k==0) { msleep(1); continue; }
        if(k<0) return -1;
        s+=k;
        n-=(unsigned)k;
    }
    return 0;
}

/* 行内の選択列だけを元の並び順で転送する． */
static int emit_line(const char *s,unsigned n,char delim,
                     const unsigned char selected[CUT_FIELDS],int newline) {
    unsigned i=0,field=0;
    int has_delim=0,printed=0;
    for(unsigned j=0;j<n;j++) if(s[j]==delim) {has_delim=1;break;}
    /* POSIX cut と同様，区切り文字のない行は変更せず通す． */
    if(!has_delim) {
        if(emit(s,n)<0) return -1;
    } else {
        while(i<=n) {
            unsigned start=i;
            while(i<n && s[i]!=delim) i++;
            if(field<CUT_FIELDS && selected[field]) {
                if(printed && emit(&delim,1)<0) return -1;
                if(emit(s+start,i-start)<0) return -1;
                printed=1;
            }
            field++;
            if(i==n) break;
            i++;
        }
    }
    return newline?emit("\n",1):0;
}

/* 標準入力または指定ファイルを逐次読み出して列抽出する． */
int main(int argc,char **argv) {
    char delim='\t';
    const char *spec=0,*path=0;
    unsigned char selected[CUT_FIELDS]={0};
    for(int i=1;i<argc;i++) {
        const char *a=argv[i];
        if(a[0]=='-' && a[1]=='d') {
            const char *v=a+2;
            if(!*v && ++i<argc) v=argv[i];
            if(!*v || v[1]) goto usage;
            delim=*v;
        } else if(a[0]=='-' && a[1]=='f') {
            spec=a+2;
            if(!*spec && ++i<argc) spec=argv[i];
            if(!*spec) goto usage;
        } else if(!path && a[0]!='-') path=a;
        else goto usage;
    }
    if(!spec || fields_parse(spec,selected)<0) goto usage;
    int fd=path?open(path,O_RDONLY):0;
    if(fd<0) {puts("cut: cannot open");return 1;}
    char line[CUT_LINE],input[64];
    unsigned used=0;
    int error=0,n;
    while((n=read(fd,input,sizeof input))>0) {
        for(int i=0;i<n;i++) {
            if(input[i]=='\n') {
                if(emit_line(line,used,delim,selected,1)<0) {error=1;goto done;}
                used=0;
            } else if(used<sizeof line) line[used++]=input[i];
            else {puts("cut: line too long");error=1;goto done;}
        }
    }
    if(n<0) error=1;
    if(!error && used && emit_line(line,used,delim,selected,0)<0) error=1;
done:
    if(path) close(fd);
    if(error) puts("cut: read/write error");
    return error?1:0;
usage:
    puts("usage: cut [-d CHAR] -f LIST [FILE]");
    puts("LIST: columns 1..64, comma-separated or ranges (e.g. 1,3-5)");
    return 2;
}
