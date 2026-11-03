/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "walk.h"
#include "stdlib.h"
static int max_depth=8;
static void visit(const char *name,int isdir,void *ctx);
typedef struct {char path[WALK_PATH];int depth;unsigned mask;} State;
static void traverse(const char *path,int depth,unsigned mask){
 if(depth>=max_depth)return;
 State s;strncpy(s.path,path,sizeof s.path-1);s.path[sizeof s.path-1]=0;
 s.depth=depth;s.mask=mask;
 walk_entries(path,visit,&s);
}
static void visit(const char *name,int isdir,void *ctx){
 State *s=ctx;char path[WALK_PATH];
 if(walk_join(s->path,name,path)<0)return;
 for(int i=0;i<s->depth;i++)printf("  ");
 printf("|-- %s%s\n",name,isdir?"/":"");
 if(isdir)traverse(path,s->depth+1,s->mask);
}
int main(int argc,char **argv){
 const char *root=".";int got=0;
 for(int i=1;i<argc;i++){
  if(!strcmp(argv[i],"-L")&&i+1<argc){
   char *end;long n=strtol(argv[++i],&end,10);
   if(*end||n<0||n>WALK_DEPTH){puts("tree: depth must be 0..8");return 1;}max_depth=(int)n;
  }else if(argv[i][0]=='-'||got++){puts("usage: tree [-L depth] [path]");return 1;}
  else root=argv[i];
 }
 char path[WALK_PATH];if(walk_normalize(root,path)<0){puts("tree: invalid path");return 1;}
 printf("%s\n",path);
 if(!walk_dir(path)){puts("tree: not a directory");return 1;}
 traverse(path,0,0);return 0;
}
