/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 小容量RAMファイル用の簡易全画面エディタ． */

/* 上限2 KiBの簡易全画面RAMファイルエディタ．端末サイズに合わせて描画する． */
#define EDITOR_NAME "vi"
#define EDITOR_GUTTER 0
#define EDITOR_TITLE "PicoNIX vi    "
#define EDITOR_HINT "ESC Normal  i Insert  :w Save  :help Keys  h/j/k/l Move"
#include "editor_common.h"

/* ANSIカーソルキーの残りを読み出し，単独Escをブロックしない．Escは挿入モード終了，ESC [ A/B/C/Dは移動として扱う．無関係な文字は主入力ループへ戻す． */
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

/* エスケープシーケンスによるカーソル移動指示を処理する． */
static void move_arrow(char c){
 int col=col_no(pos);
 if(c=='A'&&line_start(pos)>0)pos=position_col(line_start(line_start(pos)-1),col);
 else if(c=='B'&&line_end(pos)<len)pos=position_col(line_end(pos)+1,col);
 else if(c=='D'&&pos>line_start(pos))pos--;
 else if(c=='C'&&pos<line_end(pos))pos++;
}

/* 全行の文字列を字句どおり置換する．正規表現は使わず，容量超過なら一切変更しない． */
static int replace_all(char *old,const char *new_text){
 int a=(int)strlen(old),b=(int)strlen(new_text),hits=0;
 if(!a)return -1;
 for(int i=0;i+a<=len;){
  if(!memcmp(data+i,old,a)){hits++;i+=a;}
  else i++;
 }
 if(b>a && hits>(CAP-len)/(b-a))return -2;
 for(int i=0;i+a<=len;){
  if(!memcmp(data+i,old,a)){
   memmove(data+i+b,data+i+a,len-i-a);
   memcpy(data+i,new_text,b);
   len+=b-a;
   i+=b;
  }else i++;
 }
 if(hits){changed=1;pos=0;}
 return hits;
}

/* :%s/old/new/g は正規表現ではなく，全ファイルのリテラル文字列置換． */
static int substitute(char *cmd){
 if(strncmp(cmd,"%s/",3))return -1;
 char *old=cmd+3,*sep=strchr(old,'/');
 if(!sep)return -1;
 *sep++=0;
 char *end=strchr(sep,'/');
 if(!end || strcmp(end+1,"g"))return -1;
 *end=0;
 return replace_all(old,sep);
}

/* 共通の編集処理を用い，vi風キー操作でRAMファイルを編集する． */
int main(int argc,char **argv){
 if(argc!=2){puts("usage: vi file");return 1;}
 char path[128];
 if(editor_start(argv[1],path)<0)return 1;
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
     /* 対応端末では矢印キーは3バイトのCSI列を送る． */
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
   char cmd[96];int n=0;printf("\033[%d;1H\033[K:",visible_rows+3);
   while(read(0,&c,1)==1&&c!='\r'&&c!='\n'&&n<(int)sizeof(cmd)-1){
    if(c==27){n=0;break;}if(c==127||c==8){if(n){n--;printf("\b \b");}continue;}
    if(c>=32&&c<127){cmd[n++]=c;putchar(c);}
   }cmd[n]=0;
   if(!strcmp(cmd,"w")){notice=save(path)==0?"Saved":"Save failed";repaint=1;}
   else if(!strcmp(cmd,"q")){if(!changed)quit=1;else notice="Modified: :q! or :wq";}
   else if(!strcmp(cmd,"q!"))quit=1;
   else if(!strcmp(cmd,"wq")||!strcmp(cmd,"x")){if(save(path)==0)quit=1;else notice="Save failed";}
   else if(!strcmp(cmd,"help")){notice="i/a/o x dd :w :q! :wq :%s/old/new/g";}
   else if(!strncmp(cmd,"%s/",3)){
    int count=substitute(cmd);
    if(count<0)notice=count==-2?"File exceeds 2048 bytes":"Use :%s/old/new/g";
    else notice=count?"Replaced":"No match";
    repaint=0;
   }
   else notice="Unknown command";
  }
  else pending_d=0;
  if(!quit)draw(path,notice,repaint);
 }
 screen_leave();
 return 0;
}
