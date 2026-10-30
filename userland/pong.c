/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Pong: local CPU game or two-terminal game. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"

#define W 48
#define H 16
#define PADDLE 4

static void sendall(int fd,const char *s,int n) {
    if(fd<0)return;
    int pos=0,tries=0;
    while(pos<n && tries<100) {
        int r=write(fd,s+pos,(size_t)(n-pos));
        if(r<0)break;
        if(r==0){msleep(1);tries++;continue;}
        pos+=r;tries=0;
    }
}
static void message(int a,int b,const char *s) {
    int n=(int)strlen(s);
    sendall(a,s,n);sendall(b,s,n);
}
static void row(int fd,int y,const char *s) {
    char out[W+24];
    int n=snprintf(out,sizeof out,"\033[%d;1H%s",y+1,s);
    if(n>0 && n<(int)sizeof out)sendall(fd,out,n);
}
static void render(int a,int b,int left,int right,int bx,int by,int score0,int score1,int reset,char old[H][W+1]) {
    char grid[H][W+1];
    for(int y=0;y<H;y++) {
        for(int x=0;x<W;x++)grid[y][x]=' ';
        grid[y][W]=0;
    }
    /* Headers are viewer-specific; only the playing field is mirrored. */
    char title[W+1],score[W+1];
    snprintf(title,sizeof title,"PicoNIX PONG   YOU: W/S   Q: QUIT");
    if(reset) { row(a,0,title); row(b,0,title); }
    snprintf(score,sizeof score,"YOU: %d    OPPONENT: %d",score0,score1);
    if(reset || strcmp(old[1],score)) {
        row(a,1,score);
        snprintf(old[1],W+1,"%s",score);
        snprintf(score,sizeof score,"YOU: %d    OPPONENT: %d",score1,score0);
        row(b,1,score);
    }
    for(int x=0;x<W;x++)grid[3][x]=grid[H-2][x]='-';
    for(int y=4;y<H-2;y++) {
        grid[y][0]=grid[y][W-1]='|';
        if((y&1)==0)grid[y][W/2]=':';
    }
    for(int i=0;i<PADDLE;i++) {
        grid[4+left+i][W-3]='#';grid[4+right+i][2]='#';
    }
    /* The canonical field is viewed from console's right-hand side. */
    grid[by][W-1-bx]='O';
    for(int y=3;y<H;y++)if(reset || memcmp(old[y],grid[y],W)) {
        char mirror[W+1];
        for(int x=0;x<W;x++)mirror[x]=grid[y][W-1-x];
        mirror[W]=0;
        row(a,y,grid[y]);
        row(b,y,mirror);
        memcpy(old[y],grid[y],W+1);
    }
}

int main(int argc,char **argv) {
    int two=argc==2 && !strcmp(argv[1],"-2");
    if(argc>2 || (argc==2 && !two)) {
        puts("usage: pong [-2]");return 1;
    }
    int side=get_tty();
    if(side!=0 && side!=1) {puts("pong: unknown terminal");return 1;}
    int other=-1;
    if(two) {
        const char *remote=side ? "/dev/console" : "/dev/uart1";
        other=open(remote,O_RDWR|O_NONBLOCK);
        if(other<0){puts("pong: cannot open other terminal");return 1;}
        if(ioctl(other,OS_TTY_CLAIM,0)<0) {
            close(other);puts("pong: other terminal is busy");return 1;
        }
    }
    int canon=ioctl(0,OS_TTY_GET_CANON,0);
    int echo=ioctl(0,OS_TTY_GET_ECHO,0);
    int flags=fcntl(0,F_GETFL,0);
    if(canon<0||echo<0||flags<0||
       ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0||
       fcntl(0,F_SETFL,flags|O_NONBLOCK)<0||
       (two && (ioctl(other,OS_TTY_SET_CANON,0)<0||ioctl(other,OS_TTY_SET_ECHO,0)<0))) {
        if(flags>=0)fcntl(0,F_SETFL,flags);
        if(echo>=0)ioctl(0,OS_TTY_SET_ECHO,echo);
        if(canon>=0)ioctl(0,OS_TTY_SET_CANON,canon);
        if(two){ioctl(other,OS_TTY_RELEASE,0);close(other);}
        puts("pong: cannot configure terminals");return 1;
    }
    message(1,other,"\033[?1049h\033[?25l\033[2J\033[H");
    int left=3,right=3,bx=W/2,by=8,dx=1,dy=1,score0=0,score1=0,live=1;
    char old[H][W+1]; /* Local RAM, no writable PIC globals. */
    int initial=1,ticks=0;
    while(live) {
        char c;
        for(int player=0;player<(two?2:1);player++) {
            int fd=player?other:0;
            int r;
            while((r=read(fd,&c,1))==1) {
                if(c=='q'||c=='Q'||c==3){live=0;break;}
                int *paddle=((side==0)==(player==0) ? &left : &right);
                if((c=='w'||c=='W'||c=='i'||c=='I') && *paddle>0) --*paddle;
                if((c=='s'||c=='S'||c=='k'||c=='K') && *paddle<H-6-PADDLE) ++*paddle;
            }
            if(r<0){live=0;break;}
        }
        if(!live)break;
        /* A deliberately limited opponent: one step every three frames. */
        if(!two && ++ticks%3==0) {
            int *cpu=side==0 ? &right : &left;
            int center=4+*cpu+PADDLE/2;
            if(center<by && *cpu<H-6-PADDLE)++*cpu;
            else if(center>by && *cpu>0)--*cpu;
        }
        bx+=dx;by+=dy;
        if(by<=4){by=4;dy=1;}
        if(by>=H-3){by=H-3;dy=-1;}
        if(dx<0 && bx==3 && by>=4+left && by<4+left+PADDLE)dx=1;
        if(dx>0 && bx==W-4 && by>=4+right && by<4+right+PADDLE)dx=-1;
        if(bx<=1 || bx>=W-2) {
            if(bx<=1)score1++;else score0++;
            bx=W/2;by=8;dx=-dx;dy=-dy;
        }
        if(side==0) render(1,other,left,right,bx,by,score0,score1,initial,old);
        else render(other,1,left,right,bx,by,score0,score1,initial,old);
        initial=0;
        msleep(90);
    }
    message(1,other,"\033[0m\033[?1049l\033[?25h");
    fcntl(0,F_SETFL,flags);
    if(two){ioctl(other,OS_TTY_RELEASE,0);close(other);}
    printf("\r\nPong finished: %d - %d\r\n",score0,score1);
    return 0;
}
