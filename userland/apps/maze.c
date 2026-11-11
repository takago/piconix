/* SPDX-License-Identifier: GPL-3.0-or-later */
/* A small, reproducible random maze for terminal/PTY agent experiments. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "../lib/screen.h"

#define W 21
#define H 9
#define GOAL_X 17
#define GOAL_Y 7

/* Read the 1 ms SysTick counter exposed by the kernel. */
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

/* Xorshift32 needs a nonzero internal state. */
static unsigned random32(unsigned *state){
    unsigned x=*state;
    x^=x<<13;x^=x>>17;x^=x<<5;
    *state=x;
    return x;
}

/* Generate a perfect maze with randomized depth-first search. */
/* Keep the writable map on the task stack: no absolute pointers in the PIC image. */
static void generate(char map[H][W+1],unsigned seed){
    unsigned state=seed?seed:0x6d2b79f5u;
    unsigned char stack[W*H];
    int top=0;
    for(int y=0;y<H;y++){
        for(int x=0;x<W;x++)map[y][x]='#';
        map[y][W]=0;
    }
    map[1][1]=' ';
    stack[top++]=(unsigned char)(W+1);
    while(top){
        int p=stack[top-1],x=p%W,y=p/W;
        unsigned char next[4];
        int count=0;
        const int dx[4]={0,2,0,-2},dy[4]={-2,0,2,0};
        for(int k=0;k<4;k++){
            int xx=x+dx[k],yy=y+dy[k];
            if(xx>0&&xx<W-1&&yy>0&&yy<H-1&&map[yy][xx]=='#')
                next[count++]=(unsigned char)k;
        }
        if(!count){top--;continue;}
        int dir=next[random32(&state)%(unsigned)count];
        int xx=x+dx[dir],yy=y+dy[dir];
        map[(y+yy)/2][(x+xx)/2]=' ';
        map[yy][xx]=' ';
        stack[top++]=(unsigned char)(yy*W+xx);
    }
    map[1][1]='S';
    map[GOAL_Y][GOAL_X]='G';
}

/* Check whether a maze coordinate is inside a passage. */
static int traversable(const char map[H][W+1],int x,int y){
    return (unsigned)x<W && (unsigned)y<H && map[y][x]!='#';
}

/* A shortest route always exists because generation visits every odd cell. */
static int shortest(const char map[H][W+1]){
    unsigned char distances[W*H],queue[W*H];
    for(int i=0;i<W*H;i++)distances[i]=255;
    int head=0,tail=0,start=W+1;
    queue[tail++]=(unsigned char)start;
    distances[start]=0;
    while(head<tail){
        int p=queue[head++],x=p%W,y=p/W;
        if(map[y][x]=='G')return distances[p];
        const int dx[4]={0,1,0,-1},dy[4]={-1,0,1,0};
        for(int i=0;i<4;i++){
            int xx=x+dx[i],yy=y+dy[i],v=yy*W+xx;
            if(traversable(map,xx,yy)&&distances[v]==255){
                distances[v]=(unsigned char)(distances[p]+1);
                queue[tail++]=(unsigned char)v;
            }
        }
    }
    return -1;
}

/* Only successful movements count toward Moves; walls count as Hits. */
static int step(const char map[H][W+1],int *x,int *y,int dx,int dy,int *moves,int *hits){
    int xx=*x+dx,yy=*y+dy;
    if(!traversable(map,xx,yy)){(*hits)++;return 0;}
    *x=xx;*y=yy;(*moves)++;
    return map[yy][xx]=='G'?2:1;
}

/* Draw the maze and recent status in the active terminal. */
static void render(const char map[H][W+1],int x,int y,int moves,int hits,
                   int optimum,int finished,unsigned seed,const char *notice){
    printf("\033[H\033[2JPicoNIX Maze  Seed: %u\r\n\r\n",seed);
    for(int r=0;r<H;r++){
        for(int c=0;c<W;c++){
            if(c==x&&r==y)printf("@");
            else{
                char ch=map[r][c];
                printf("%c",ch=='S'?' ':ch);
            }
        }
        printf("\r\n");
    }
    printf("\r\nPlayer: (%d,%d)  Goal: (%d,%d)\r\n",x,y,GOAL_X,GOAL_Y);
    printf("Moves: %d  Wall hits: %d  Shortest: %d\r\n",moves,hits,optimum);
    if(finished){
        printf("GOAL! Extra moves: %d\r\n",moves-optimum);
        printf("Efficiency: %d%%\r\n",moves?100*optimum/moves:100);
    }else printf("Status: Playing\r\n");
    printf("%s\r\nArrows: Move   r: New maze   q: Quit\033[K",notice);
    fflush(stdout);
}

/* Run the seeded maze in an interactive terminal or MCP PTY. */
int main(int argc,char **argv){
    int fixed=(argc==3 && argv[1][0]=='-' && argv[1][1]=='s' && !argv[1][2]);
    unsigned seed=0;
    if(fixed){
        char *end;
        unsigned long value=strtoul(argv[2],&end,0);
        if(!argv[2][0]||*end||argv[2][0]=='-')fixed=0;
        else seed=(unsigned)value;
    }
    if((argc!=1 && !fixed) || (argc==1 && fixed)){
        puts("usage: maze [-s seed]");return 1;
    }
    if(!fixed)seed=tick_seed();
    int canon=ioctl(0,OS_TTY_GET_CANON,0);
    int echo=ioctl(0,OS_TTY_GET_ECHO,0);
    int flags=fcntl(0,F_GETFL,0);
    if(canon<0||echo<0||flags<0){puts("maze: terminal required");return 1;}
    if(ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0){
        ioctl(0,OS_TTY_SET_CANON,canon);ioctl(0,OS_TTY_SET_ECHO,echo);
        puts("maze: terminal setup failed");return 1;
    }
    char map[H][W+1];
    int x=1,y=1,moves=0,hits=0,finished=0,escape=0;
    generate(map,seed);
    int optimum=shortest(map);
    const char *notice="Find the goal G.";
    screen_enter();
    render(map,x,y,moves,hits,optimum,finished,seed,notice);
    for(;;){
        char ch;
        if(read(0,&ch,1)!=1)break;
        int key=(unsigned char)ch,dx=0,dy=0;
        if(escape==1){escape=(key=='['||key=='O')?2:0;continue;}
        if(escape==2){
            escape=0;
            if(key=='A')dy=-1;
            else if(key=='B')dy=1;
            else if(key=='C')dx=1;
            else if(key=='D')dx=-1;
            else continue;
        }else if(key==27){escape=1;continue;}
        else if(key=='q'||key=='Q'||key==3)break;
        else if(key=='r'||key=='R'){
            if(!fixed){
                unsigned next=tick_seed();
                seed=(next!=seed?next:seed+1u);
            }
            generate(map,seed);
            optimum=shortest(map);
            x=y=1;moves=hits=finished=0;
            notice=fixed?"Maze restarted (same seed).":"New maze generated.";
        }else if(key=='h')dx=-1;
        else if(key=='j')dy=1;
        else if(key=='k')dy=-1;
        else if(key=='l')dx=1;
        else continue;
        if((dx||dy)&&!finished){
            int result=step(map,&x,&y,dx,dy,&moves,&hits);
            if(!result)notice="Wall! Choose another direction.";
            else if(result==2){finished=1;notice="Goal reached; press r for another maze.";}
            else notice="Moved.";
        }
        render(map,x,y,moves,hits,optimum,finished,seed,notice);
    }
    screen_leave();
    ioctl(0,OS_TTY_SET_ECHO,echo);
    ioctl(0,OS_TTY_SET_CANON,canon);
    fcntl(0,F_SETFL,flags);
    puts("");
    return 0;
}
