/* PicoNIX read-only virtual directory traversal. No new syscalls. */
#ifndef PICONIX_WALK_H
#define PICONIX_WALK_H
#include "unistd.h"
#include "stdio.h"
#include "string.h"

#define WALK_PATH 128
#define WALK_DEPTH 8
/* callback receives an entry name without trailing slash */
typedef void (*walk_visit)(const char *, int, void *);
static int walk_eq(const char *a,const char *b){return strcmp(a,b)==0;}
static int walk_normalize(const char *input,char out[WALK_PATH]) {
 char cwd[WALK_PATH],src[256];unsigned used=0,len=1;
 if(!getcwd(cwd,sizeof cwd))return -1;
 if(*input!='/'){
  for(const char *t=cwd;*t;t++){if(used>=sizeof src-2)return -1;src[used++]=*t;}
  if(!used||src[used-1]!='/')src[used++]='/';
 }
 for(const char *t=input;*t;t++){if(used>=sizeof src-1)return -1;src[used++]=*t;}
 src[used]=0;out[0]='/';out[1]=0;
 for(unsigned i=0;i<used;){
  while(src[i]=='/')i++;
  if(!src[i])break;
  unsigned j=i;while(src[i]&&src[i]!='/')i++;
  unsigned n=i-j;
  if(n==1&&src[j]=='.')continue;
  if(n==2&&src[j]=='.'&&src[j+1]=='.'){
   if(len>1){while(len>1&&out[len-1]!='/')len--;if(len>1)len--;out[len]=0;}
   continue;
  }
  if(len>1){if(len+1>=WALK_PATH)return -1;out[len++]='/';}
  if(len+n>=WALK_PATH)return -1;
  memcpy(out+len,src+j,n);len+=n;out[len]=0;
 }
 return 0;
}
static int walk_dir(const char *p){
 int fd=open(p,0);if(fd<0)return 0;
 piconix_dirent_t entry;int rc=getdents(fd,&entry,sizeof entry);
 close(fd);return rc>=0;
}
static int walk_entries(const char *p,walk_visit visit,void *arg){
 int fd=open(p,0);if(fd<0)return -1;
 piconix_dirent_t entry;int rc;
 while((rc=getdents(fd,&entry,sizeof entry))>0)visit(entry.name,entry.is_dir,arg);
 close(fd);return rc<0?-1:0;
}
static int walk_join(const char *parent,const char *name,char out[WALK_PATH]){
 unsigned a=(unsigned)strlen(parent),b=(unsigned)strlen(name);
 if(a+b+(a>1?1:0)>=WALK_PATH)return -1;
 memcpy(out,parent,a);
 if(a>1)out[a++]='/';
 memcpy(out+a,name,b+1);return 0;
}
#endif
