/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Three-symbol SameGame for PicoNIX: no color terminal needed. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "../lib/screen.h"

#define W 20
#define H 12
#define CELLS (W*H)

/* Obtain a reproducible default seed from the kernel's millisecond counter. */
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

/* Small PRNG; this is a game seed, not cryptographic randomness. */
static unsigned random32(unsigned *s){
    unsigned x=*s;
    x^=x<<13;x^=x>>17;x^=x<<5;
    *s=x;
    return x;
}

/* Fill the board with A/B/C blocks; the initial board has a legal pair. */
static void generate(unsigned char board[CELLS],unsigned seed){
    unsigned state=seed?seed:0x6d2b79f5u;
    for(int k=0;k<CELLS;k++)board[k]=(unsigned char)('A'+random32(&state)%3u);
    board[1]=board[0];
}

/* Return the orthogonally connected component, using a bounded BFS queue. */
static int group(const unsigned char board[CELLS],int pos,unsigned char mark[CELLS]){
    for(int i=0;i<CELLS;i++)mark[i]=0;
    if(pos<0||pos>=CELLS||!board[pos])return 0;
    int q[CELLS],head=0,tail=0;
    unsigned char target=board[pos];
    q[tail++]=pos;
    mark[pos]=1;
    while(head<tail){
        int k=q[head++],x=k%W,y=k/W;
        const int dx[4]={0,1,0,-1},dy[4]={-1,0,1,0};
        for(int j=0;j<4;j++){
            int xx=x+dx[j],yy=y+dy[j];
            if(xx<0||xx>=W||yy<0||yy>=H)continue;
            int next=yy*W+xx;
            if(!mark[next]&&board[next]==target){
                mark[next]=1;
                q[tail++]=next;
            }
        }
    }
    return tail;
}

/* Blocks fall down after removal, then nonempty columns slide to the left. */
static void compact(unsigned char board[CELLS]){
    for(int x=0;x<W;x++){
        int dest=H-1;
        for(int y=H-1;y>=0;y--)if(board[y*W+x]){
            unsigned char value=board[y*W+x];
            board[y*W+x]=0;
            board[dest*W+x]=value;
            dest--;
        }
        while(dest>=0){board[dest*W+x]=0;dest--;}
    }
    int destx=0;
    for(int x=0;x<W;x++){
        if(!board[(H-1)*W+x])continue;
        if(destx!=x)for(int y=0;y<H;y++){
            board[y*W+destx]=board[y*W+x];
            board[y*W+x]=0;
        }
        destx++;
    }
}

/* Determine whether any same-letter adjacent pair remains. */
static int has_moves(const unsigned char board[CELLS]){
    for(int y=0;y<H;y++)for(int x=0;x<W;x++){
        int p=y*W+x;
        if(!board[p])continue;
        if(x+1<W&&board[p]==board[p+1])return 1;
        if(y+1<H&&board[p]==board[p+W])return 1;
    }
    return 0;
}

/* Return removed count (0 for invalid selection); score is standard (n-2)^2. */
static int remove_group(unsigned char board[CELLS],int pos,int *score,int *remaining){
    unsigned char mark[CELLS];
    int count=group(board,pos,mark);
    if(count<2)return 0;
    for(int k=0;k<CELLS;k++)if(mark[k])board[k]=0;
    compact(board);
    *remaining-=count;
    *score+=(count-2)*(count-2);
    if(!*remaining)*score+=1000;
    return count;
}

/* Track the largest score reached during this process, even across r. */
static void update_best(int score,unsigned seed,int *best,unsigned *best_seed){
    if(score>*best){*best=score;*best_seed=seed;}
}

/* Decode :a01 through :t12 without waiting for Enter. */
static int direct_position(int col,int tens,int ones){
    if(col<'a'||col>='a'+W||tens<'0'||tens>'1'||ones<'0'||ones>'9')return -1;
    int row=(tens-'0')*10+(ones-'0');
    return row>=1&&row<=H?(row-1)*W+(col-'a'):-1;
}

/* Draw an 80-column-safe text board and its current selection. */
static void render(const unsigned char board[CELLS],unsigned seed,int cursor,
                   int score,int best,unsigned best_seed,int remaining,int moves,int invalid,const char *notice){
    unsigned char mask[CELLS];
    int count=group(board,cursor,mask);
    printf("\033[H\033[2JPicoNIX SameGame  Seed: %u  (A / B / C)\r\n\r\n",seed);
    printf("    ");
    for(int x=0;x<W;x++)printf(" %c ",'a'+x);
    printf("\r\n");
    for(int y=0;y<H;y++){
        printf(" %2d ",y+1);
        for(int x=0;x<W;x++){
            int k=y*W+x;
            char c=board[k]?(char)board[k]:'.';
            if(k==cursor)printf("\033[7m");
            printf(" %c ",c);
            if(k==cursor)printf("\033[0m");
        }
        printf("\r\n");
    }
    printf("\r\nScore: %d  Best: %d  Best seed: %u\r\n",score,best,best_seed);
    printf("Remaining: %d/%d  Groups removed: %d\r\n",remaining,CELLS,moves);
    printf("Selection: %c%d  Group: %d  Invalid: %d\r\n",
           'a'+cursor%W,1+cursor/W,count,invalid);
    printf("Status: %s\r\n",remaining==0?"Cleared!":has_moves(board)?"Playing":"Game over");
    printf("%s\r\nArrows/hjkl: Select  SPACE: Remove  :a01 or :t12: Direct remove\r\n",notice);
    printf("r: Restart  q: Quit\033[J");
    fflush(stdout);
}

/* Start a seeded SameGame and process terminal / MCP PTY actions. */
int main(int argc,char **argv){
    int fixed=argc==3&&!strcmp(argv[1],"-s");
    unsigned seed=0;
    if(fixed){
        char *end;
        unsigned long v=strtoul(argv[2],&end,0);
        if(!argv[2][0]||argv[2][0]=='-'||*end)fixed=0;
        else seed=(unsigned)v;
    }
    if(argc!=1&&!fixed){puts("usage: samegame [-s seed]");return 1;}
    if(!fixed)seed=tick_seed();
    int canon=ioctl(0,OS_TTY_GET_CANON,0),echo=ioctl(0,OS_TTY_GET_ECHO,0);
    int flags=fcntl(0,F_GETFL,0);
    if(canon<0||echo<0||flags<0){puts("samegame: terminal required");return 1;}
    if(ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0){
        ioctl(0,OS_TTY_SET_CANON,canon);ioctl(0,OS_TTY_SET_ECHO,echo);
        puts("samegame: terminal setup failed");return 1;
    }
    unsigned char board[CELLS];
    int cursor=0,score=0,best=0,remaining=CELLS,moves=0,invalid=0,escape=0,cmd=0,coord_col=0,coord_tens=0;
    unsigned best_seed=seed;
    generate(board,seed);
    const char *notice="Remove adjacent groups of 2 or more matching letters.";
    screen_enter();
    setvbuf(stdout,0,_IOFBF,0); /* render() flushes before input or pauses. */
    render(board,seed,cursor,score,best,best_seed,remaining,moves,invalid,notice);
    for(;;){
        char ch;
        if(read(0,&ch,1)!=1)break;
        int key=(unsigned char)ch,remove=0;
        if(cmd){
            if(cmd==1&&key>='a'&&key<'a'+W){coord_col=key;cmd=2;continue;}
            if(cmd==2&&(key=='0'||key=='1')){coord_tens=key;cmd=3;continue;}
            if(cmd==3){
                int pos=direct_position(coord_col,coord_tens,key);
                if(pos>=0){cursor=pos;remove=1;}
                else notice="Invalid coordinate. Use :a01 to :t12.";
            }else notice="Invalid coordinate. Use :a01 to :t12.";
            cmd=0;
        }else if(escape==1){escape=(key=='['||key=='O')?2:0;continue;}
        else if(escape==2){
            escape=0;
            if(key=='A'&&cursor>=W)cursor-=W;
            else if(key=='B'&&cursor<CELLS-W)cursor+=W;
            else if(key=='C'&&cursor%W<W-1)cursor++;
            else if(key=='D'&&cursor%W>0)cursor--;
            else continue;
        }else if(key==27){escape=1;continue;}
        else if(key=='q'||key=='Q'||key==3)break;
        else if(key==':'){cmd=1;continue;}
        else if(key=='r'||key=='R'){
            if(!fixed){unsigned next=tick_seed();seed=next!=seed?next:seed+1u;}
            generate(board,seed);
            cursor=score=moves=invalid=0;remaining=CELLS;
            notice=fixed?"Board restarted (same seed).":"New board generated.";
        }else if(key=='h'&&cursor%W>0)cursor--;
        else if(key=='l'&&cursor%W<W-1)cursor++;
        else if(key=='k'&&cursor>=W)cursor-=W;
        else if(key=='j'&&cursor<CELLS-W)cursor+=W;
        else if(key==' ')remove=1;
        else continue;
        if(remove){
            if(remaining==0||!has_moves(board))notice="Game over; press r to play again.";
            else{
                int removed=remove_group(board,cursor,&score,&remaining);
                if(!removed){invalid++;notice="Select a connected group of at least 2.";}
                else{
                    moves++;
                    update_best(score,seed,&best,&best_seed);
                    notice=remaining==0?"Board cleared! Bonus +1000.":
                           has_moves(board)?"Group removed.":"No groups left. Game over.";
                }
            }
        }
        render(board,seed,cursor,score,best,best_seed,remaining,moves,invalid,notice);
    }
    screen_leave();
    ioctl(0,OS_TTY_SET_ECHO,echo);
    ioctl(0,OS_TTY_SET_CANON,canon);
    fcntl(0,F_SETFL,flags);
    puts("");
    return 0;
}
