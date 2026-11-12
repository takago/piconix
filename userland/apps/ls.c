/* SPDX-License-Identifier: GPL-3.0-or-later */
/* ディレクトリの内容を列挙する． */

#include "unistd.h"
#include "stdio.h"
#include "string.h"
/* 二つのファイル名・オプション文字列が等しいか判定する． */
static int eq(const char *a,const char *b) {return strcmp(a,b)==0;}

/* 標準出力への全バイトの書込みを行う．部分書込みを処理する． */
static void write_all(int fd,const char *p,size_t n) {while(n) {int k=write(fd,p,n);if(k<0)return;if(!k){msleep(1);continue;}p+=k;n-=k;}}

/* 表示文字列を標準出力へ出力する． */
static void put(const char *s) {write_all(1,s,strlen(s));}

/* 数字や欠損値を幅に合わせて右寄せ表示する． */
static void right_field(const char *value,unsigned width) {
    unsigned len=(unsigned)strlen(value);
    while(len<width) {put(" ");len++;}
    put(value);
}

/* 相対パスを現在ディレクトリからのパスへ変換する． */
static int resolve(const char *cwd, const char *input, char out[128]) {
    char combined[256];
    unsigned n=0;
    if(*input!='/') {
        for(const char *q=cwd;*q;q++) { if(n>=sizeof(combined)-2) return -1; combined[n++]=*q; }
        if(n==0 || combined[n-1]!='/')
            combined[n++]='/';
    }
    for(const char *q=input;*q;q++) { if(n>=sizeof(combined)-1) return -1; combined[n++]=*q; }
    combined[n]=0;
    unsigned o=1; out[0]='/'; out[1]=0;
    for(unsigned i=0;i<n;) {
        while(combined[i]=='/')
            i++;
        if(!combined[i])
            break;
        unsigned start=i;
        while(combined[i] && combined[i]!='/')
            i++;
        unsigned len=i-start;
        if(len==1 && combined[start]=='.')
            continue;
        if(len==2 && combined[start]=='.' && combined[start+1]=='.') {
            if(o>1) { while(o>1 && out[o-1]!='/') o--; if(o>1) o--; }
            out[o]=0;
            continue;
        }
        if(o>1) { if(o>=127) return -1; out[o++]='/'; }
        if(o+len>=128)
            return -1;
        for(unsigned k=0;k<len;k++)
            out[o++]=combined[start+k];
        out[o]=0;
    }
    return 0;
}
static int detailed, show_all;
/* デバイスはデバイスID，/binはプログラムID，RAMファイルはスロットIDを表示する． */
static void entry(const char *directory,const char *name,int directory_entry,unsigned id) {
    if(detailed) {
        const char *type=directory_entry?"d":
            eq(directory,"/bin") || eq(directory,"/userland")?"p":
            eq(directory,"/tmp") || eq(directory,"/etc")?"f":"c";
        char id_text[16],size_text[16];
        if(id!=65535u)snprintf(id_text,sizeof id_text,"%u",id);
        else strcpy(id_text,"-");
        strcpy(size_text,"-");
        if(!directory_entry && eq(directory,"/tmp")) {
            char path[128];
            snprintf(path,sizeof path,"/tmp/%s",name);
            int fd=open(path,0);
            if(fd>=0) {
                char buf[64];int n;unsigned size=0;
                while((n=read(fd,buf,sizeof buf))>0)size+=(unsigned)n;
                close(fd);
                if(n>=0)snprintf(size_text,sizeof size_text,"%u",size);
            }
        }
        put(type);put("    ");
        right_field(id_text,5);put(" ");
        right_field(size_text,6);put(" ");
    }
    put(name);put("\r\n");
}

/* 親ディレクトリからファイル属性を取得する．通常ファイルなら開かずに表示する． */
static int file_entry(const char *path) {
    const char *slash=0;
    for(const char *p=path;*p;p++)if(*p=='/')slash=p;
    if(!slash || !slash[1])return -1;
    char parent[128];
    size_t len=(size_t)(slash-path);
    if(len==0) {parent[0]='/';parent[1]=0;}
    else {if(len>=sizeof parent)return -1;memcpy(parent,path,len);parent[len]=0;}
    int fd=open(parent,0);
    if(fd<0)return -1;
    piconix_dirent_t e;
    int n,result=-1;
    while((n=getdents(fd,&e,sizeof e))>0) {
        if(!strcmp(e.name,slash+1)) {
            if(e.is_dir) result=0;
            else { entry(parent,e.name,0,e.device_id); result=1; }
            break;
        }
    }
    close(fd);
    return result;
}

/* ディレクトリなら内容を，通常ファイルならそのファイル自身を表示する． */
static void ls(const char *path) {
    /* /dev/ptmxのopenには副作用があるため，ファイルは先に親の一覧から調べる． */
    int is_file=0;
    if(!eq(path,"/")) {
        /* 詳細表示の見出しは一度だけ出す． */
        if(detailed)put("TYPE    ID   SIZE NAME\r\n");
        is_file=file_entry(path);
        if(is_file==1)return;
    }
    int fd=open(path,0);
    if(fd<0) {put("ls: cannot open\r\n");return;}
    piconix_dirent_t e;
    int n=getdents(fd,&e,sizeof e);
    if(n<0) {
        close(fd);
        if(is_file<0)put("ls: cannot stat file\r\n");
        return;
    }
    if(detailed && eq(path,"/"))put("TYPE    ID   SIZE NAME\r\n");
    do {
        if(n>0 && (show_all || e.name[0]!='.')) {
            char name[40];snprintf(name,sizeof name,"%s%s",e.name,e.is_dir?"/":"");
            entry(path,name,e.is_dir,e.device_id);
        }
    } while((n=getdents(fd,&e,sizeof e))>0);
    close(fd);
    if(n<0)put("ls: cannot read directory\r\n");
    if(eq(path,"/bin")) {put("\r\n---- sh built-in commands ----\r\n");
        entry(path,"help",0,65535);entry(path,"logout",0,65535);entry(path,"exit",0,65535);
        entry(path,"cd",0,65535);entry(path,"getty",0,65535);}
}

/* 指定パスの一覧を表示し，オプションに従って表示形式を切り替える． */
int main(int argc,char *argv[]) {
    char cwd[128], path[128];
    const char *arg=".";
    int i=1;
    if(i<argc && argv[i][0]=='-') {
        for(const char *a=argv[i]+1;*a;a++) {
            if(*a=='l')
                detailed=1;
            else if(*a=='a')show_all=1;
            else {puts("usage: ls [-la] [PATH]");return 1;}
        }
        i++;
    }
    if(i<argc)arg=argv[i++];
    if(i!=argc){puts("usage: ls [-la] [PATH]");return 1;}
    if(!getcwd(cwd,sizeof cwd)) {puts("ls: cannot get cwd");return 1;}
    if(resolve(cwd,arg,path)<0) {puts("ls: path too long");return 1;}
    ls(path);
    return 0;
}
