/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Reversi against a small built-in computer opponent. TTY and PTY share the same UI. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include "../lib/screen.h"

#define N 8
#define EMPTY 0
#define BLACK 1
#define WHITE 2

/* Board state is local: PIC applications must not require writable global data. */
/* Test that coordinates fall inside the 8x8 board. */
static int inside(int x,int y){return (unsigned)x<N && (unsigned)y<N;}
/* Return the opposite side (BLACK or WHITE). */
static int opponent(int stone){return 3-stone;}

/* Return the number of reversals; write changes only when apply is nonzero. */
static int move(unsigned char board[64],int x,int y,int stone,int apply){
    if(!inside(x,y) || board[y*N+x]!=EMPTY)return 0;
    int total=0;
    for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
        if(!dx&&!dy)continue;
        int cx=x+dx,cy=y+dy,n=0;
        while(inside(cx,cy) && board[cy*N+cx]==opponent(stone)){
            n++;cx+=dx;cy+=dy;
        }
        if(n && inside(cx,cy) && board[cy*N+cx]==stone){
            total+=n;
            if(apply)for(int k=1;k<=n;k++)board[(y+k*dy)*N+x+k*dx]=(unsigned char)stone;
        }
    }
    if(total && apply)board[y*N+x]=(unsigned char)stone;
    return total;
}

/* Count attempted placements that do not flip any stones. */
static int player_move(unsigned char board[64],int x,int y,int *invalid){
    int flips=move(board,x,y,BLACK,1);
    if(!flips)(*invalid)++;
    return flips;
}

/* Find whether this side has at least one legal placement. */
static int can_move(unsigned char board[64],int stone){
    for(int y=0;y<N;y++)for(int x=0;x<N;x++)if(move(board,x,y,stone,0))return 1;
    return 0;
}

/* Prefer corners; avoid squares adjacent to empty corners. Ties are deterministic. */
static void computer_move(unsigned char board[64],int *mx,int *my,int *invalid){
    int best=-100000,xbest=-1,ybest=-1;
    for(int y=0;y<N;y++)for(int x=0;x<N;x++){
        int flips=move(board,x,y,WHITE,0);
        if(!flips)continue;
        int score=flips*4;
        if((x==0||x==7)&&(y==0||y==7))score+=1000;
        else if((x<=1||x>=6)&&(y<=1||y>=6)){
            int corner_x=x<2?0:7,corner_y=y<2?0:7;
            if(board[corner_y*N+corner_x]==EMPTY)score-=100;
        }
        if(x==0||x==7||y==0||y==7)score+=18;
        if(score>best){best=score;xbest=x;ybest=y;}
    }
    *mx=xbest;*my=ybest;
    if(xbest>=0 && !move(board,xbest,ybest,WHITE,1))(*invalid)++;
}

/* Draw the 8x8 board and status on a physical TTY or PTY. */
static void render(unsigned char board[64],int cx,int cy,int turn,int finished,
                   int moves,int lastx,int lasty,int invalid_black,int invalid_white,const char *notice){
    int black=0,white=0;
    for(int i=0;i<64;i++){
        black+=board[i]==BLACK;
        white+=board[i]==WHITE;
    }
    printf("\033[H\033[2J");
    printf("PicoNIX Reversi - Player(B) vs COM(W)\r\n\r\n");
    printf("       a  b  c  d  e  f  g  h\r\n");
    for(int y=0;y<N;y++){
        printf("   %d  ",y+1);
        for(int x=0;x<N;x++){
            int p=board[y*N+x];
            char c=p==BLACK?'B':p==WHITE?'W':'.';
            int selected=x==cx&&y==cy;
            if(selected)printf("\033[7m");
            if(x==lastx&&y==lasty)printf("[%c]",c);
            else printf(" %c ",c);
            if(selected)printf("\033[0m");
        }
        printf("\r\n");
    }
    printf("\r\nBlack: %d  White: %d  Moves: %d\r\n",black,white,moves);
    printf("Invalid moves: PLAYER %d  COM %d\r\n",invalid_black,invalid_white);
    if(finished)printf("Game Over: %s\r\n",black>white?"PLAYER wins":white>black?"COM wins":"Draw");
    else printf("Turn: %s\r\n",turn==BLACK?"PLAYER":"COM");
    printf("%s\r\n",notice);
    printf("Arrows: Move   SPACE: Place   q: Quit\033[K");
    fflush(stdout);
}

/* Set up a full-screen TTY session, play against COM, and restore modes. */
int main(int argc,char **argv){
    (void)argv;
    if(argc!=1){puts("usage: reversi");return 1;}
    int canon=ioctl(0,OS_TTY_GET_CANON,0);
    int echo=ioctl(0,OS_TTY_GET_ECHO,0);
    int flags=fcntl(0,F_GETFL,0);
    if(canon<0||echo<0||flags<0){puts("reversi: terminal required");return 1;}
    if(ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0){
        ioctl(0,OS_TTY_SET_CANON,canon);ioctl(0,OS_TTY_SET_ECHO,echo);
        puts("reversi: terminal setup failed");return 1;
    }
    unsigned char board[64]={0};
    board[3*N+3]=WHITE;board[3*N+4]=BLACK;
    board[4*N+3]=BLACK;board[4*N+4]=WHITE;
    int x=3,y=2,turn=BLACK,finished=0,moves=0,escape=0;
    int lastx=-1,lasty=-1;
    int invalid_black=0,invalid_white=0;
    const char *notice="Your move (Black).";
    screen_enter();
    render(board,x,y,turn,finished,moves,lastx,lasty,invalid_black,invalid_white,notice);
    for(;;){
        char ch;
        if(read(0,&ch,1)!=1)break;
        int key=(unsigned char)ch;
        if(escape==1){escape=(key=='['||key=='O')?2:0;continue;}
        if(escape==2){
            escape=0;
            if(key=='A'&&y>0)y--;
            else if(key=='B'&&y<7)y++;
            else if(key=='C'&&x<7)x++;
            else if(key=='D'&&x>0)x--;
            else continue;
        }else if(key==27){escape=1;continue;}
        else if(key=='q'||key=='Q'||key==3)break;
        else if(key=='h'&&x>0)x--;
        else if(key=='l'&&x<7)x++;
        else if(key=='k'&&y>0)y--;
        else if(key=='j'&&y<7)y++;
        else if(key==' '&&!finished){
            if(!player_move(board,x,y,&invalid_black))notice="Illegal move. Choose a square that flips stones.";
            else{
                moves++;
                lastx=x;lasty=y;
                turn=WHITE;
                /* Show the player's last move before the COM responds. */
                render(board,x,y,turn,0,moves,lastx,lasty,invalid_black,invalid_white,"COM is thinking...");
                msleep(1000);
                if(can_move(board,WHITE)){
                    int mx,my;computer_move(board,&mx,&my,&invalid_white);moves++;
                    lastx=mx;lasty=my;
                    notice="COM played. Your move.";
                }else notice="COM passes. Your move.";
                if(!can_move(board,BLACK)){
                    if(can_move(board,WHITE)){
                        /* The player must pass; the computer may have multiple turns. */
                        do{int mx,my;computer_move(board,&mx,&my,&invalid_white);moves++;
                           lastx=mx;lasty=my;}
                        while(!can_move(board,BLACK)&&can_move(board,WHITE));
                        notice="Player passed; COM played.";
                    }
                }
                finished=!can_move(board,BLACK)&&!can_move(board,WHITE);
                turn=BLACK;
            }
        }else continue;
        render(board,x,y,turn,finished,moves,lastx,lasty,invalid_black,invalid_white,notice);
    }
    screen_leave();
    ioctl(0,OS_TTY_SET_ECHO,echo);
    ioctl(0,OS_TTY_SET_CANON,canon);
    fcntl(0,F_SETFL,flags);
    puts("");
    return 0;
}
