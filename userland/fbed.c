/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Interactive editor for the PicoNIX 8x8 LED framebuffer. */
#include <stdint.h>
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"

static void draw(const uint8_t p[8],int pos) {
    printf("\033[HFrame buffer editor 8x8  (arrows move, Space toggles, c clears, q quits)\033[K\r\n");
    for(int y=0;y<8;y++) {
        for(int x=0;x<8;x++) {
            int on=(p[y]>>(7-x))&1;
            if(pos==y*8+x) printf("\033[7m%c\033[0m ",on?'#':'.');
            else printf("%c ",on?'#':'.');
        }
        const char hex[]="0123456789ABCDEF";
        putchar(' ');putchar(' ');putchar(hex[p[y]>>4]);putchar(hex[p[y]&15]);
        printf("\033[K\r\n");
    }
    printf("\033[K\r\nContents persist after exit.\033[K");
}
int main(void) {
    int fb=open("/dev/gpio/fb",O_RDWR);
    if(fb<0){puts("fbed: cannot open /dev/gpio/fb");return 1;}
    uint8_t pixels[8];
    if(read(fb,pixels,8)!=8 || ioctl(fb,FB_ENABLE,0)<0){
        puts("fbed: framebuffer unavailable");close(fb);return 1;
    }
    int canon=ioctl(0,OS_TTY_GET_CANON,0), echo=ioctl(0,OS_TTY_GET_ECHO,0);
    if(canon<0 || echo<0 || ioctl(0,OS_TTY_SET_CANON,0)<0 || ioctl(0,OS_TTY_SET_ECHO,0)<0){
        puts("fbed: terminal required");close(fb);return 1;
    }
    printf("\033[?1049h\033[?25l\033[2J");
    int pos=0, esc=0, run=1;
    draw(pixels,pos);
    while(run) {
        char c;
        if(read(0,&c,1)!=1)break;
        int dirty=0,refresh=0;
        if(esc==1) {esc=(c=='[')?2:0;continue;}
        if(esc==2) {
            esc=0;
            if(c=='A'&&pos>=8)pos-=8;
            if(c=='B'&&pos<56)pos+=8;
            if(c=='C'&&pos%8<7)pos++;
            if(c=='D'&&pos%8>0)pos--;
            refresh=1;
        } else if(c==27) {esc=1;continue;}
        else if(c=='q'||c=='Q'||c==3){run=0;continue;}
        else if(c==' '){pixels[pos/8]^=(uint8_t)(0x80u>>(pos%8));dirty=refresh=1;}
        else if(c=='c'||c=='C') {for(int i=0;i<8;i++)pixels[i]=0;dirty=refresh=1;}
        if(dirty && write(fb,pixels,8)!=8){puts("fbed: write failed");break;}
        if(refresh)draw(pixels,pos);
    }
    printf("\033[0m\033[?1049l\033[?25h");
    close(fb);
    return 0;
}
