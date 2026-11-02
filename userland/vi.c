/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Tiny full-screen /tmp editor: 2048-byte cap, adaptive terminal layout. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
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
/* Redraw only what changed. Full refresh is needed for scrolling and
 * changes to the number of lines; ordinary edits repaint just one row. */
static void paint_row(int row){
 int p=0,n=0,text_cols=screen_cols;
 while(p<len&&n<top+row){if(data[p++]=='\n')n++;}
 printf("\033[%d;1H",row+2);
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
/* mode: 0 = full, 1 = edited row, 2 = cursor/status only. */
static void draw(const char *name,const char *notice,int mode){
 int line=line_no(pos),column=col_no(pos),text_cols=screen_cols;
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
  printf("\033[1;1H\033[7mPicoNIX vi    %-20s  %d/2048 bytes %c\033[K\033[0m",
         short_name,len,changed?'*':' ');
  if(mode==0){
   for(int row=0;row<visible_rows;row++)paint_row(row);
   printf("\033[%d;1H\033[7mESC Normal  i Insert  :w Save  :q Quit  h/j/k/l Move\033[K\033[0m",visible_rows+2);
  }else if(line>=top&&line<top+visible_rows)paint_row(line-top);
 }
 printf("\033[%d;1H\033[KLn %d Col %d  %s\033[K",visible_rows+3,line+1,column+1,notice?notice:"");
 int y=line-top+2,x=column-left_col+1;
 if(y<2)y=2;if(y>visible_rows+1)y=visible_rows+1;
 if(x<1)x=1;if(x>screen_cols)x=screen_cols;
 printf("\033[%d;%dH\033[?25h",y,x);
}


/* Read the remainder of an ANSI cursor-key sequence without blocking Esc.
 * A lone Esc exits insert mode; ESC [ A/B/C/D moves in either mode.
 * Unrelated characters following Esc are returned to the main input loop. */
static int poll_key(char *out){
 int flags=fcntl(0,F_GETFL,0);
 if(flags<0)return 0;
 if(fcntl(0,F_SETFL,flags|O_NONBLOCK)<0)return 0;
 int got=0;
 for(int i=0;i<5;i++){
  if(read(0,out,1)==1){got=1;break;}
  msleep(10);
 }
 fcntl(0,F_SETFL,flags);
 return got;
}
static void move_arrow(char c){
 int col=col_no(pos);
 if(c=='A'&&line_start(pos)>0)pos=position_col(line_start(line_start(pos)-1),col);
 else if(c=='B'&&line_end(pos)<len)pos=position_col(line_end(pos)+1,col);
 else if(c=='D'&&pos>line_start(pos))pos--;
 else if(c=='C'&&pos<line_end(pos))pos++;
}

int main(int argc,char **argv){
 if(argc!=2){puts("usage: vi file");return 1;}
 /* Resolve relative names before accessing the virtual filesystem. */
 char path[128],cwd[128];
 if(argv[1][0]=='/') {
  if(strlen(argv[1])>=sizeof(path)){puts("vi: path too long");return 1;}
  strcpy(path,argv[1]);
 } else {
  if(!getcwd(cwd,sizeof(cwd))){puts("vi: cannot get cwd");return 1;}
  unsigned a=(unsigned)strlen(cwd),b=(unsigned)strlen(argv[1]);
  if(a+b+2>sizeof(path)){puts("vi: path too long");return 1;}
  strcpy(path,cwd);
  if(a && path[a-1]!='/'){path[a++]='/';path[a]=0;}
  strcpy(path+a,argv[1]);
 }
 if(strncmp(path,"/tmp/",5)!=0 || !path[5] || strchr(path+5,'/')){
  puts("vi: only files in /tmp supported");return 1;
 }
 int fd=open(path,O_RDWR|O_CREAT);
 if(fd<0){puts("vi: cannot open file");return 1;}
 int n=read(fd,data,CAP);
 if(n<0){close(fd);puts("vi: cannot read file");return 1;}
 if(n==CAP){
  char extra;int more=read(fd,&extra,1);
  if(more!=0){close(fd);puts("vi: file too large or read error");return 1;}
 }
 close(fd);
 len=n;
 int canon=ioctl(0,OS_TTY_GET_CANON,0),echo=ioctl(0,OS_TTY_GET_ECHO,0);
 if(canon<0||echo<0){puts("vi: terminal required");return 1;}
 if(ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0){
  ioctl(0,OS_TTY_SET_ECHO,echo);ioctl(0,OS_TTY_SET_CANON,canon);
  puts("vi: terminal required");return 1;
 }
 printf("\033[?1049h\033[?25h\033[2J");
 measure_screen();
 int quit=0,insert_mode=0,pending_d=0;
 int queued=-1;
 const char *notice="NORMAL";draw(path,notice,0);
 while(!quit){
  char c;
  if(queued>=0){c=(char)queued;queued=-1;}
  else if(read(0,&c,1)!=1)break;
  int repaint=2;
  if(c==27){
   char next;
   if(poll_key(&next)){
    if(next=='['){
     char key;
     /* Cursor keys are three-byte CSI sequences on supported terminals. */
     if(poll_key(&key))move_arrow(key);
     draw(path,insert_mode?"INSERT":"NORMAL",2);
     continue;
    }
    queued=(unsigned char)next;
   }
   insert_mode=0;pending_d=0;
   draw(path,"NORMAL",2);
   continue;
  }
  if(c==3){quit=1;continue;}
  if(insert_mode){
   if(c==127||c==8){int old=len;erase();if(len!=old)repaint=0;}
   else if(c=='\n'||c=='\r'){insert('\n');repaint=0;}
   else if(c=='\t'||(c>=32&&c<127)){insert(c);repaint=1;}
   draw(path,"INSERT",repaint);continue;
  }
  if(c=='i'){insert_mode=1;notice="INSERT";}
  else if(c=='a'){if(pos<len&&data[pos]!='\n')pos++;insert_mode=1;notice="INSERT";}
  else if(c=='h'&&pos>line_start(pos))pos--;
  else if(c=='l'&&pos<len&&data[pos]!='\n')pos++;
  else if(c=='j'&&line_end(pos)<len)pos=position_col(line_end(pos)+1,col_no(pos));
  else if(c=='k'&&line_start(pos)>0)pos=position_col(line_start(line_start(pos)-1),col_no(pos));
  else if(c=='0')pos=line_start(pos);
  else if(c=='$')pos=line_end(pos);
  else if(c=='x'){int old=len;del();if(len!=old)repaint=1;}
  else if(c=='o'){pos=line_end(pos);insert('\n');insert_mode=1;repaint=0;notice="INSERT";}
  else if(c=='d'&&pending_d){int st=line_start(pos),en=line_end(pos);if(en<len)en++;
    memmove(data+st,data+en,len-en);len-=en-st;pos=st>len?len:st;changed=1;repaint=0;pending_d=0;}
  else if(c=='d'){pending_d=1;notice="d (press d again to delete line)";}
  else if(c==':'){
   char cmd[12];int n=0;printf("\033[%d;1H\033[K:",visible_rows+3);
   while(read(0,&c,1)==1&&c!='\r'&&c!='\n'&&n<11){
    if(c==27){n=0;break;}if(c==127||c==8){if(n){n--;printf("\b \b");}continue;}
    if(c>=32&&c<127){cmd[n++]=c;putchar(c);}
   }cmd[n]=0;
   if(!strcmp(cmd,"w")){notice=save(path)==0?"Saved":"Save failed";repaint=1;}
   else if(!strcmp(cmd,"q")){if(!changed)quit=1;else notice="Modified: :q! or :wq";}
   else if(!strcmp(cmd,"q!"))quit=1;
   else if(!strcmp(cmd,"wq")||!strcmp(cmd,"x")){if(save(path)==0)quit=1;else notice="Save failed";}
   else notice="Unknown command";
  }
  else pending_d=0;
  if(!quit)draw(path,notice,repaint);
 }
 printf("\033[0m\033[?1049l\033[?25h");
 return 0;
}
