/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Live GPIO status editor.  The kernel remains the authority on ownership. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "../include/piconix.h"
#include "../include/gpio_status.h"

#define ROWS 8
#define COLS 4
#define PIN_COUNT 29

static int available(int pin) { return pin>=0 && pin<PIN_COUNT && pin!=24; }
static int at(int row,int col) { return (3-col)*8+7-row; }
static int editable(int m) {
    return m==GPIO_STATUS_FREE || m==GPIO_STATUS_IN || m==GPIO_STATUS_OUT;
}
static int query(int fd,int pin) { return ioctl(fd,OS_GPIO_STATUS_BASE+pin,0); }
static const char *owner(int m) {
    switch(m) {
    case GPIO_STATUS_FB:return "FB";
    case GPIO_STATUS_PWM:return "PWM";
    case GPIO_STATUS_ADC:return "ADC";
    case GPIO_STATUS_UART_TX:return "UART-TX";
    case GPIO_STATUS_UART_RX:return "UART-RX";
    default:return "RESERVED";
    }
}
static int pin_ioctl(int pin,int cmd) {
    char path[24];
    snprintf(path,sizeof path,"/dev/gpio/%d",pin);
    int fd=open(path,O_RDWR);
    if(fd<0)return -1;
    int rc=ioctl(fd,cmd,0);
    close(fd);
    return rc;
}
static int set_level(int pin,int high) {
    char path[24];
    snprintf(path,sizeof path,"/dev/gpio/%d",pin);
    int fd=open(path,O_RDWR);
    if(fd<0)return -1;
    char c=high?'1':'0';
    int rc=write(fd,&c,1);
    close(fd);
    return rc==1?0:-1;
}
static void cell(int fd,int pin,int selected) {
    char label[18];
    if(!available(pin))snprintf(label,sizeof label,"---");
    else {
        int s=query(fd,pin),m=s&GPIO_STATUS_MODE_MASK;
        int value=(s>>GPIO_STATUS_LEVEL_SHIFT)&1;
        int pull=(s>>GPIO_STATUS_PULL_SHIFT)&3;
        if(s<0)snprintf(label,sizeof label,"GP%02d [ERR]",pin);
        else if(m==GPIO_STATUS_FREE)snprintf(label,sizeof label,"GP%02d FREE",pin);
        else if(m==GPIO_STATUS_IN)snprintf(label,sizeof label,"GP%02d IN  %c %c",pin,value?'H':'L',pull==1?'U':pull==2?'D':'-');
        else if(m==GPIO_STATUS_OUT)snprintf(label,sizeof label,"GP%02d OUT %c",pin,value?'H':'L');
        else snprintf(label,sizeof label,"GP%02d [%s]",pin,owner(m));
    }
    if(pin==selected)printf("\033[7m");
    printf("%-18s",label);
    if(pin==selected)printf("\033[0m");
}
static void draw(int fd,int selected) {
    printf("\033[HGPIO editor (live status)\033[K\r\n\r\n");
    for(int row=0;row<ROWS;row++) {
        for(int col=0;col<COLS;col++)cell(fd,at(row,col),selected);
        printf("\033[K\r\n");
    }
    int s=query(fd,selected),m=s&GPIO_STATUS_MODE_MASK;
    printf("\r\nSelected GP%02d: ",selected);
    if(s<0)printf("unavailable");
    else if(m==GPIO_STATUS_IN||m==GPIO_STATUS_OUT) {
        int pull=(s>>GPIO_STATUS_PULL_SHIFT)&3;
        printf("%s  %s  pull %s",m==GPIO_STATUS_IN?"INPUT":"OUTPUT",
            ((s>>GPIO_STATUS_LEVEL_SHIFT)&1)?"HIGH":"LOW",
            pull==1?"up":pull==2?"down":"none");
    } else if(m==GPIO_STATUS_FREE)printf("FREE");
    else printf("[%s]  (read only)",owner(m));
    printf("\033[K\r\nArrows: select   i: input   o: output   h/l: high/low   SP: toggle\033[K\r\n");
    printf("u: pull up   d: pull down   n: no pull   q: quit\033[K");
}
/* Follow the visual grid, skipping unavailable positions, never changing owners. */
static int navigate(int pin,char key) {
    int row=7-(pin&7),col=3-(pin/8);
    int dr=key=='A'?-1:key=='B'?1:0;
    int dc=key=='C'?1:key=='D'?-1:0;
    for(;;) {
        row+=dr; col+=dc;
        if(row<0||row>=ROWS||col<0||col>=COLS)return pin;
        int candidate=at(row,col);
        if(available(candidate))return candidate;
    }
}
int main(void) {
    int status_fd=open("/dev/gpio/0",O_RDONLY);
    if(status_fd<0){puts("gpio: cannot inspect GPIO");return 1;}
    int canon=ioctl(0,OS_TTY_GET_CANON,0),echo=ioctl(0,OS_TTY_GET_ECHO,0);
    int flags=fcntl(0,F_GETFL,0);
    if(canon<0||echo<0||flags<0){puts("gpio: terminal required");close(status_fd);return 1;}
    if(ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0||
       fcntl(0,F_SETFL,flags|O_NONBLOCK)<0) {
        fcntl(0,F_SETFL,flags);
        ioctl(0,OS_TTY_SET_ECHO,echo);
        ioctl(0,OS_TTY_SET_CANON,canon);
        puts("gpio: terminal setup failed");close(status_fd);return 1;
    }
    printf("\033[?1049h\033[?25l\033[2J");
    int pin=16,esc=0,running=1,previous[PIN_COUNT];
    for(int p=0;p<PIN_COUNT;p++)previous[p]=available(p)?query(status_fd,p):-1;
    draw(status_fd,pin);
    while(running) {
        char c;
        int redraw=0;
        while(read(0,&c,1)==1) {
            if(esc==1){esc=c=='['?2:0;continue;}
            if(esc==2) {
                esc=0;
                if(c=='A'||c=='B'||c=='C'||c=='D') {
                    int next=navigate(pin,c);
                    if(next!=pin){pin=next;redraw=1;}
                }
                continue;
            }
            if(c==27){esc=1;continue;}
            if(c=='q'||c=='Q'||c==3){running=0;break;}
            int state=query(status_fd,pin),mode=state&GPIO_STATUS_MODE_MASK;
            if(state<0||!editable(mode))continue;
            int rc=-1;
            if(c=='i')rc=pin_ioctl(pin,OS_GPIO_INPUT);
            else if(c=='o')rc=pin_ioctl(pin,OS_GPIO_OUTPUT);
            else if(mode==GPIO_STATUS_OUT&&(c=='h'||c=='l'||c==' '))
                rc=set_level(pin,c=='h'?1:c=='l'?0:!((state>>GPIO_STATUS_LEVEL_SHIFT)&1));
            else if(mode==GPIO_STATUS_IN&&c=='u')rc=pin_ioctl(pin,OS_GPIO_PULL_UP);
            else if(mode==GPIO_STATUS_IN&&c=='d')rc=pin_ioctl(pin,OS_GPIO_PULL_DOWN);
            else if(mode==GPIO_STATUS_IN&&c=='n')rc=pin_ioctl(pin,OS_GPIO_NO_PULL);
            if(rc>=0)redraw=1;
        }
        if(!running)break;
        /* Poll state, including dynamic FB ownership; no separate reservation table. */
        for(int p=0;p<PIN_COUNT;p++) {
            if(!available(p))continue;
            int s=query(status_fd,p);
            if(s!=previous[p]){previous[p]=s;redraw=1;}
        }
        if(redraw)draw(status_fd,pin);
        msleep(50);
    }
    printf("\033[?25h\033[?1049l");
    fcntl(0,F_SETFL,flags);
    ioctl(0,OS_TTY_SET_ECHO,echo);
    ioctl(0,OS_TTY_SET_CANON,canon);
    close(status_fd);return 0;
}
