/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 3x3 tic-tac-toe against an optimal computer player; TTY and MCP PTY. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include "../lib/screen.h"

/* All mutable state is on the stack (PIC user program). */
static const unsigned char triples[8][3]={
    {0,1,2},{3,4,5},{6,7,8},{0,3,6},
    {1,4,7},{2,5,8},{0,4,8},{2,4,6}
};

/* X=1, O=2; returns winning side, draw=3, or unfinished=0. */
/* Determine win, draw or unfinished board state. */
static int outcome(const unsigned char b[9]){
    for(int k=0;k<8;k++){
        int a=triples[k][0],c=triples[k][1],d=triples[k][2];
        if(b[a] && b[a]==b[c] && b[a]==b[d])return b[a];
    }
    for(int k=0;k<9;k++)if(!b[k])return 0;
    return 3;
}

/* Exhaustive minimax: X maximizes, O minimizes, independent of PLAYER. */
/* Compute a minimax evaluation for the next side. */
static int score(unsigned char b[9],int side){
    int result=outcome(b);
    if(result)return result==1?1:result==2?-1:0;
    int best=side==1?-2:2;
    for(int k=0;k<9;k++)if(!b[k]){
        b[k]=(unsigned char)side;
        int value=score(b,3-side);
        b[k]=0;
        if(side==1){if(value>best)best=value;}
        else if(value<best)best=value;
    }
    return best;
}

/* Deterministic tie break: center, corners, then edges. */
/* Pick an optimal move for either side with a stable tie-break order. */
static int computer_move(unsigned char b[9],int com){
    static const unsigned char order[9]={4,0,2,6,8,1,3,5,7};
    int best=com==1?-2:2,chosen=-1;
    for(int i=0;i<9;i++){
        int k=order[i];
        if(b[k])continue;
        b[k]=(unsigned char)com;
        int value=score(b,3-com);
        b[k]=0;
        if((com==1&&value>best)||(com==2&&value<best)){best=value;chosen=k;}
    }
    if(chosen>=0)b[chosen]=(unsigned char)com;
    return chosen;
}

/* Draw the board and status in the current terminal. */
static void render(const unsigned char b[9],int cursor,int last,int moves,int invalid,
                   int finished,int player,const char *message){
    printf("\033[H\033[2JPicoNIX Tic-Tac-Toe - PLAYER(%c, %s) vs COM(%c, %s)\r\n\r\n",
           player==1?'X':'O',player==1?"First":"Second",
           player==1?'O':'X',player==1?"Second":"First");
    printf("       a   b   c\r\n");
    for(int y=0;y<3;y++){
        printf("   %d  ",y+1);
        for(int x=0;x<3;x++){
            int k=y*3+x;
            char c=b[k]==1?'X':b[k]==2?'O':'.';
            if(k==cursor)printf("\033[7m");
            if(k==last)printf("[%c]",c);
            else printf(" %c ",c);
            if(k==cursor)printf("\033[0m");
            if(x!=2)printf("|");
        }
        printf("\r\n");
        if(y!=2)printf("      ---+---+---\r\n");
    }
    printf("\r\nMoves: %d  Invalid moves: PLAYER %d  COM 0\r\n",moves,invalid);
    if(finished){
        int result=outcome(b);
        printf("Game Over: %s\r\n",result==player?"PLAYER wins":result==3?"Draw":"COM wins");
    }else printf("Turn: PLAYER\r\n");
    printf("%s\r\nArrows: Move   SPACE: Place   r: Restart   q: Quit\033[K",message);
    fflush(stdout);
}

/* Run an interactive TTY/PTY Tic-Tac-Toe session. */
int main(int argc,char **argv){
    if(argc>2 || (argc==2 && strcmp(argv[1],"-c"))){puts("usage: tictactoe [-c]");return 1;}
    int player=argc==2?2:1,com=3-player;
    int canon=ioctl(0,OS_TTY_GET_CANON,0);
    int echo=ioctl(0,OS_TTY_GET_ECHO,0);
    int flags=fcntl(0,F_GETFL,0);
    if(canon<0||echo<0||flags<0){puts("tictactoe: terminal required");return 1;}
    if(ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0){
        ioctl(0,OS_TTY_SET_CANON,canon);ioctl(0,OS_TTY_SET_ECHO,echo);
        puts("tictactoe: terminal setup failed");return 1;
    }
    unsigned char board[9]={0};
    int cursor=4,last=-1,moves=0,invalid=0,finished=0,escape=0;
    const char *notice="Your move.";
    if(com==1){last=computer_move(board,com);moves++;notice="COM opened. Your move.";}
    screen_enter();
    setvbuf(stdout,0,_IOFBF,0); /* render() flushes before input or pauses. */
    render(board,cursor,last,moves,invalid,finished,player,notice);
    for(;;){
        char ch;
        if(read(0,&ch,1)!=1)break;
        int key=(unsigned char)ch;
        if(escape==1){escape=(key=='['||key=='O')?2:0;continue;}
        if(escape==2){
            escape=0;
            if(key=='A'&&cursor>=3)cursor-=3;
            else if(key=='B'&&cursor<6)cursor+=3;
            else if(key=='C'&&cursor%3<2)cursor++;
            else if(key=='D'&&cursor%3>0)cursor--;
            else continue;
        }else if(key==27){escape=1;continue;}
        else if(key=='q'||key=='Q'||key==3)break;
        else if(key=='r'||key=='R'){
            for(int k=0;k<9;k++)board[k]=0;
            cursor=4;last=-1;moves=invalid=finished=0;
            notice="New game.";
            if(com==1){last=computer_move(board,com);moves++;notice="COM opened. Your move.";}
        }
        else if(key=='h'&&cursor%3>0)cursor--;
        else if(key=='l'&&cursor%3<2)cursor++;
        else if(key=='k'&&cursor>=3)cursor-=3;
        else if(key=='j'&&cursor<6)cursor+=3;
        else if(key==' '&&!finished){
            if(board[cursor]){invalid++;notice="Occupied square. Try another.";}
            else{
                board[cursor]=(unsigned char)player;last=cursor;moves++;
                finished=outcome(board)!=0;
                if(!finished){
                    render(board,cursor,last,moves,invalid,0,player,"COM is thinking...");
                    msleep(400);
                    last=computer_move(board,com);moves++;
                    finished=outcome(board)!=0;
                }
                notice=finished?"Game finished; press r for another game.":"COM played. Your move.";
            }
        }else continue;
        render(board,cursor,last,moves,invalid,finished,player,notice);
    }
    screen_leave();
    ioctl(0,OS_TTY_SET_ECHO,echo);
    ioctl(0,OS_TTY_SET_CANON,canon);
    fcntl(0,F_SETFL,flags);
    puts("");
    return 0;
}
