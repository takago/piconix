/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Tiny full-screen /tmp editor: 1024-byte cap, adaptive terminal layout. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#define CAP 1024
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
/* Query cursor location at the lower-right corner (DSR). Unsupported terminals
 * simply use 80x24; never require a terminal-specific extension to edit. */
static void measure_screen(void){
 int flags=fcntl(0,F_GETFL,0);
 if(flags<0||fcntl(0,F_SETFL,flags|O_NONBLOCK)<0)return;
 printf("\033[999;999H\033[6n");
 int state=0,row=0,col=0;
 for(int t=0;t<20&&state!=6;t++){
  char c;
  while(read(0,&c,1)==1){
   if(state==0)state=c==27?1:0;
   else if(state==1)state=c=='['?2:0;
   else if(state==2){if(c>='0'&&c<='9'){row=row*10+c-'0';state=3;}else state=0;}
   else if(state==3){if(c>='0'&&c<='9')row=row*10+c-'0';else if(c==';')state=4;else state=0;}
   else if(state==4){if(c>='0'&&c<='9'){col=col*10+c-'0';state=5;}else state=0;}
   else if(state==5){if(c>='0'&&c<='9')col=col*10+c-'0';else if(c=='R')state=6;else state=0;}
   if(state==6)break;
  }
  if(state!=6)msleep(10);
 }
 fcntl(0,F_SETFL,flags);
 if(state==6&&row>=8&&col>=30){
  screen_rows=row>60?60:row;
  screen_cols=col>160?160:col;
 }
 visible_rows=screen_rows-4;
}
static void draw(const char *name,const char *notice){
 int line=line_no(pos),column=col_no(pos),text_cols=screen_cols-4;
 if(line<top)top=line;
 if(line>=top+visible_rows)top=line-visible_rows+1;
 if(column<left_col)left_col=column;
 if(column>=left_col+text_cols)left_col=column-text_cols+1;
 /* Clear and repaint the whole usable screen; no scrolling or stale text. */
 const char *base=name;
 for(const char *q=name;*q;q++)if(*q=='/')base=q+1;
 char short_name[21];int i=0;
 while(base[i]&&i<20){short_name[i]=base[i];i++;}short_name[i]=0;
 printf("\033[H\033[J\033[7mPicoNIX edit  %-20s  %d/1024 bytes %c\033[K\033[0m\r\n",
        short_name,len,changed?'*':' ');
 int p=0,n=0;
 while(p<len&&n<top){if(data[p++]=='\n')n++;}
 for(int row=0;row<visible_rows;row++){
  int number=top+row+1;
  if(number<=999)printf("\033[7m%03d \033[0m",number);
  else printf("\033[7m--- \033[0m");
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
  if(p<len&&data[p]=='\n')p++;
  printf("\033[K\r\n");
 }
 printf("\033[7m^S Save  ^Q Quit  Arrows Move  Enter Newline  TAB=4\033[K\033[0m\r\n");
 printf("\033[KLn %d Col %d  %s\033[K",line+1,column+1,notice?notice:"");
 int y=line-top+2,x=column-left_col+5;
 if(y<2)y=2;if(y>visible_rows+1)y=visible_rows+1;
 if(x<5)x=5;if(x>screen_cols)x=screen_cols;
 printf("\033[%d;%dH",y,x);
}
int main(int argc,char **argv){
 if(argc!=2){puts("usage: edit file");return 1;}
 /* Resolve relative names before accessing the virtual filesystem. */
 char path[128],cwd[128];
 if(argv[1][0]=='/') {
  if(strlen(argv[1])>=sizeof(path)){puts("edit: path too long");return 1;}
  strcpy(path,argv[1]);
 } else {
  if(!getcwd(cwd,sizeof(cwd))){puts("edit: cannot get cwd");return 1;}
  unsigned a=(unsigned)strlen(cwd),b=(unsigned)strlen(argv[1]);
  if(a+b+2>sizeof(path)){puts("edit: path too long");return 1;}
  strcpy(path,cwd);
  if(a && path[a-1]!='/'){path[a++]='/';path[a]=0;}
  strcpy(path+a,argv[1]);
 }
 if(strncmp(path,"/tmp/",5)!=0 || !path[5] || strchr(path+5,'/')){
  puts("edit: only files in /tmp supported");return 1;
 }
 int fd=open(path,O_RDWR|O_CREAT);
 if(fd<0){puts("edit: cannot open file");return 1;}
 int n=read(fd,data,CAP);close(fd);
 if(n<0){puts("edit: cannot read file");return 1;}
 len=n;
 int canon=ioctl(0,OS_TTY_GET_CANON,0),echo=ioctl(0,OS_TTY_GET_ECHO,0);
 if(canon<0||echo<0){puts("edit: terminal required");return 1;}
 if(ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0){
  ioctl(0,OS_TTY_SET_ECHO,echo);ioctl(0,OS_TTY_SET_CANON,canon);
  puts("edit: terminal required");return 1;
 }
 printf("\033[?1049h\033[?25h\033[2J");
 measure_screen();
 int esc=0,quit=0;const char *notice="";draw(path,notice);
 while(!quit){char c;if(read(0,&c,1)!=1)break;notice="";
  if(esc==1){esc=c=='['?2:0;continue;}
  if(esc==2){esc=0;int col=col_no(pos);if(c=='A'&&line_start(pos)>0)pos=position_col(line_start(line_start(pos)-1),col);
   else if(c=='B'&&line_end(pos)<len)pos=position_col(line_end(pos)+1,col);
   else if(c=='D'&&pos>0)pos--;
   else if(c=='C'&&pos<len)pos++;
   else if(c=='3'){esc=3;continue;}
  }else if(esc==3){esc=0;if(c=='~')del();}
  else if(c==27){esc=1;continue;}
  else if(c==3){quit=1;continue;} /* Ctrl+C: leave without saving; restore TTY below. */
  else if(c==17){if(!changed)quit=1;else {notice="Unsaved changes: Ctrl+S then Ctrl+Q";}}
  else if(c==19){notice=save(path)==0?"Saved":"Save failed";}
  else if(c==127||c==8)erase();
  else if(c==13||c==10)insert('\n');
  else if(c=='\t'||(c>=32&&c<127))insert(c);
  draw(path,notice);
 }
 printf("\033[0m\033[?1049l\033[?25h");
 return 0;
}
