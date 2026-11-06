/* SPDX-License-Identifier: GPL-3.0-or-later */
/* ファイル内容をコピーする． */

/* カーネルやバッファ数を増やさずにRAMファイルを一つコピーする． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"

/* 自分自身をtruncateしないよう，コピー元とコピー先の相対パスを正規化する． */
static int path(const char *src,char out[128]) {
    char input[256],cwd[128];
    unsigned n=0,o=1;
    if(*src!='/') {
        if(!getcwd(cwd,sizeof cwd))
            return -1;
        for(const char *p=cwd;*p;p++) {if(n>=sizeof input-2)return -1;input[n++]=*p;}
        if(!n || input[n-1]!='/')
            input[n++]='/';
    }
    for(const char *p=src;*p;p++) {if(n>=sizeof input-1)return -1;input[n++]=*p;}
    input[n]=0;out[0]='/';out[1]=0;
    for(unsigned i=0;i<n;) {
        while(input[i]=='/')i++;
        if(!input[i])break;
        unsigned start=i;
        while(input[i] && input[i]!='/')i++;
        unsigned len=i-start;
        if(len==1 && input[start]=='.')continue;
        if(len==2 && input[start]=='.' && input[start+1]=='.') {
            if(o>1) {while(o>1 && out[o-1]!='/')o--;if(o>1)o--;}
            out[o]=0;continue;
        }
        if(o>1) {if(o>=127)return -1;out[o++]='/';}
        if(o+len>=128)return -1;
        memcpy(out+o,input+start,len);o+=len;out[o]=0;
    }
    /* コピー可能なのは/tmpと/etcの直下のRAMファイルである． */
    const char *name=0;
    if(!strncmp(out,"/tmp/",5))
        name=out+5;
    else if(!strncmp(out,"/etc/",5)) name=out+5;
    return (name && *name && !strchr(name,'/'))?0:-1;
}

/* 指定したファイルの内容を別ファイルへ複写する．VFS経由で読み書きする． */
int main(int argc,char **argv) {
    char src[128],dst[128],buf[64];
    if(argc!=3) {puts("usage: cp SRC DST");return 1;}
    if(path(argv[1],src)<0 || path(argv[2],dst)<0) {
        puts("cp: use /tmp or /etc RAM files");return 1;
    }
    if(!strcmp(src,dst)) {puts("cp: source and destination are the same");return 1;}
    int in=open(src,O_RDONLY);
    if(in<0) {puts("cp: cannot open source");return 1;}
    int out=open(dst,O_WRONLY|O_CREAT|O_TRUNC);
    if(out<0) {puts("cp: cannot open destination");close(in);return 1;}
    int n,ok=1;
    while((n=read(in,buf,sizeof buf))>0) {
        int done=0;
        while(done<n) {
            int w=write(out,buf+done,(unsigned)(n-done));
            if(w<=0) {ok=0;break;}
            done+=w;
        }
        if(!ok)break;
    }
    if(n<0)ok=0;
    if(close(out)<0)ok=0;
    close(in);
    if(!ok)puts("cp: read/write error (destination may be incomplete)");
    return ok?0:1;
}
