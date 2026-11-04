/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Compact IPv4 observation and single-request ICMP Echo, Pico W only. */
#include "net.h"
#ifndef PICONIX_WIFI
#define PICONIX_WIFI 0
#endif
#if PICONIX_WIFI
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/raw.h"
#include "lwip/ip4_addr.h"
#include "lwip/ip4.h"
#include "pico/time.h"
#include <string.h>

#define PACKET_COUNT 16
static piconix_packet_t packets[PACKET_COUNT];
static uint32_t packet_serial;
/* Avoid recording packets before a reader opens the capture device. */
static unsigned packet_readers;
static struct {
    struct netif *netif;
    netif_output_fn original;
} outputs[3];
static struct raw_pcb *echo_pcb;
static piconix_echo_t echo_request, echo_reply;
static uint64_t echo_started;
static unsigned echo_pending, echo_waiting, echo_ready;

static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0]<<8)|p[1]); }
/* The pbuf may be chained, so never access packet bytes past payload. */
static void observe(struct pbuf *p, uint8_t direction) {
    if (!packet_readers || p->tot_len < 20) return;
    uint8_t h[60];
    if (pbuf_copy_partial(p,h,20,0)!=20 || (h[0]>>4)!=4) return;
    unsigned ihl=(h[0]&15u)*4u;
    if (ihl<20 || ihl>60 || p->tot_len<ihl) return;
    unsigned total=be16(h+2);
    if(total<ihl || total>p->tot_len) return;
    unsigned proto=h[9], offset=ihl, transport=0;
    /* Noninitial fragments have no transport header at this offset. */
    unsigned first_fragment=(be16(h+6)&0x1fffu)==0;
    piconix_packet_t rec={0};
    memcpy(rec.src,h+12,4); memcpy(rec.dst,h+16,4);
    rec.ip_len=(uint16_t)total;
    rec.protocol=(uint8_t)proto;
    rec.direction=direction;
    uint8_t th[20];
    if(first_fragment && (proto==6 || proto==17) && total>=ihl+8 &&
            pbuf_copy_partial(p,th,proto==6?20:8,(u16_t)ihl)==(proto==6?20:8)) {
        rec.sport=be16(th); rec.dport=be16(th+2);
        transport=proto==6 ? (unsigned)((th[12]>>4)*4u) : 8u;
        if(proto==6) rec.flags=th[13];
        if(transport<(proto==6?20u:8u) || total<ihl+transport) transport=0;
    } else if(first_fragment && proto==1 && total>=ihl+8) {
        transport=8;
        if(pbuf_copy_partial(p,th,2,(u16_t)ihl)==2) rec.flags=th[0]; /* ICMP type */
    }
    offset+=transport;
    if(transport && total>=offset) {
        rec.data_len=(uint16_t)(total-offset);
        rec.sample_len=(uint8_t)(rec.data_len<16 ? rec.data_len : 16);
        if(rec.sample_len) pbuf_copy_partial(p,rec.sample,rec.sample_len,(u16_t)offset);
    }
    packets[packet_serial%PACKET_COUNT]=rec;
    packet_serial++;
}
int piconix_packet_input(struct pbuf *p, struct netif *n) {
    (void)n;
    observe(p,0);
    return 0; /* allow normal lwIP processing */
}
static err_t observed_output(struct netif *iface, struct pbuf *p, const ip4_addr_t *ip) {
    for(unsigned i=0;i<3;i++) if(outputs[i].netif==iface) {
        observe(p,1);
        return outputs[i].original(iface,p,ip);
    }
    return ERR_IF;
}
void net_capture_attach(struct netif *iface) {
    if(!iface || !iface->output || iface->output==observed_output) return;
    for(unsigned i=0;i<3;i++) if(!outputs[i].netif) {
        outputs[i].netif=iface;
        outputs[i].original=iface->output;
        iface->output=observed_output;
        return;
    }
}
uint32_t net_packet_serial(void) { return packet_serial; }
void net_packet_reader(int delta) { packet_readers=(unsigned)((int)packet_readers+delta); }
int net_packet_read(uint32_t *serial, piconix_packet_t *out) {
    uint32_t oldest=packet_serial>PACKET_COUNT ? packet_serial-PACKET_COUNT : 0;
    if((int32_t)(*serial-oldest)<0) *serial=oldest;
    if(*serial==packet_serial) return 0;
    *out=packets[(*serial)++%PACKET_COUNT];
    return sizeof(*out);
}
static u8_t echo_received(void *arg, struct raw_pcb *pcb, struct pbuf *p,
                           const ip_addr_t *addr) {
    (void)arg; (void)pcb;
    if(!echo_waiting || p->tot_len<28) return 0;
    uint8_t h[28];
    if(pbuf_copy_partial(p,h,sizeof(h),0)!=sizeof(h) || (h[0]>>4)!=4) return 0;
    unsigned ihl=(h[0]&15u)*4u;
    uint8_t icmp[8];
    if(p->tot_len<ihl+8 || pbuf_copy_partial(p,icmp,8,(u16_t)ihl)!=8 || icmp[0]!=0) return 0;
    if(be16(icmp+4)!=echo_request.id || be16(icmp+6)!=echo_request.sequence) return 0;
    const uint8_t *source=(const uint8_t *)&ip_2_ip4(addr)->addr;
    if(memcmp(source,echo_request.address,4)!=0) return 0;
    echo_reply=echo_request;
    echo_reply.milliseconds=(uint32_t)((time_us_64()-echo_started)/1000u);
    echo_ready=1;
    echo_waiting=0;
    return 0; /* raw callback does not own the packet */
}
void net_capture_init(void) {
    echo_pcb=raw_new(IP_PROTO_ICMP);
    if(echo_pcb) raw_recv(echo_pcb,echo_received,0);
}
int net_echo_write(const piconix_echo_t *request) {
    if(!echo_pcb || echo_pending || echo_waiting || echo_ready) return -1;
    echo_request=*request;
    echo_pending=1;
    return sizeof(*request);
}
int net_echo_read(piconix_echo_t *reply) {
    if(!echo_ready) return 0;
    *reply=echo_reply;
    echo_ready=0;
    return sizeof(*reply);
}
void net_echo_close(void) { echo_pending=echo_waiting=echo_ready=0; }
void net_capture_poll(void) {
    if(echo_waiting && time_us_64()-echo_started>=1000000u) {
        echo_waiting=0;
        echo_reply=echo_request;
        echo_reply.milliseconds=0xffffffffu;
        echo_ready=1;
    }
    if(!echo_pending || !echo_pcb) return;
    echo_pending=0;
    uint8_t data[24]={8,0,0,0}; /* ICMP Echo Request, 16-byte sample */
    data[4]=(uint8_t)(echo_request.id>>8); data[5]=(uint8_t)echo_request.id;
    data[6]=(uint8_t)(echo_request.sequence>>8); data[7]=(uint8_t)echo_request.sequence;
    for(unsigned i=8;i<sizeof(data);i++) data[i]=(uint8_t)i;
    uint32_t sum=0;
    for(unsigned i=0;i<sizeof(data);i+=2) sum+=((unsigned)data[i]<<8)|data[i+1];
    while(sum>>16) sum=(sum&0xffffu)+(sum>>16);
    uint16_t check=(uint16_t)~sum;
    data[2]=(uint8_t)(check>>8); data[3]=(uint8_t)check;
    struct pbuf *p=pbuf_alloc(PBUF_IP,sizeof(data),PBUF_RAM);
    if(!p) return;
    ip_addr_t dest;
    IP_ADDR4(&dest,echo_request.address[0],echo_request.address[1],
             echo_request.address[2],echo_request.address[3]);
    if(pbuf_take(p,data,sizeof(data))==ERR_OK) {
        echo_started=time_us_64();
        if(raw_sendto(echo_pcb,p,&dest)==ERR_OK) echo_waiting=1;
    }
    pbuf_free(p);
}
#else
void net_capture_init(void) {}
void net_capture_poll(void) {}
void net_capture_attach(void *iface) { (void)iface; }
uint32_t net_packet_serial(void) { return 0; }
void net_packet_reader(int delta) { (void)delta; }
int net_packet_read(uint32_t *serial,piconix_packet_t *out) { (void)serial;(void)out;return -1; }
int net_echo_write(const piconix_echo_t *request) { (void)request;return -1; }
int net_echo_read(piconix_echo_t *reply) { (void)reply;return -1; }
void net_echo_close(void) {}
#endif
