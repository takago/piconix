/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Live RP2040 pin assignment snapshot, formatted as a Pico header diagram. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "../include/gpio_status.h"
#define PINS_COUNT 29
typedef struct { unsigned mode,level,pull,duty,freq; } pins_record_t;
#define PINS_FB GPIO_STATUS_FB
#define PINS_PWM GPIO_STATUS_PWM
#define PINS_ADC GPIO_STATUS_ADC
#define PINS_UART_TX GPIO_STATUS_UART_TX
#define PINS_UART_RX GPIO_STATUS_UART_RX
#define PINS_GPIO_IN GPIO_STATUS_IN
#define PINS_GPIO_OUT GPIO_STATUS_OUT
#define PINS_FREE GPIO_STATUS_FREE

static void describe(char *dst, unsigned size, int pin, const pins_record_t *r) {
    switch(r->mode) {
    case PINS_FB: snprintf(dst,size,"fb"); break;
    case PINS_PWM: snprintf(dst,size,"pwm %u%% %uHz",(unsigned)r->duty,(unsigned)r->freq); break;
    case PINS_ADC: snprintf(dst,size,"adc%d",pin-26); break;
    case PINS_UART_TX: snprintf(dst,size,"uart-tx"); break;
    case PINS_UART_RX: snprintf(dst,size,"uart-rx"); break;
    case PINS_GPIO_IN: snprintf(dst,size,"in %s%s",r->level?"HIGH":"LOW",r->pull==1?" pull-up":r->pull==2?" pull-down":""); break;
    case PINS_GPIO_OUT: snprintf(dst,size,"out %s",r->level?"HIGH":"LOW"); break;
    case PINS_FREE: snprintf(dst,size,"free"); break;
    default: snprintf(dst,size,"other"); break;
    }
}
static void pinlabel(char *dst,unsigned size,int pin,const pins_record_t *records) {
    char mode[36]; describe(mode,sizeof mode,pin,&records[pin]);
    snprintf(dst,size,"GP%d %s",pin,mode);
}
int main(int argc,char **argv) {
    (void)argv;
    if(argc!=1) { puts("Usage: pins"); return 1; }
    pins_record_t records[PINS_COUNT]={0};
    /* Existing GPIO device provides live mux/direction/level/pull status. */
    int fd=open("/dev/gpio/0",O_RDONLY);
    if(fd<0) { puts("pins: cannot open /dev/gpio/0"); return 1; }
    for(int pin=0;pin<PINS_COUNT;pin++) {
        int status=ioctl(fd,OS_GPIO_STATUS_BASE+pin,0);
        if(status<0) { close(fd); puts("pins: GPIO status failed"); return 1; }
        records[pin].mode=(unsigned)status & GPIO_STATUS_MODE_MASK;
        records[pin].level=((unsigned)status>>GPIO_STATUS_LEVEL_SHIFT)&1u;
        records[pin].pull=((unsigned)status>>GPIO_STATUS_PULL_SHIFT)&3u;
    }
    close(fd);
    /* Read PWM configuration through its existing device files. */
    unsigned freq=0;
    fd=open("/dev/pwm/1/freq",O_RDONLY);
    if(fd>=0) {
        char buf[20]; int n=read(fd,buf,sizeof buf-1); close(fd);
        if(n>0) { buf[n]=0; for(int i=0;i<n && buf[i]>='0' && buf[i]<='9';i++) freq=freq*10u+(unsigned)(buf[i]-'0'); }
    }
    for(int pin=18;pin<=19;pin++) if(records[pin].mode==PINS_PWM) {
        const char *path=pin==18 ? "/dev/pwm/1/a" : "/dev/pwm/1/b";
        fd=open(path,O_RDONLY);
        if(fd>=0) {
            char buf[20]; int n=read(fd,buf,sizeof buf-1); close(fd);
            if(n>0) { buf[n]=0; for(int i=0;i<n && buf[i]>='0' && buf[i]<='9';i++)
                records[pin].duty=records[pin].duty*10u+(unsigned)(buf[i]-'0'); }
        }
        records[pin].freq=freq;
    }
    static const int left[20]={0,1,-1,2,3,4,5,-1,6,7,8,9,-1,10,11,12,13,-1,14,15};
    static const int right[20]={-1,-1,-1,-1,-1,-1,28,-1,27,26,-1,22,-1,21,20,19,18,-1,17,16};
    /* Avoid arrays of string pointers: their absolute addresses break RAM-app PIC. */

    puts("Raspberry Pi Pico / PicoNIX pins (live snapshot)");
    puts("                          +------- USB -------+");
    for(int i=0;i<20;i++) {
        char l[60],r[60];
        if(left[i]>=0) pinlabel(l,sizeof l,left[i],records);
        else snprintf(l,sizeof l,"%s","GND");
        if(right[i]>=0) pinlabel(r,sizeof r,right[i],records);
        else {
            /* Keep string literals at the call site for RAM-app PIC. */
            switch(i) {
            case 0: snprintf(r,sizeof r,"VBUS"); break;
            case 1: snprintf(r,sizeof r,"VSYS"); break;
            case 2: snprintf(r,sizeof r,"GND"); break;
            case 3: snprintf(r,sizeof r,"3V3_EN"); break;
            case 4: snprintf(r,sizeof r,"3V3_OUT"); break;
            case 5: snprintf(r,sizeof r,"ADC_VREF"); break;
            case 7: snprintf(r,sizeof r,"AGND"); break;
            case 10: snprintf(r,sizeof r,"RUN"); break;
            case 12: case 17: snprintf(r,sizeof r,"GND"); break;
            default: r[0]=0; break;
            }
        }
        printf("%s",l);
        unsigned len=0; while(l[len]) len++;
        while(len++<23) putchar(' ');
        /* PicoNIX printf does not support width modifiers such as %2d. */
        if(i+1<10) putchar(' ');
        printf("%d |                 | %d  %s\r\n",i+1,40-i,r);
    }
    puts("                          +-------------------+");
    puts("fb = framebuffer in use; PWM duty/frequency are live settings.");
    puts("GP23..25 are internal (not on the Pico header).");
    return 0;
}
