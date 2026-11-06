/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 端末制御と時間処理の教材となる月面バギーゲーム． */

/* 端末向けの小さな月面バギーゲーム．Spaceでジャンプしqで終了する． */
#include "unistd.h"
#include "stdio.h"
#include "fcntl.h"
#include <stdint.h>
#include "../lib/screen.h"

#define WIDTH 52
#define HEIGHT 10
#define BUGGY_X 8
#define FRONT_WHEEL_X (BUGGY_X + 8)
#define REAR_WHEEL_X  (BUGGY_X + 1)

/* 複数行の月面バギーを固定位置に描く．改行変換や画面スクロールを避けるためカーソルを絶対位置へ移動する． */
static void draw(int obstacle, int crater_width, int jump, int score, int frame, int top, int left) {
    char screen[HEIGHT][WIDTH+1];
    for(int y=0;y<HEIGHT;y++) {
        for(int x=0;x<WIDTH;x++)
            screen[y][x]=' ';
        screen[y][WIDTH]=0;
    }
    const char *title="  M O O N   B U G G Y";
    const char *help="  SPACE/j: jump    q: quit";
    for(int i=0;title[i];i++)
        screen[0][i]=title[i];
    for(int i=0;help[i];i++)
        screen[9][i]=help[i];
    /* 得点は右上に独立して表示する． */
    int lift=jump>0 ? ((jump>=16 || jump<=3) ? 1 : 2) : 0;
    int y=4-lift;
    const char *body0="    __     ";
    const char *body1=" __/[]\\__  ";
    const char *body2=(frame&1) ? "(-)    (-)" : "(o)    (o)";
    for(int i=0;body0[i];i++)
        screen[y][BUGGY_X+i]=body0[i];
    for(int i=0;body1[i];i++)
        screen[y+1][BUGGY_X+i]=body1[i];
    for(int i=0;body2[i];i++)
        screen[y+2][BUGGY_X+i]=body2[i];
    /* クレーターは幅が変化し，周囲に盛り上がりがある． */
    for(int x=0;x<WIDTH;x++)
        screen[7][x]='-';
    if(obstacle>=0 && obstacle<WIDTH) {
        for(int i=0;i<crater_width;i++)
            if(obstacle+i<WIDTH)
                screen[7][obstacle+i]=' ';
        if(obstacle>0)
            screen[7][obstacle-1]='^';
        if(obstacle+crater_width<WIDTH)
            screen[7][obstacle+crater_width]='^';
    }
    for(int x=0;x<WIDTH;x++)
        screen[8][x]='.';
    /* 毎フレーム絶対座標で描画し，CR/LFは出力しない． */
    for(int row=0;row<HEIGHT;row++) {
        screen_move(top+row,left);
        printf("%s",screen[row]);
    }
    screen_move(top,left+34);
    printf("SCORE: %-6d",score);
}

/* 端末を非カノニカル・非ブロック入力に変更して月面バギーゲームを動かす．終了時はTTY設定を復元する． */
int main(int argc, char **argv) {
    (void)argv;
    if(argc!=1) {puts("usage: mb");return 1;}
    int canon=ioctl(0,OS_TTY_GET_CANON,0);
    int echo=ioctl(0,OS_TTY_GET_ECHO,0);
    int flags=fcntl(0,F_GETFL,0);
    if(canon<0 || echo<0 || flags<0) {puts("mb: console terminal required");return 1;}
    if(ioctl(0,OS_TTY_SET_CANON,0)<0 || ioctl(0,OS_TTY_SET_ECHO,0)<0 ||
       fcntl(0,F_SETFL,flags | O_NONBLOCK)<0) {
        fcntl(0,F_SETFL,flags);
        ioctl(0,OS_TTY_SET_ECHO,echo);
        ioctl(0,OS_TTY_SET_CANON,canon);
        puts("mb: terminal setup failed");return 1;
    }
    screen_size_t terminal={24,80};
    uint32_t rng=0x3a5f917u; /* PICイメージの書込み可能グローバル変数を避け，状態をローカル領域へ置く． */
    int obstacle=45, crater_width=4, jump=0, score=0, running=1, again=1, frame=0;
    screen_enter();
    screen_measure(&terminal);
    int top=terminal.rows>HEIGHT?(terminal.rows-HEIGHT)/2+1:1;
    int left=terminal.cols>WIDTH?(terminal.cols-WIDTH)/2+1:1;
    while(again) {
      obstacle=45; crater_width=4; jump=0; score=0; running=1; frame=0;
      while(running) {
        char ch;
        while(read(0,&ch,1)==1) {
            if(ch=='q'||ch=='Q')
                running=0;
            if((ch==' ' || ch=='j') && jump==0)
                jump=18;
        }
        if(!running)
            break;
        draw(obstacle,crater_width,jump,score,frame++,top,left);
        /* クレーターの幅は変わるため，越えるにはジャンプ状態が必要になる． */
        if(jump==0 && ((REAR_WHEEL_X>=obstacle && REAR_WHEEL_X<obstacle+crater_width) ||
                       (FRONT_WHEEL_X>=obstacle && FRONT_WHEEL_X<obstacle+crater_width))) {
            running=2; break;
        }
        if(--obstacle < -crater_width-2) {
            rng^=rng<<13; rng^=rng>>17; rng^=rng<<5;
            obstacle=43+(int)(rng%9);
            rng^=rng<<13; rng^=rng>>17; rng^=rng<<5;
            crater_width=3+(int)(rng%4);score++;
        }
        if(jump>0)
            jump--;
        msleep(110);
    }
      if(running==2) {
          screen_move(top+8,left);
          printf("\033[2K  GAME OVER!  SCORE: %d   Restart? (y/n)",score);
          /* ビジーループせず，ユーザの明示的な応答を待つ． */
          for(;;){
              char key;
              if(read(0,&key,1)==1){
                  if(key=='y'||key=='Y'){printf("\033[2J\033[H");break;}
                  if(key=='n'||key=='N'||key=='q'||key=='Q'){again=0;break;}
              }
              msleep(30);
          }
      } else again=0;
    }
    screen_leave(); /* カーソルと元の画面を復元する． */
    fcntl(0,F_SETFL,flags);
    printf("\r\nScore: %d\r\n",score);
    return 0;
}
