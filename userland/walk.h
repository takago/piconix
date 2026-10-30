/* PicoNIX read-only virtual directory traversal. No new syscalls. */
#ifndef PICONIX_WALK_H
#define PICONIX_WALK_H
#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "../kernel/pins.h"
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
 return walk_eq(p,"/")||walk_eq(p,"/bin")||walk_eq(p,"/tmp")||
 walk_eq(p,"/dev")||walk_eq(p,"/dev/gpio")||walk_eq(p,"/dev/adc")||
 walk_eq(p,"/dev/pwm")||(pin_any_pwm()&&walk_eq(p,"/dev/pwm/1"))||
 walk_eq(p,"/proc")||walk_eq(p,"/proc/task")||walk_eq(p,"/proc/sym")||
 walk_eq(p,"/proc/sym/userland");
}
static void walk_emit(walk_visit visit,void *arg,const char *name,int dir){visit(name,dir,arg);}
/* Read newline-delimited directory entries. */
static int walk_stream(const char *source,int dir,walk_visit visit,void *arg){
 int fd=open(source,0);if(fd<0)return -1;
 char name[64];unsigned used=0;char c;
 while(read(fd,&c,1)==1){
  if(c=='\n'){
   name[used]=0;
   if(used){if(name[used-1]=='/'){name[--used]=0;walk_emit(visit,arg,name,1);}else walk_emit(visit,arg,name,dir);}
   used=0;
  }else if(used+1<sizeof name)name[used++]=c;
 }
 if(used){name[used]=0;walk_emit(visit,arg,name,dir);}
 close(fd);return 0;
}
static int walk_entries(const char *p,walk_visit visit,void *arg){
 if(walk_eq(p,"/")){
  walk_emit(visit,arg,"bin",1);walk_emit(visit,arg,"dev",1);
  walk_emit(visit,arg,"proc",1);walk_emit(visit,arg,"tmp",1);return 0;
 }
 if(walk_eq(p,"/bin")||walk_eq(p,"/tmp"))return walk_stream(p,0,visit,arg);
 if(walk_eq(p,"/dev"))return walk_stream("/proc/devices",0,visit,arg);
 if(walk_eq(p,"/proc")){
  walk_emit(visit,arg,"uptime",0);
  walk_emit(visit,arg,"ticks",0);
  walk_emit(visit,arg,"interrupts",0);
  walk_emit(visit,arg,"context_switches",0);
  walk_emit(visit,arg,"memory",0);
  walk_emit(visit,arg,"ramfiles",0);
  walk_emit(visit,arg,"devices",0);
  walk_emit(visit,arg,"tasks",0);
  walk_emit(visit,arg,"fb",0);
  walk_emit(visit,arg,"task",1);walk_emit(visit,arg,"sym",1);return 0;
 }
 if(walk_eq(p,"/proc/sym")){
  walk_emit(visit,arg,"kernel",0);walk_emit(visit,arg,"userland",1);return 0;
 }
 if(walk_eq(p,"/proc/sym/userland"))return walk_stream("/bin",0,visit,arg);
 if(walk_eq(p,"/dev/gpio")){
  walk_emit(visit,arg,"fb",0);
  for(unsigned i=0;i<=28;i++)if(pin_gpio(i)){
   char name[20];snprintf(name,sizeof name,"%u",i);walk_emit(visit,arg,name,0);
   snprintf(name,sizeof name,"%u.dir",i);walk_emit(visit,arg,name,0);
   snprintf(name,sizeof name,"%u.pull",i);walk_emit(visit,arg,name,0);
  }
  return 0;
 }
 if(walk_eq(p,"/dev/adc")){
  for(unsigned i=0;i<3;i++)if(pin_adc(i)){
   char name[8];snprintf(name,sizeof name,"%u",i);walk_emit(visit,arg,name,0);
  }
  walk_emit(visit,arg,"temp",0);return 0;
 }
 if(walk_eq(p,"/dev/pwm")){
  if(pin_any_pwm())walk_emit(visit,arg,"1",1);return 0;
 }
 if(walk_eq(p,"/dev/pwm/1")){
  if(pin_any_pwm())walk_emit(visit,arg,"freq",0);
  if(pin_pwm(18))walk_emit(visit,arg,"a",0);
  if(pin_pwm(19))walk_emit(visit,arg,"b",0);return 0;
 }
 if(walk_eq(p,"/proc/task")){
  /* /proc/tasks lines: PID CMD ... ; accept leading whitespace. */
  int fd=open("/proc/tasks",0);if(fd<0)return -1;
  char line[120],c;unsigned used=0;
  while(read(fd,&c,1)==1){
   if(c!='\n'){if(used+1<sizeof line)line[used++]=c;continue;}
   line[used]=0;used=0;
   char *q=line;while(*q==' '||*q=='\t')q++;
   if(*q>='0'&&*q<='9'){
    char num[16];unsigned n=0;
    while(*q>='0'&&*q<='9'&&n+1<sizeof num)num[n++]=*q++;
    num[n]=0;walk_emit(visit,arg,num,0);
   }
  }
  close(fd);return 0;
 }
 return -1;
}
static int walk_join(const char *parent,const char *name,char out[WALK_PATH]){
 unsigned a=(unsigned)strlen(parent),b=(unsigned)strlen(name);
 if(a+b+(a>1?1:0)>=WALK_PATH)return -1;
 memcpy(out,parent,a);
 if(a>1)out[a++]='/';
 memcpy(out+a,name,b+1);return 0;
}
#endif
