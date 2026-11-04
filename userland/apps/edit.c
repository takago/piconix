/* SPDX-License-Identifier: GPL-3.0-or-later */
/* RAMファイルの編集を行う簡易全画面エディタ． */

/* /tmp向けの全画面エディタ．上限は2 KiBで，端末サイズに合わせて描画する． */
#define EDITOR_NAME "edit"
#define EDITOR_GUTTER 4
#define EDITOR_TITLE "PicoNIX edit  "
#define EDITOR_HINT "^S Save  ^Q Quit (^Q again: Discard)  Arrows  TAB=4"
#include "editor_common.h"

/* 共通エディタ処理を使って編集対象を開き，キー入力で編集して保存する． */
int main(int argc,char **argv){
 if(argc!=2){puts("usage: edit file");return 1;}
 char path[128];
 if(editor_start(argv[1],path)<0)return 1;
 int esc=0,quit=0,confirm_discard=0;const char *notice="";draw(path,notice,0);
 while(!quit){char c;if(read(0,&c,1)!=1)break;notice="";int mode=2;
  if(c!=17)confirm_discard=0;
  if(esc==1){esc=c=='['?2:0;continue;}
  if(esc==2){esc=0;int col=col_no(pos);if(c=='A'&&line_start(pos)>0)pos=position_col(line_start(line_start(pos)-1),col);
   else if(c=='B'&&line_end(pos)<len)pos=position_col(line_end(pos)+1,col);
   else if(c=='D'&&pos>0)pos--;
   else if(c=='C'&&pos<len)pos++;
   else if(c=='3'){esc=3;continue;}
  }else if(esc==3){esc=0;if(c=='~'){int oldlen=len;int newline=pos<len&&data[pos]=='\n';del();if(len!=oldlen)mode=newline?0:1;}}
  else if(c==27){esc=1;continue;}
  else if(c==3){quit=1;continue;} /* Ctrl+Cで保存せず終了し，TTY設定を後で復旧する． */
  else if(c==17){if(!changed||confirm_discard)quit=1;else {confirm_discard=1;notice="Unsaved: Ctrl+Q again to discard, Ctrl+S to save";}}
  else if(c==19){notice=save(path)==0?"Saved":"Save failed";mode=1;}
  else if(c==127||c==8){int newline=pos>0&&data[pos-1]=='\n';int oldlen=len;erase();if(len!=oldlen)mode=newline?0:1;}
  else if(c==13||c==10){int oldlen=len;insert('\n');if(len!=oldlen)mode=0;}
  else if(c=='\t'||(c>=32&&c<127)){int oldlen=len;insert(c);if(len!=oldlen)mode=1;}
  draw(path,notice,mode);
 }
 screen_leave();
 return 0;
}
