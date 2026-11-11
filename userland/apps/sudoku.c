/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Reproducible 4x4/9x9 Sudoku for the PicoNIX terminal and MCP PTY. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "../lib/screen.h"

/* Both source puzzles have exactly one solution.  The transformations preserve it. */
static const char puzzle4[4][5]={
    "1..4", ".41.", ".1.3", "4.2."
};
static const char puzzle9[9][10]={
    "53..7....", "6..195...", ".98....6.",
    "8...6...3", "4..8.3..1", "7...2...6",
    ".6....28.", "...419..5", "....8..79"
};

/* Read the 1ms uptime counter; no network or kernel changes are needed. */
static unsigned tick_seed(void){
    char buf[24];
    int fd=open("/proc/ticks",O_RDONLY);
    if(fd<0)return 1;
    int n=read(fd,buf,sizeof(buf)-1);
    close(fd);
    if(n<=0)return 1;
    buf[n]=0;
    return (unsigned)strtoul(buf,0,10);
}

/* Pseudorandom generator used for reproducible puzzle transformations. */
static unsigned random32(unsigned *s){
    unsigned x=*s;
    x^=x<<13;x^=x>>17;x^=x<<5;
    *s=x;
    return x;
}

/* Shuffle rows/columns within groups and the groups themselves. */
static void order_indices(int n,int box,int result[9],unsigned *state){
    for(int i=0;i<n;i++)result[i]=i;
    for(int start=0;start<n;start+=box){
        for(int i=box-1;i>0;i--){
            int j=(int)(random32(state)%(unsigned)(i+1));
            int t=result[start+i];result[start+i]=result[start+j];result[start+j]=t;
        }
    }
    for(int i=n/box-1;i>0;i--){
        int j=(int)(random32(state)%(unsigned)(i+1));
        for(int k=0;k<box;k++){
            int t=result[i*box+k];result[i*box+k]=result[j*box+k];result[j*box+k]=t;
        }
    }
}

/* A given seed generates the same puzzle; writable grids live on the task stack. */
static void generate(int n,unsigned seed,unsigned char board[81],unsigned char given[81]){
    unsigned state=seed?seed:0x6d2b79f5u;
    int row[9],col[9],digits[9],box=n==4?2:3;
    order_indices(n,box,row,&state);
    order_indices(n,box,col,&state);
    for(int i=0;i<n;i++)digits[i]=i+1;
    for(int i=n-1;i>0;i--){
        int j=(int)(random32(&state)%(unsigned)(i+1));
        int t=digits[i];digits[i]=digits[j];digits[j]=t;
    }
    int transpose=(int)(random32(&state)&1);
    for(int y=0;y<n;y++)for(int x=0;x<n;x++){
        int sy=transpose?col[x]:row[y],sx=transpose?row[y]:col[x];
        char ch=n==4?puzzle4[sy][sx]:puzzle9[sy][sx];
        int k=y*n+x;
        given[k]=ch=='.'?0:(unsigned char)digits[ch-'1'];
        board[k]=given[k];
    }
}

/* Check for duplicates without consulting the hidden solution. */
static int allowed(const unsigned char board[81],int n,int pos,int value){
    if(value==0)return 1;
    int y=pos/n,x=pos%n,box=n==4?2:3;
    for(int i=0;i<n;i++){
        if(i!=x&&board[y*n+i]==value)return 0;
        if(i!=y&&board[i*n+x]==value)return 0;
    }
    int y0=y/box*box,x0=x/box*box;
    for(int yy=y0;yy<y0+box;yy++)for(int xx=x0;xx<x0+box;xx++)
        if(yy*n+xx!=pos&&board[yy*n+xx]==value)return 0;
    return 1;
}

/* Count cells involved in a conflict, including both copies of a duplicate. */
static int count_conflicts(const unsigned char board[81],int n){
    int conflicts=0;
    for(int k=0;k<n*n;k++)
        if(board[k]&&!allowed(board,n,k,board[k]))conflicts++;
    return conflicts;
}

/* A full board without conflicts is a solved Sudoku. */
static int completed(const unsigned char board[81],int n){
    for(int k=0;k<n*n;k++)if(!board[k])return 0;
    return count_conflicts(board,n)==0;
}

/* Allow arbitrary valid digits, including duplicates.  Only givens are locked. */
static const char *place(unsigned char board[81],const unsigned char given[81],
                         int n,int pos,int value,int *rejected,int *finished){
    if(given[pos]){(*rejected)++;return "Given cells cannot be changed.";}
    if(value<0||value>n){
        (*rejected)++;return "Number outside the puzzle range.";
    }
    board[pos]=(unsigned char)value;
    *finished=completed(board,n);
    if(*finished)return "Solved! Press r for another puzzle.";
    return count_conflicts(board,n)?"Entry saved; conflicts remain.":"Entry saved.";
}

/* Keep the header, 4-character cells and box boundaries aligned. */
static void draw_border(int n,int box){
    printf("   +");
    for(int x=0;x<n;x++){
        printf("----");
        if((x+1)%box==0)printf(x+1<n?"+ ":"+");
    }
    printf("\r\n");
}

/* ASCII brackets identify editable cells even in MCP's plain-text screen. */
static void render(const unsigned char board[81],const unsigned char given[81],
                   int n,unsigned seed,int cursor,int rejected,int finished,
                   const char *notice){
    int box=n==4?2:3,filled=0,givens=0;
    printf("\033[H\033[2JPicoNIX Sudoku %dx%d  Seed: %u\r\n\r\n",n,n,seed);
    printf("    ");
    for(int x=0;x<n;x++){
        printf(" %c  ",'a'+x);
        if((x+1)%box==0&&x+1<n)printf("  ");
    }
    printf("\r\n");
    for(int y=0;y<n;y++){
        if(y%box==0)draw_border(n,box);
        printf(" %d |",y+1);
        for(int x=0;x<n;x++){
            int k=y*n+x;
            if(board[k])filled++;
            if(given[k])givens++;
            if(k==cursor)printf("\033[7m");
            if(given[k])printf(" %c  ",'0'+board[k]);
            else printf("[%c] ",board[k]?'0'+board[k]:' ');
            if(k==cursor)printf("\033[0m");
            if((x+1)%box==0)printf(x+1<n?"| ":"|");
        }
        printf("\r\n");
    }
    draw_border(n,box);
    printf("\r\nFilled: %d/%d  Conflicts: %d  Rejected: %d  Status: %s\r\n",
           filled,n*n,count_conflicts(board,n),rejected,finished?"Solved":"Playing");
    printf("Given: %d  Editable: [ ] / [3]  Cursor: %c%d\r\n",
           givens,'a'+cursor%n,1+cursor/n);
    printf("%s\r\nArrows/hjkl: Select  1-%d: Set  0: Erase\r\n",notice,n);
    printf(":a1=2: Direct entry  r: Restart  q: Quit\033[J");
    fflush(stdout);
}

/* Parse -4 and -s <seed> independently of their order. */
int main(int argc,char **argv){
    int n=9,fixed=0;
    unsigned seed=0;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"-4"))n=4;
        else if(!strcmp(argv[i],"-s")&&!fixed&&i+1<argc){
            char *end;
            const char *arg=argv[++i];
            unsigned long v=strtoul(arg,&end,0);
            if(!arg[0]||arg[0]=='-'||*end){puts("usage: sudoku [-4] [-s seed]");return 1;}
            fixed=1;seed=(unsigned)v;
        }else{puts("usage: sudoku [-4] [-s seed]");return 1;}
    }
    if(!fixed)seed=tick_seed();
    int canon=ioctl(0,OS_TTY_GET_CANON,0),echo=ioctl(0,OS_TTY_GET_ECHO,0);
    int flags=fcntl(0,F_GETFL,0);
    if(canon<0||echo<0||flags<0){puts("sudoku: terminal required");return 1;}
    if(ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0){
        ioctl(0,OS_TTY_SET_CANON,canon);ioctl(0,OS_TTY_SET_ECHO,echo);
        puts("sudoku: terminal setup failed");return 1;
    }
    unsigned char board[81]={0},given[81]={0};
    int cursor=0,rejected=0,finished=0,escape=0,cmd=0,coord_x=0,coord_y=0;
    generate(n,seed,board,given);
    const char *notice="Fill every row, column and box without duplicates.";
    screen_enter();
    render(board,given,n,seed,cursor,rejected,finished,notice);
    for(;;){
        char ch;
        if(read(0,&ch,1)!=1)break;
        int key=(unsigned char)ch;
        if(cmd){
            if(cmd==1&&key>='a'&&key<'a'+n){coord_x=key-'a';cmd=2;continue;}
            if(cmd==2&&key>='1'&&key<'1'+n){coord_y=key-'1';cmd=3;continue;}
            if(cmd==3&&key=='='){cmd=4;continue;}
            if(cmd==4&&key>='0'&&key<='9'){
                cursor=coord_y*n+coord_x;
                notice=finished?"Solved; press r to start another puzzle.":
                       place(board,given,n,cursor,key-'0',&rejected,&finished);
            }else notice="Invalid coordinate. Use :a1=2 (or :a1=0).";
            cmd=0;
        }else if(escape==1){escape=(key=='['||key=='O')?2:0;continue;}
        else if(escape==2){
            escape=0;
            int x=cursor%n,y=cursor/n;
            if(key=='A'&&y>0)cursor-=n;
            else if(key=='B'&&y<n-1)cursor+=n;
            else if(key=='C'&&x<n-1)cursor++;
            else if(key=='D'&&x>0)cursor--;
            else continue;
        }else if(key==27){escape=1;continue;}
        else if(key=='q'||key=='Q'||key==3)break;
        else if(key==':'){cmd=1;continue;}
        else if(key=='r'||key=='R'){
            if(!fixed){unsigned next=tick_seed();seed=next!=seed?next:seed+1u;}
            generate(n,seed,board,given);
            cursor=rejected=finished=0;
            notice=fixed?"Puzzle restarted (same seed).":"New puzzle generated.";
        }else if(key=='h'&&cursor%n>0)cursor--;
        else if(key=='l'&&cursor%n<n-1)cursor++;
        else if(key=='k'&&cursor>=n)cursor-=n;
        else if(key=='j'&&cursor<n*(n-1))cursor+=n;
        else if(!finished&&((key>='1'&&key<='9')||key=='0'||key==' '))
            notice=place(board,given,n,cursor,key==' '?0:key-'0',&rejected,&finished);
        else continue;
        render(board,given,n,seed,cursor,rejected,finished,notice);
    }
    screen_leave();
    ioctl(0,OS_TTY_SET_ECHO,echo);
    ioctl(0,OS_TTY_SET_CANON,canon);
    fcntl(0,F_SETFL,flags);
    puts("");
    return 0;
}
