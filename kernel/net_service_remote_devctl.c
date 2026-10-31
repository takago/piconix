/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "net.h"
#include "net_services.h"
#include "devices.h"
#include "pins.h"
#include "piconix.h"
#include <stdio.h>
#include <string.h>
/* Restricted UDP device operations.  No process fd or user RAM is involved.
 * The allowlist is intentional: never expose the general VFS over UDP. */
static int net_device_request(const unsigned char *req, unsigned n, unsigned char *out, unsigned cap) {
    if(n<6 || cap<16) return -1;
    unsigned writing=0, enable=0, disable=0, prefix=0;
    if(n>=5 && !memcmp(req,"READ ",5)) prefix=5;
    else if(n>=6 && !memcmp(req,"WRITE ",6)) { prefix=6; writing=1; }
    else if(n>=7 && !memcmp(req,"ENABLE ",7)) { prefix=7; enable=1; }
    else if(n>=8 && !memcmp(req,"DISABLE ",8)) { prefix=8; disable=1; }
    else return -1;
    unsigned end=prefix;
    while(end<n && req[end]!='\n' && req[end]!=' ') ++end;
    if(end==prefix || end>=64) return -1;
    char path[64];
    memcpy(path,req+prefix,end-prefix); path[end-prefix]=0;
    unsigned payload=end;
    if(writing) {
        if(payload>=n) return -1;
        /* One separator before payload: newline permits binary framebuffer data. */
        ++payload;
    } else if(end!=n && !(end+1==n && req[end]=='\n')) return -1;
    if(!strcmp(path,"/dev/gpio/fb")) {
        if(enable || disable) return fb_ioctl(enable ? OS_FB_ENABLE : OS_FB_DISABLE)==0 ? 0 : -1;
        if(writing) return n-payload==8 && fb_rw((void *)(req+payload),8,1)==8 ? 0 : -1;
        if(cap<8) return -1;
        return fb_rw(out,8,0)==8 ? 8 : -1;
    }
    /* Only board-safe GPIOs are exposed over unauthenticated UDP.
     * GP23..25 are reserved by Pico W hardware; PWM, ADC and UART
     * pins are excluded by pin_gpio().  The framebuffer driver
     * rejects GP0..15 while matrix scanning is active. */
    if(!strncmp(path,"/dev/gpio/",10)) {
        const char *digits=path+10;
        unsigned pin=0;
        if(!*digits) return -1;
        for(const char *c=digits; *c; ++c) {
            if(*c<'0' || *c>'9') return -1;
            pin=pin*10u+(unsigned)(*c-'0');
            if(pin>28u) return -1;
        }
        if(!pin_gpio(pin) || pin>=23u) return -1;
        if(enable || disable)
            return gpio_driver_ioctl(pin,enable ? OS_GPIO_OUTPUT : OS_GPIO_INPUT)==0 ? 0 : -1;
        if(writing) {
            if(n-payload!=1u && !(n-payload==2u && req[payload+1]=='\n')) return -1;
            if(req[payload]!='0' && req[payload]!='1') return -1;
            unsigned char value=req[payload];
            return gpio_driver_rw(0,pin,&value,1,1)==1 ? 0 : -1;
        }
        if(cap<2u) return -1;
        int count=gpio_driver_rw(0,pin,out,1,0);
        if(count!=1) return -1;
        out[1]='\n';
        return 2;
    }
    if(enable || disable) return -1;
    unsigned which=0;
    if(!strcmp(path,"/dev/pwm/1/freq")) which=0;
    else if(!strcmp(path,"/dev/pwm/1/a")) which=1;
    else if(!strcmp(path,"/dev/pwm/1/b")) which=2;
    else which=3;
    if(which<3) {
        if(writing) {
            unsigned value;
            if(pwm_parse_value(which,req+payload,n-payload,&value)<0) return -1;
            return pwm_driver_set(which,value)==0 ? 0 : -1;
        }
        unsigned value=pwm_driver_get(which);
        int len=which==0 ? snprintf((char *)out,cap,"%u\n",value)
                         : snprintf((char *)out,cap,"%u.%u\n",value/10u,value%10u);
        return len>=0 && (unsigned)len<cap ? len : -1;
    }
    if(writing) return -1;
    unsigned channel=0;
    if(!strcmp(path,"/dev/adc/0")) channel=0;
    else if(!strcmp(path,"/dev/adc/1")) channel=1;
    else if(!strcmp(path,"/dev/adc/2")) channel=2;
    else if(!strcmp(path,"/dev/adc/temp")) channel=4;
    else return -1;
    if(channel!=4 && !pin_adc(channel)) return -1;
    int len=adc_driver_format(adc_driver_read(channel),channel==4,(char *)out,cap);
    return len>=0 && (unsigned)len<cap ? len : -1;
}

#ifndef PICONIX_WIFI
#define PICONIX_WIFI 0
#endif
#if PICONIX_WIFI
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/ip4_addr.h"
#include <stdint.h>
#include <string.h>
static struct udp_pcb *device_pcb;
/* Requests are processed outside lwIP receive callbacks. */
static unsigned char device_req[128];
static unsigned device_req_len;
static ip_addr_t device_peer;
static u16_t device_peer_port;
static unsigned char device_pending;
static void device_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                        const ip_addr_t *addr, u16_t port) {
    (void)arg; (void)pcb;
    if(!device_pending && p->tot_len<=sizeof(device_req) && p->tot_len>0) {
        device_req_len=pbuf_copy_partial(p,device_req,p->tot_len,0);
        ip_addr_copy(device_peer,*addr);
        device_peer_port=port;
        device_pending=1;
    }
    pbuf_free(p);
}
static int device_start(void) {
    device_pcb=udp_new_ip_type(IPADDR_TYPE_V4);
    if(!device_pcb) return 0;
    if(udp_bind(device_pcb,IP_ANY_TYPE,2000)!=ERR_OK) {
        udp_remove(device_pcb); device_pcb=NULL; return 0;
    }
    udp_recv(device_pcb,device_recv,NULL);
    return 1;
}
static void device_process_pending(void) {
    if(!device_pending || !device_pcb) return;
    unsigned char data[128];
    int len=net_device_request(device_req,device_req_len,data+3,sizeof(data)-3);
    if(len<0) { memcpy(data,"ERR",3); len=0; }
    else memcpy(data,"OK\n",3);
    struct pbuf *out=pbuf_alloc(PBUF_TRANSPORT,(u16_t)(3+len),PBUF_RAM);
    if(out) {
        if(pbuf_take(out,data,(u16_t)(3+len))==ERR_OK)
            udp_sendto(device_pcb,out,&device_peer,device_peer_port);
        pbuf_free(out);
    }
    device_pending=0;
}


int net_service_remote_devctl_init(void) { return device_start(); }
void net_service_remote_devctl_poll(void) { device_process_pending(); }
#else
int net_service_remote_devctl_init(void) { return 0; }
void net_service_remote_devctl_poll(void) {}
#endif
