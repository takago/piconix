/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 全画面アプリが共有する端末の描画・復帰操作． */

/* ANSI端末のサイズ計測と画面制御の共通処理．screen_measureはcanonicalを無効にして呼び，端末が位置照会に応答しない場合は与えた既定値を使う．カーネルABIは変えない． */
#ifndef PICONIX_SCREEN_H
#define PICONIX_SCREEN_H
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"

typedef struct { int rows, cols; } screen_size_t;
/* 全画面操作用に代替画面へ切り替え，カーソルを隠す． */
static void screen_enter(void){setvbuf(stdout,0,_IONBF,0);printf("\033[?1049h\033[?25l\033[2J\033[H");}

/* 全画面表示を終え，元の端末画面とカーソルを復元する． */
static void screen_leave(void){printf("\033[0m\033[?1049l\033[?25h");setvbuf(stdout,0,_IOLBF,0);}

/* ANSIエスケープシーケンスで指定行・列へカーソルを移す． */
static void screen_move(int row,int col){printf("\033[%d;%dH",row,col);}

/* 端末サイズを問い合わせ，取得できなければ指定された既定値を利用する． */
static void screen_measure(screen_size_t *size){
 int flags=fcntl(0,F_GETFL,0);
 if(flags<0||fcntl(0,F_SETFL,flags|O_NONBLOCK)<0)return;
 /* 不正な応答で座標がオーバーフローしないように上限値へ丸める． */
 printf("\033[999;999H\033[6n");fflush(stdout);
 int state=0,row=0,col=0;
 for(int t=0;t<20&&state!=6;t++){
  char c;
  while(read(0,&c,1)==1){
   if(state==0)state=c==27?1:0;
   else if(state==1)state=c=='['?2:0;
   else if(state==2){if(c>='0'&&c<='9'){row=c-'0';state=3;}else state=0;}
   else if(state==3){if(c>='0'&&c<='9'){if(row<1000)row=row*10+c-'0';else state=0;}else if(c==';')state=4;else state=0;}
   else if(state==4){if(c>='0'&&c<='9'){col=c-'0';state=5;}else state=0;}
   else if(state==5){if(c>='0'&&c<='9'){if(col<1000)col=col*10+c-'0';else state=0;}else if(c=='R')state=6;else state=0;}
   if(state==6)break;
  }
  if(state!=6)msleep(10);
 }
 fcntl(0,F_SETFL,flags);
 if(state==6&&row>=8&&col>=30){
  size->rows=row>1000?1000:row;
  size->cols=col>1000?1000:col;
 }
}
#endif
