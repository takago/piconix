/* ASCII scrolling display: stdin + optional UDP, multiplexed by select(). */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include <stdint.h>
#define CAP 1024
#define PORT 2001
static unsigned char queue[CAP];
static unsigned head,used;
static unsigned char pixels[8];
#include "../../include/font5x7.h"
#define font piconix_font5x7
/* 文字待ち行列の残容量を返す． */
static unsigned free_space(void){return CAP-used;}
/* 文字列をリングバッファの末尾に追加する． */
static void enqueue(const unsigned char *s,unsigned n){
    while(n--){queue[(head+used)%CAP]=*s++;used++;}
}
/* リングバッファの先頭から1文字を取り出す． */
static int dequeue(void){int c=queue[head];head=(head+1)%CAP;used--;return c;}
/* 表示対象の印字可能ASCII文字か判定する． */
static int valid(unsigned char c){return c>=32 && c<=126;}
/* The font has five columns, but most glyphs occupy fewer. Remove
 * both empty margins. Insert exactly one dark column between glyphs. */
static void glyph_bounds(unsigned char c,int *first,int *last){
    const unsigned char *g=font[c-32];
    int left=0,right=4;
    while(left<5 && !g[left])left++;
    while(right>=left && !g[right])right--;
    *first=left;
    *last=right;
}
/* 表示パターンを左へ送って新しい1列を描画する． */
static void draw_column(int fd,unsigned char column){
    for(unsigned y=0;y<8;y++)/* Bit 0 is the rightmost LED; shift left to scroll toward the left. */
    pixels[y]=(unsigned char)((pixels[y]<<1)|((column>>y)&1u));
    write(fd,pixels,8);
}
/* UDP待受ポートの十進文字列を検証し数値へ変換する． */
static int parse_port(const char *s){unsigned v=0;if(!*s)return -1;while(*s){if(*s<'0'||*s>'9')return -1;v=v*10+(*s++-'0');if(v>65535)return -1;}return v?(int)v:-1;}
/* UDP datagrams are plain text. Ignore control characters and drop
 * excess bytes when the ring is full; no acknowledgements are sent. */
static void receive_udp(int fd){
    unsigned char b[512];
    int n=recvfrom(fd,b,sizeof b,0,0,0);
    if(n<=0)return;
    for(int i=0;i<n;i++)
        if(valid(b[i]) && free_space())enqueue(b+i,1);
}

/* 標準入力と任意のUDP入力を待ち受け文字をスクロール表示する． */
int main(int argc,char **argv){
    int udp=0,port=PORT;
    if(argc==2 && !strcmp(argv[1],"-u"))udp=1;
    else if(argc==3 && !strcmp(argv[1],"-u")){udp=1;port=parse_port(argv[2]);}
    else if(argc!=1){puts("usage: scroll [-u [PORT]]");return 1;}
    if(port<0){puts("scroll: invalid port");return 1;}
    int fb=open("/dev/fb",O_RDWR);
    if(fb<0 || ioctl(fb,OS_FB_ENABLE,0)<0){puts("scroll: framebuffer unavailable");return 1;}
    int sock=-1;
    if(udp){
        sock=socket(AF_INET,SOCK_DGRAM,0);
        struct sockaddr_in addr={0};addr.sin_family=AF_INET;addr.sin_port=htons((uint16_t)port);
        if(sock<0 || bind(sock,(struct sockaddr *)&addr,sizeof addr)<0){puts("scroll: UDP bind failed");close(fb);return 1;}
        fcntl(sock,F_SETFL,O_NONBLOCK);
    }
    fcntl(0,F_SETFL,O_NONBLOCK);
    int eof=0,columns=0,idx=0,blank=0;
    unsigned char letter=0;
    /* Each select timeout advances the animation; input activity does not
     * reset the frame timer, because each iteration waits at most 100ms.
     * The receive budget is deliberately bounded to prevent starvation. */
    for(;;){
        fd_set rfds=0;
        if(!eof && free_space()>=64)FD_SET(0,&rfds);
        if(sock>=0)FD_SET(sock,&rfds);
        struct timeval tv={0,100000};
        int maxfd=sock>0?sock:0;
        int ready=select(maxfd+1,&rfds,0,0,&tv);
        if(ready>0){
            if(sock>=0 && FD_ISSET(sock,&rfds))receive_udp(sock);
            if(!eof && FD_ISSET(0,&rfds)){
                unsigned char data[64];int n=read(0,data,sizeof data);
                if(n==0)eof=1;
                else if(n>0){for(int i=0;i<n;i++)if(valid(data[i]) && free_space())enqueue(data+i,1);}
            }
        }
        /* Keep advancing even while input remains continuously ready. */
        if(ready>0)msleep(100);
        if(ready>=0){
            unsigned char column=0;
            if(columns){column=font[letter-32][idx++];columns--;}
            else if(blank){blank=0;}
            else if(used){
                int first,last;
                letter=(unsigned char)dequeue();
                glyph_bounds(letter,&first,&last);
                if(first<=last){
                    column=font[letter-32][first];
                    idx=first+1;
                    columns=last-first;
                    blank=1; /* One dark LED column between visible glyph pixels. */
                }else if(letter==' '){
                    /* A word space adds one column to the usual gap. */
                    blank=1;
                }
            }
            draw_column(fb,column);
            if(eof && sock<0 && !used && !columns && !blank){
                for(int i=0;i<8;i++)draw_column(fb,0);
                break;
            }
        }
    }
    if(sock>=0)close(sock);
    close(fb);
    return 0;
}
