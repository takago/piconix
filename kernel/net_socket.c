/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "net_socket.h"
#ifndef PICONIX_WIFI
#define PICONIX_WIFI 0
#endif
#if PICONIX_WIFI
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/ip4_addr.h"
#include <string.h>
/* Single datagram slot: bounded RAM, no allocation in receive callback. */
static struct udp_pcb *udp_socket;
static unsigned char rx_data[NET_SOCKET_UDP_MAX];
static unsigned rx_length;
static uint32_t rx_peer_ip;
static uint16_t rx_peer_port;
static unsigned rx_ready;
static void udp_received(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                         const ip_addr_t *addr, u16_t port) {
    (void)arg; (void)pcb;
    if (!p) return;
    if (!rx_ready && IP_IS_V4(addr) && p->tot_len <= sizeof(rx_data)) {
        rx_length = pbuf_copy_partial(p, rx_data, p->tot_len, 0);
        rx_peer_ip = ip4_addr_get_u32(ip_2_ip4(addr));
        rx_peer_port = port;
        rx_ready = 1;
    }
    pbuf_free(p);
}
int net_socket_udp_bind(uint16_t port) {
    if (udp_socket || !port) return -1;
    struct udp_pcb *pcb = udp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb) return -1;
    if (udp_bind(pcb, IP_ANY_TYPE, port) != ERR_OK) {
        udp_remove(pcb);
        return -1;
    }
    udp_socket = pcb;
    rx_ready = 0;
    udp_recv(pcb, udp_received, NULL);
    return 0;
}
int net_socket_udp_recv(void *buf, unsigned capacity, uint32_t *peer_ip, uint16_t *peer_port) {
    if (!buf) return -1;
    /* A UDP client has no PCB until its first send; this is not an error. */
    if (!udp_socket) return -3;
    if (!rx_ready) return -3; /* No datagram; zero-length datagrams are valid. */
    if (capacity < rx_length) return -2; /* Retain datagram for retry. */
    memcpy(buf, rx_data, rx_length);
    if (peer_ip) *peer_ip = rx_peer_ip;
    if (peer_port) *peer_port = rx_peer_port;
    unsigned n = rx_length;
    rx_ready = 0;
    return (int)n;
}
int net_socket_udp_send(const void *buf, unsigned length, uint32_t peer_ip, uint16_t peer_port) {
    if ((!buf && length) || length > NET_SOCKET_UDP_MAX || !peer_port) return -1;
    /* Client mode: allocate and bind an ephemeral local port on first send. */
    if (!udp_socket) {
        struct udp_pcb *pcb = udp_new_ip_type(IPADDR_TYPE_V4);
        if (!pcb) return -1;
        if (udp_bind(pcb, IP_ANY_TYPE, 0) != ERR_OK) {
            udp_remove(pcb);
            return -1;
        }
        udp_socket = pcb;
        rx_ready = 0;
        udp_recv(pcb, udp_received, NULL);
    }
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, length, PBUF_RAM);
    if (!p) return -1;
    if (length && pbuf_take(p, buf, length) != ERR_OK) { pbuf_free(p); return -1; }
    ip_addr_t dest;
    ip_addr_set_ip4_u32(&dest, peer_ip);
    err_t err = udp_sendto(udp_socket, p, &dest, peer_port);
    pbuf_free(p);
    return err == ERR_OK ? (int)length : -1;
}
void net_socket_udp_close(void) {
    if (udp_socket) { udp_remove(udp_socket); udp_socket = NULL; }
    rx_ready = 0;
}
/* Experimental TCP transport backend for the PicoNIX socket syscalls. */
#include "lwip/tcp.h"
#define NET_SOCKET_TCP_RX_MAX 1024u
#define NET_SOCKET_TCP_SLOTS 4
typedef struct {
    struct tcp_pcb *listener, *client;
    unsigned char rx[NET_SOCKET_TCP_RX_MAX];
    unsigned rx_len;
    unsigned char peer_closed, fault, connecting, established, pending_accept, allocated;
} tcp_slot_t;
static tcp_slot_t slots[NET_SOCKET_TCP_SLOTS];
static tcp_slot_t *slot_at(int id) {
    return id>=0 && id<NET_SOCKET_TCP_SLOTS ? &slots[id] : NULL;
}
int net_socket_tcp_alloc(void) {
    for(int i=0;i<NET_SOCKET_TCP_SLOTS;i++)
        if(!slots[i].allocated) {
            memset(&slots[i],0,sizeof slots[i]);
            slots[i].allocated=1;
            return i;
        }
    return -1;
}
static err_t socket_tcp_connected_cb(void *arg, struct tcp_pcb *pcb, err_t err) {
    (void)pcb;
    tcp_slot_t *s=(tcp_slot_t *)arg;
    s->connecting = 0;
    if (err == ERR_OK) { s->established = 1; return ERR_OK; }
    s->fault = 1;
    return err;
}
static void socket_tcp_err(void *arg, err_t err) {
    (void)err;
    tcp_slot_t *s=(tcp_slot_t *)arg;
    s->client = NULL; /* lwIP already freed the PCB. */
    s->fault = 1;
    s->connecting = s->established = 0;
}
static err_t socket_tcp_recv(void *arg, struct tcp_pcb *pcb,
                             struct pbuf *p, err_t err) {
    tcp_slot_t *s=(tcp_slot_t *)arg;
    if (err != ERR_OK) return err;
    if (!p) { s->peer_closed = 1; return ERR_OK; }
    /* ERR_MEM leaves ownership with lwIP; it retries when the receive
     * window opens. Never acknowledge bytes before the application reads. */
    if (p->tot_len > NET_SOCKET_TCP_RX_MAX - s->rx_len) return ERR_MEM;
    pbuf_copy_partial(p, s->rx + s->rx_len, p->tot_len, 0);
    s->rx_len += p->tot_len;
    pbuf_free(p);
    (void)pcb;
    return ERR_OK;
}
static err_t socket_tcp_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
    tcp_slot_t *s=(tcp_slot_t *)arg;
    if (err != ERR_OK || s->client) return ERR_VAL;
    s->client = pcb;
    s->rx_len = 0;
    s->peer_closed = s->fault = s->connecting = 0;
    s->established = 1;
    s->pending_accept = 1;
    tcp_arg(pcb, s);
    tcp_recv(pcb, socket_tcp_recv);
    tcp_err(pcb, socket_tcp_err);
    return ERR_OK;
}
int net_socket_tcp_listen(int id, uint16_t port) {
    tcp_slot_t *s=slot_at(id);
    if(!s || !s->allocated || s->listener || s->client || !port) return -1;
    struct tcp_pcb *pcb=tcp_new_ip_type(IPADDR_TYPE_V4);
    if(!pcb) return -1;
    if(tcp_bind(pcb,IP_ANY_TYPE,port)!=ERR_OK) { tcp_close(pcb); return -1; }
    struct tcp_pcb *listener=tcp_listen_with_backlog(pcb,1);
    if(!listener) { tcp_close(pcb); return -1; }
    s->listener=listener;
    tcp_arg(listener,s);
    tcp_accept(listener,socket_tcp_accept);
    return 0;
}
int net_socket_tcp_connect(int id,uint32_t ip,uint16_t port) {
    tcp_slot_t *s=slot_at(id);
    if(!s || !s->allocated || s->listener || s->client || !port) return -1;
    struct tcp_pcb *pcb=tcp_new_ip_type(IPADDR_TYPE_V4);
    if(!pcb) return -1;
    s->client=pcb;
    s->rx_len=0;
    s->peer_closed=s->fault=s->established=0;
    s->connecting=1;
    tcp_arg(pcb,s);
    tcp_recv(pcb,socket_tcp_recv);
    tcp_err(pcb,socket_tcp_err);
    ip_addr_t dest;
    ip_addr_set_ip4_u32(&dest,ip);
    if(tcp_connect(pcb,&dest,port,socket_tcp_connected_cb)!=ERR_OK) {
        s->client=NULL; s->connecting=0;
        tcp_arg(pcb,NULL); tcp_abort(pcb);
        return -1;
    }
    return 0;
}
int net_socket_tcp_accept_ready(int id) {
    tcp_slot_t *s=slot_at(id);
    if(!s || !s->listener || !s->pending_accept || !s->client) return 0;
    s->pending_accept=0;
    return 1;
}
int net_socket_tcp_listening(int id) { tcp_slot_t *s=slot_at(id); return s && s->listener; }
void net_socket_tcp_close_listener(int id) {
    tcp_slot_t *s=slot_at(id);
    if(!s || !s->listener) return;
    struct tcp_pcb *pcb=s->listener;
    s->listener=NULL;
    tcp_arg(pcb,NULL); tcp_accept(pcb,NULL);
    if(tcp_close(pcb)!=ERR_OK) tcp_abort(pcb);
}
static err_t socket_tcp_closing_poll(void *arg,struct tcp_pcb *pcb) {
    (void)arg;
    (void)tcp_close(pcb);
    return ERR_OK;
}
static err_t socket_tcp_closing_sent(void *arg,struct tcp_pcb *pcb,u16_t len) {
    (void)arg; (void)len;
    return socket_tcp_closing_poll(NULL,pcb);
}
static err_t socket_tcp_closing_recv(void *arg,struct tcp_pcb *pcb,struct pbuf *p,err_t err) {
    (void)arg;
    if(p) { tcp_recved(pcb,p->tot_len); pbuf_free(p); }
    return err==ERR_OK ? ERR_OK : err;
}
void net_socket_tcp_close_client(int id) {
    tcp_slot_t *s=slot_at(id);
    if(!s) return;
    if(s->client) {
        struct tcp_pcb *pcb=s->client;
        s->client=NULL;
        tcp_arg(pcb,NULL); tcp_err(pcb,NULL);
        tcp_recv(pcb,socket_tcp_closing_recv);
        if(tcp_close(pcb)!=ERR_OK) {
            tcp_sent(pcb,socket_tcp_closing_sent);
            tcp_poll(pcb,socket_tcp_closing_poll,2);
        }
    }
    s->rx_len=0;
    s->pending_accept=s->peer_closed=s->fault=s->connecting=s->established=0;
}
int net_socket_tcp_connected(int id) { tcp_slot_t *s=slot_at(id); return s && s->client && s->established; }
int net_socket_tcp_status(int id) {
    tcp_slot_t *s=slot_at(id);
    if(!s) return -1;
    if(s->fault) return -1;
    if(s->peer_closed && !s->rx_len) return -2;
    return s->established ? 1 : 0;
}
int net_socket_tcp_read(int id,void *dst,unsigned capacity) {
    tcp_slot_t *s=slot_at(id);
    if(!s || !dst || !capacity) return -1;
    if(s->rx_len) {
        unsigned n=capacity<s->rx_len ? capacity : s->rx_len;
        memcpy(dst,s->rx,n);
        memmove(s->rx,s->rx+n,s->rx_len-n);
        s->rx_len-=n;
        if(s->client) tcp_recved(s->client,(u16_t)n);
        return (int)n;
    }
    return s->peer_closed || s->fault ? -2 : 0;
}
int net_socket_tcp_write(int id,const void *src,unsigned length) {
    tcp_slot_t *s=slot_at(id);
    if(!s || !s->client || s->peer_closed || s->fault || (!src && length)) return -1;
    if(!length) return 0;
    unsigned n=length;
    if(n>tcp_sndbuf(s->client)) n=tcp_sndbuf(s->client);
    if(n>TCP_MSS) n=TCP_MSS;
    if(!n) return 0;
    if(tcp_write(s->client,src,(u16_t)n,TCP_WRITE_FLAG_COPY)!=ERR_OK) return 0;
    tcp_output(s->client);
    return (int)n;
}
void net_socket_tcp_close(int id) {
    net_socket_tcp_close_client(id);
    net_socket_tcp_close_listener(id);
    tcp_slot_t *s=slot_at(id);
    if(s) s->allocated=0;
}
uint16_t net_socket_local_port(int kind,int id) {
    if(kind==0) return udp_socket ? udp_socket->local_port : 0;
    tcp_slot_t *s=slot_at(id);
    if(!s) return 0;
    if(kind==1) return s->listener ? s->listener->local_port : (s->client ? s->client->local_port : 0);
    if(kind==2) return s->client ? s->client->local_port : 0;
    return 0;
}

#else
/* Pico builds without Wi-Fi: no lwIP objects are available. */
int net_socket_udp_bind(uint16_t port) { (void)port; return -1; }
int net_socket_udp_recv(void *b, unsigned n, uint32_t *a, uint16_t *p) {
    (void)b; (void)n; (void)a; (void)p; return -1;
}
int net_socket_udp_send(const void *b, unsigned n, uint32_t a, uint16_t p) {
    (void)b; (void)n; (void)a; (void)p; return -1;
}
void net_socket_udp_close(void) {}
int net_socket_tcp_alloc(void) { return -1; }
int net_socket_tcp_listen(int id,uint16_t port) { (void)id; (void)port; return -1; }
int net_socket_tcp_accept_ready(int id) { (void)id; return 0; }
int net_socket_tcp_listening(int id) { (void)id; return 0; }
void net_socket_tcp_close_listener(int id) { (void)id; }
void net_socket_tcp_close_client(int id) { (void)id; }
int net_socket_tcp_connected(int id) { (void)id; return 0; }
int net_socket_tcp_status(int id) { (void)id; return -1; }
int net_socket_tcp_connect(int id,uint32_t ip,uint16_t port) { (void)id; (void)ip; (void)port; return -1; }
int net_socket_tcp_read(int id,void *dst,unsigned n) { (void)id; (void)dst; (void)n; return -1; }
int net_socket_tcp_write(int id,const void *src,unsigned n) { (void)id; (void)src; (void)n; return -1; }
void net_socket_tcp_close(int id) { (void)id; }
uint16_t net_socket_local_port(int kind,int id) { (void)kind; (void)id; return 0; }
#endif
