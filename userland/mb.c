/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Small original multi-line moon buggy game. Space jumps, q quits. */
#include "unistd.h"
#include "stdio.h"
#include "fcntl.h"
#include <stdint.h>

#define WIDTH 52
#define HEIGHT 10
#define BUGGY_X 8
#define FRONT_WHEEL_X (BUGGY_X + 8)
#define REAR_WHEEL_X  (BUGGY_X + 1)

/* A small original lunar buggy scene, inspired by classic terminal games.
   Fixed cursor positions avoid CR/LF translation and scrolling. */
static void draw(int obstacle, int crater_width, int jump, int score, int frame) {
    char screen[HEIGHT][WIDTH+1];
    for(int y=0;y<HEIGHT;y++) {
        for(int x=0;x<WIDTH;x++) screen[y][x]=' ';
        screen[y][WIDTH]=0;
    }
    const char *title="  M O O N   B U G G Y";
    const char *help="  SPACE/j: jump    q: quit";
    for(int i=0;title[i];i++) screen[0][i]=title[i];
    for(int i=0;help[i];i++) screen[9][i]=help[i];
    /* Score is drawn separately in the upper right. */
    int lift=jump>0 ? ((jump>=16 || jump<=3) ? 1 : 2) : 0;
    int y=4-lift;
    const char *body0="    __     ";
    const char *body1=" __/[]\\__  ";
    const char *body2=(frame&1) ? "(-)    (-)" : "(o)    (o)";
    for(int i=0;body0[i];i++) screen[y][BUGGY_X+i]=body0[i];
    for(int i=0;body1[i];i++) screen[y+1][BUGGY_X+i]=body1[i];
    for(int i=0;body2[i];i++) screen[y+2][BUGGY_X+i]=body2[i];
    /* Variable-width crater with raised rims. */
    for(int x=0;x<WIDTH;x++) screen[7][x]='-';
    if(obstacle>=0 && obstacle<WIDTH) {
        for(int i=0;i<crater_width;i++) if(obstacle+i<WIDTH) screen[7][obstacle+i]=' ';
        if(obstacle>0) screen[7][obstacle-1]='^';
        if(obstacle+crater_width<WIDTH) screen[7][obstacle+crater_width]='^';
    }
    for(int x=0;x<WIDTH;x++) screen[8][x]='.';
    /* Move to absolute screen coordinates; never emit CR/LF per frame. */
    for(int row=0;row<HEIGHT;row++) {
        printf("\033[%d;1H",row+1);
        printf("%s",screen[row]);
    }
    printf("\033[1;35HSCORE: %-6d",score);
}

int main(void) {
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
    printf("\033[?25l"); /* Hide cursor while animating. */
    uint32_t rng=0x3a5f917u; /* Local state: no writable global data in PIC image. */
    int obstacle=45, crater_width=4, jump=0, score=0, running=1, again=1, frame=0;
    printf("\033[?1049h\033[2J\033[H"); /* Alternate screen. */
    while(again) {
      obstacle=45; crater_width=4; jump=0; score=0; running=1; frame=0;
      while(running) {
        char ch;
        while(read(0,&ch,1)==1) {
            if(ch=='q'||ch=='Q') running=0;
            if((ch==' ' || ch=='j') && jump==0) jump=18;
        }
        if(!running) break;
        draw(obstacle,crater_width,jump,score,frame++);
        /* Crater width varies; jump must be active to cross. */
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
        if(jump>0) jump--;
        msleep(110);
    }
      if(running==2) {
          printf("\033[9;1H\033[2K  GAME OVER!  SCORE: %d   Restart? (y/n)",score);
          /* Wait for an explicit response without busy-spinning. */
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
    printf("\033[0m\033[?1049l\033[?25h"); /* Restore cursor and original screen. */
    fcntl(0,F_SETFL,flags);
    printf("\r\nScore: %d\r\n",score);
    return 0;
}
