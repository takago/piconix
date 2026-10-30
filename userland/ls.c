/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "unistd.h"
#include "stdio.h"
#include "string.h"
static int eq(const char *a,const char *b) {return strcmp(a,b)==0;}
static void write_all(int fd,const char *p,size_t n) {while(n) {int k=write(fd,p,n);if(k<0)return;if(!k){msleep(1);continue;}p+=k;n-=k;}}
static void put(const char *s) {write_all(1,s,strlen(s));}
static void number(int v) {char b[16];snprintf(b,sizeof b,"%d",v);put(b);}
static int resolve(const char *cwd, const char *input, char out[128]) {
    char combined[256];
    unsigned n=0;
    if(*input!='/') {
        for(const char *q=cwd;*q;q++) { if(n>=sizeof(combined)-2) return -1; combined[n++]=*q; }
        if(n==0 || combined[n-1]!='/') combined[n++]='/';
    }
    for(const char *q=input;*q;q++) { if(n>=sizeof(combined)-1) return -1; combined[n++]=*q; }
    combined[n]=0;
    unsigned o=1; out[0]='/'; out[1]=0;
    for(unsigned i=0;i<n;) {
        while(combined[i]=='/') i++;
        if(!combined[i]) break;
        unsigned start=i;
        while(combined[i] && combined[i]!='/') i++;
        unsigned len=i-start;
        if(len==1 && combined[start]=='.') continue;
        if(len==2 && combined[start]=='.' && combined[start+1]=='.') {
            if(o>1) { while(o>1 && out[o-1]!='/') o--; if(o>1) o--; }
            out[o]=0;
            continue;
        }
        if(o>1) { if(o>=127) return -1; out[o++]='/'; }
        if(o+len>=128) return -1;
        for(unsigned k=0;k<len;k++) out[o++]=combined[start+k];
        out[o]=0;
    }
    return 0;
}
static int detailed;
/* File sizes are available for writable /tmp files only; other nodes are virtual. */
static void entry(const char *directory,const char *name,int directory_entry) {
    if(detailed) {
        if(directory_entry)put("d    -  ");
        else if(eq(directory,"/tmp")) {
            char path[128];
            snprintf(path,sizeof path,"/tmp/%s",name);
            int fd=open(path,0);
            if(fd>=0) {
                char buf[64];int n;unsigned size=0;
                while((n=read(fd,buf,sizeof buf))>0)size+=(unsigned)n;
                close(fd);
                if(n>=0){put("- ");number((int)size);put("  ");}
                else put("-    -  ");
            } else put("-    -  ");
        } else put("-    -  ");
    }
    put(name);put("\r\n");
}
static void ls(const char *path) {
 int fd=open(path,0);if(fd<0){put("ls: cannot open directory\r\n");return;}
 piconix_dirent_t e;int n;
 while((n=getdents(fd,&e,sizeof e))>0) {
     char name[40];snprintf(name,sizeof name,"%s%s",e.name,e.is_dir?"/":"");
     entry(path,name,e.is_dir);
 }
 close(fd);
 if(n<0)put("ls: not a directory\r\n");
 if(eq(path,"/bin")) {put("\r\n---- sh built-in commands ----\r\n");
     entry(path,"help",0);entry(path,"logout",0);entry(path,"exit",0);
     entry(path,"cd",0);entry(path,"getty",0);}
}
int main(int argc,char *argv[]) {
    char cwd[128], path[128];
    const char *arg=".";
    if(argc>=2 && eq(argv[1],"-l")) {detailed=1;if(argc==3)arg=argv[2];else if(argc>3){puts("usage: ls [-l] [PATH]");return 1;}}
    else if(argc==2)arg=argv[1];
    else if(argc>2){puts("usage: ls [-l] [PATH]");return 1;}
    if(!getcwd(cwd,sizeof cwd)) {puts("ls: cannot get cwd");return 1;}
    if(resolve(cwd,arg,path)<0) {puts("ls: path too long");return 1;}
    ls(path);
    return 0;
}
