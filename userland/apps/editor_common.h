/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Common RAM-file editing and terminal rendering used by edit and vi.
 * Each program includes this header in its own PIC image. */
#ifndef PICONIX_EDITOR_COMMON_H
#define PICONIX_EDITOR_COMMON_H
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include "../lib/screen.h"

#define CAP 2048
#define DEFAULT_ROWS 24
#define DEFAULT_COLS 80
static int screen_rows=DEFAULT_ROWS,screen_cols=DEFAULT_COLS;
static int visible_rows=DEFAULT_ROWS-4, left_col;
static char data[CAP];
static int len,pos,top,changed;

static int width(char c,int col) {return c=='\t'?4-col%4:1;}
static int line_start(int p){while(p>0&&data[p-1]!='\n')p--;return p;}
static int line_end(int p){while(p<len&&data[p]!='\n')p++;return p;}
static int line_no(int p){int n=0;for(int i=0;i<p;i++)if(data[i]=='\n')n++;return n;}
static int col_no(int p){int c=0;for(int i=line_start(p);i<p;i++)c+=width(data[i],c);return c;}
static int position_col(int p,int col){int e=line_end(p),x=0;while(p<e&&x<col){int w=width(data[p],x);if(x+w>col)break;x+=w;p++;}return p;}
static void insert(char c){if(len>=CAP)return;memmove(data+pos+1,data+pos,len-pos);data[pos++]=c;len++;changed=1;}
static void erase(void){if(!pos)return;memmove(data+pos-1,data+pos,len-pos);pos--;len--;changed=1;}
static void del(void){if(pos>=len)return;memmove(data+pos,data+pos+1,len-pos-1);len--;changed=1;}
static int save(const char *name){int fd=open(name,O_WRONLY|O_CREAT|O_TRUNC);if(fd<0)return -1;int n=write(fd,data,len);close(fd);if(n==len){changed=0;return 0;}return -1;}

/* Keep the editor's output bounded even on extremely large terminals. */
static void measure_screen(void){
 screen_size_t size={DEFAULT_ROWS,DEFAULT_COLS};
 screen_measure(&size);
 screen_rows=size.rows>60?60:size.rows;
 screen_cols=size.cols>160?160:size.cols;
 visible_rows=screen_rows-4;
}

/* EDITOR_GUTTER is 4 for edit's line numbers, 0 for vi. */
static void paint_row(int row){
 int p=0,n=0,text_cols=screen_cols-EDITOR_GUTTER;
 while(p<len&&n<top+row){if(data[p++]=='\n')n++;}
 printf("\033[%d;1H",row+2);
#if EDITOR_GUTTER
 int number=top+row+1;
 if(number<=999)printf("\033[7m%03d \033[0m",number);
 else printf("\033[7m--- \033[0m");
#endif
 int c=0;
 while(p<len&&data[p]!='\n'){
  char ch=data[p++];int w=width(ch,c);
  for(int i=0;i<w;i++){
   if(c>=left_col&&c<left_col+text_cols){
    char shown=ch=='\t'?' ':(ch>=32&&ch<127?ch:'.');
    putchar(i==0?shown:' ');
   }
   c++;
  }
 }
 printf("\033[K");
}
/* mode: 0 = full, 1 = changed row, 2 = cursor/status only. */
static void draw(const char *name,const char *notice,int mode){
 int line=line_no(pos),column=col_no(pos),text_cols=screen_cols-EDITOR_GUTTER;
 int old_top=top,old_left=left_col;
 if(line<top)top=line;
 if(line>=top+visible_rows)top=line-visible_rows+1;
 if(column<left_col)left_col=column;
 if(column>=left_col+text_cols)left_col=column-text_cols+1;
 if(old_top!=top||old_left!=left_col)mode=0;
 printf("\033[?25l");
 if(mode!=2){
  const char *base=name;
  for(const char *q=name;*q;q++)if(*q=='/')base=q+1;
  char short_name[21];int i=0;
  while(base[i]&&i<20){short_name[i]=base[i];i++;}short_name[i]=0;
  printf("\033[1;1H\033[7m" EDITOR_TITLE "%-20s  %d/2048 bytes %c\033[K\033[0m",short_name,len,changed?'*':' ');
  if(mode==0){
   for(int row=0;row<visible_rows;row++)paint_row(row);
   printf("\033[%d;1H\033[7m" EDITOR_HINT "\033[K\033[0m",visible_rows+2);
  }else if(line>=top&&line<top+visible_rows)paint_row(line-top);
 }
 printf("\033[%d;1H\033[KLn %d Col %d  %s\033[K",visible_rows+3,line+1,column+1,notice?notice:"");
 int y=line-top+2,x=column-left_col+EDITOR_GUTTER+1;
 if(y<2)y=2;
 if(y>visible_rows+1)y=visible_rows+1;
 if(x<EDITOR_GUTTER+1)x=EDITOR_GUTTER+1;
 if(x>screen_cols)x=screen_cols;
 printf("\033[%d;%dH\033[?25h",y,x);
}

/* Normalize . and .. before checking the writable RAM-file paths. */
static void editor_normalize_path(char path[128]){
 char normalized[128];unsigned out=0;
 for(const char *p=path;*p;){
  while(*p=='/')p++;
  if(!*p)break;
  const char *part=p;
  while(*p&&*p!='/')p++;
  unsigned n=(unsigned)(p-part);
  if(n==1&&part[0]=='.')continue;
  if(n==2&&part[0]=='.'&&part[1]=='.'){
   while(out&&normalized[out-1]!='/')out--;
   if(out)out--;
   continue;
  }
  normalized[out++]='/';
  for(unsigned i=0;i<n;i++)normalized[out++]=part[i];
 }
 if(!out)normalized[out++]='/';
 normalized[out]=0;
 strcpy(path,normalized);
}

/* Path resolution, strict 2-KiB read, and TTY setup are identical for both.
 * The kernel restores foreground TTY modes after process termination. */
static int editor_start(const char *arg, char path[128]){
 char cwd[128];
 if(arg[0]=='/'){
  if(strlen(arg)>=128){puts(EDITOR_NAME ": path too long");return -1;}
  strcpy(path,arg);
 }else{
  if(!getcwd(cwd,sizeof(cwd))){puts(EDITOR_NAME ": cannot get cwd");return -1;}
  unsigned a=(unsigned)strlen(cwd),b=(unsigned)strlen(arg);
  if(a+b+2>128){puts(EDITOR_NAME ": path too long");return -1;}
  strcpy(path,cwd);
  if(a&&path[a-1]!='/'){path[a++]='/';path[a]=0;}
  strcpy(path+a,arg);
 }
 editor_normalize_path(path);
 if((strncmp(path,"/tmp/",5)&&strncmp(path,"/etc/",5))||
    !path[5]||strchr(path+5,'/')){
  puts(EDITOR_NAME ": only files in /tmp or /etc supported");return -1;
 }
 int fd=open(path,O_RDWR|O_CREAT);
 if(fd<0){puts(EDITOR_NAME ": cannot open file");return -1;}
 int n=read(fd,data,CAP);
 if(n<0){close(fd);puts(EDITOR_NAME ": cannot read file");return -1;}
 if(n==CAP){
  char extra;int more=read(fd,&extra,1);
  if(more!=0){close(fd);puts(EDITOR_NAME ": file too large or read error");return -1;}
 }
 close(fd);
 len=n;
 int canon=ioctl(0,OS_TTY_GET_CANON,0),echo=ioctl(0,OS_TTY_GET_ECHO,0);
 if(canon<0||echo<0){puts(EDITOR_NAME ": terminal required");return -1;}
 if(ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0){
  ioctl(0,OS_TTY_SET_ECHO,echo);ioctl(0,OS_TTY_SET_CANON,canon);
  puts(EDITOR_NAME ": terminal required");return -1;
 }
 screen_enter();
 measure_screen();
 return 0;
}
#endif
