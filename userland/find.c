/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "walk.h"
#include "stdlib.h"
static const char *pattern=0;
static int match(const char *s,const char *p){
 while(*p){
  if(*p=='*'){p++;if(!*p)return 1;do{if(match(s,p))return 1;}while(*s++);return 0;}
  if(*p=='?'){if(!*s)return 0;s++;p++;continue;}
  if(*s++!=*p++)return 0;
 }
 return !*s;
}
typedef struct {char path[WALK_PATH];int depth;} State;
static void descend(const char *,int);
static void visit(const char *name,int isdir,void *ctx){
 State *s=ctx;char path[WALK_PATH];
 if(walk_join(s->path,name,path)<0)return;
 if(!pattern||match(name,pattern))puts(path);
 if(isdir)descend(path,s->depth+1);
}
static void descend(const char *path,int depth){
 if(depth>=WALK_DEPTH)return;
 State s;strncpy(s.path,path,sizeof s.path-1);s.path[sizeof s.path-1]=0;s.depth=depth;
 walk_entries(path,visit,&s);
}
int main(int argc,char **argv){
 const char *root=".";int i=1;
 if(i<argc&&strcmp(argv[i],"-name"))root=argv[i++];
 if(i<argc){if(strcmp(argv[i],"-name")||i+1>=argc||i+2!=argc){puts("usage: find [path] [-name pattern]");return 1;}pattern=argv[i+1];}
 char path[WALK_PATH];if(walk_normalize(root,path)<0){puts("find: invalid path");return 1;}
 const char *base=path;for(const char *q=path;*q;q++)if(*q=='/')base=q+1;
 if(!pattern||match(*base?base:"/",pattern))puts(path);
 if(walk_dir(path))descend(path,0);
 else {int fd=open(path,0);if(fd<0){puts("find: path not found");return 1;}close(fd);}
 return 0;
}
