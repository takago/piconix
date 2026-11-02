#!/usr/bin/env python3
"""Host-compile the real TCP callback sources against a small lwIP test double.

Tests include full advertised receive windows, refused/retried pbufs, and the
mandatory ERR_ABRT return when a receive callback aborts its TCP PCB.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HEADER = r'''
#ifndef TEST_LWIP_MOCK
#define TEST_LWIP_MOCK
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef int err_t;
typedef uint16_t u16_t;
#define ERR_OK 0
#define ERR_MEM (-1)
#define ERR_VAL (-6)
#define ERR_ABRT (-13)
#define ERR_RST (-14)
#define IPADDR_TYPE_V4 1
#define IP_ANY_TYPE ((const ip_addr_t *)0)
#define TCP_MSS 536
#define TCP_WND (2 * TCP_MSS)
#define TCP_WRITE_FLAG_COPY 1
#define SOF_REUSEADDR 0x04
#define IP_IS_V4(addr) (1)
typedef struct { uint32_t addr; } ip_addr_t;
typedef ip_addr_t ip4_addr_t;
#define ip_2_ip4(addr) (addr)
static inline uint32_t ip4_addr_get_u32(const ip4_addr_t *ip) { return ip->addr; }
static inline void ip_addr_set_ip4_u32(ip_addr_t *ip, uint32_t val) { ip->addr = val; }
struct pbuf {
    struct pbuf *next;
    void *payload;
    u16_t len, tot_len;
};
struct udp_pcb { u16_t local_port; };
struct tcp_pcb;
typedef err_t (*tcp_recv_fn)(void *, struct tcp_pcb *, struct pbuf *, err_t);
typedef err_t (*tcp_accept_fn)(void *, struct tcp_pcb *, err_t);
typedef err_t (*tcp_connected_fn)(void *, struct tcp_pcb *, err_t);
typedef err_t (*tcp_poll_fn)(void *, struct tcp_pcb *);
typedef void (*tcp_err_fn)(void *, err_t);
struct tcp_pcb {
    u16_t local_port, sndbuf;
    ip_addr_t remote_ip;
    void *arg;
    tcp_recv_fn recv;
    tcp_accept_fn accept;
    tcp_connected_fn connected;
    tcp_poll_fn poll;
    tcp_err_fn error;
};
#define PBUF_TRANSPORT 0
#define PBUF_RAM 0
void pbuf_free(struct pbuf *p);
u16_t pbuf_copy_partial(const struct pbuf *p, void *dst, u16_t n, u16_t off);
struct pbuf *pbuf_alloc(int, u16_t, int);
err_t pbuf_take(struct pbuf *, const void *, u16_t);
struct udp_pcb *udp_new_ip_type(int);
err_t udp_bind(struct udp_pcb *, const ip_addr_t *, u16_t);
void udp_remove(struct udp_pcb *);
void udp_recv(struct udp_pcb *, void (*)(void *, struct udp_pcb *, struct pbuf *, const ip_addr_t *, u16_t), void *);
err_t udp_sendto(struct udp_pcb *, struct pbuf *, const ip_addr_t *, u16_t);
struct tcp_pcb *tcp_new_ip_type(int);
void ip_set_option(struct tcp_pcb *, int);
err_t tcp_bind(struct tcp_pcb *, const ip_addr_t *, u16_t);
struct tcp_pcb *tcp_listen_with_backlog(struct tcp_pcb *, int);
void tcp_arg(struct tcp_pcb *, void *);
void tcp_accept(struct tcp_pcb *, tcp_accept_fn);
void tcp_recv(struct tcp_pcb *, tcp_recv_fn);
void tcp_err(struct tcp_pcb *, tcp_err_fn);
void tcp_sent(struct tcp_pcb *, void *);
void tcp_poll(struct tcp_pcb *, tcp_poll_fn, int);
err_t tcp_connect(struct tcp_pcb *, const ip_addr_t *, u16_t, tcp_connected_fn);
err_t tcp_close(struct tcp_pcb *);
void tcp_abort(struct tcp_pcb *);
void tcp_recved(struct tcp_pcb *, u16_t);
u16_t tcp_sndbuf(struct tcp_pcb *);
err_t tcp_write(struct tcp_pcb *, const void *, u16_t, int);
err_t tcp_output(struct tcp_pcb *);
err_t tcp_shutdown(struct tcp_pcb *, int, int);
#endif
'''

HARNESS = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "net_socket.c"
#include "net_service_telnet.c"
static unsigned freed, credited, aborted, closed, retry_close;
static unsigned enqueued, outputs, shutdowns;
static err_t write_result=ERR_OK, output_result=ERR_OK, shutdown_result=ERR_OK;
void pbuf_free(struct pbuf *p) { (void)p; ++freed; }
u16_t pbuf_copy_partial(const struct pbuf *p, void *dst, u16_t n, u16_t off) {
    assert(off + n <= p->tot_len); memcpy(dst, (const char *)p->payload + off, n); return n;
}
void tcp_recved(struct tcp_pcb *p, u16_t n) { (void)p; credited += n; }
err_t tcp_close(struct tcp_pcb *p) { (void)p; ++closed; return retry_close ? ERR_MEM : ERR_OK; }
void tcp_abort(struct tcp_pcb *p) { (void)p; ++aborted; }
void tcp_arg(struct tcp_pcb *p, void *arg) { p->arg=arg; }
void tcp_recv(struct tcp_pcb *p, tcp_recv_fn fn) { p->recv=fn; }
void tcp_err(struct tcp_pcb *p, tcp_err_fn fn) { p->error=fn; }
void tcp_poll(struct tcp_pcb *p, tcp_poll_fn fn, int n) { (void)n; p->poll=fn; }
void tcp_sent(struct tcp_pcb *p, void *fn) { (void)p; (void)fn; }
u16_t tcp_sndbuf(struct tcp_pcb *p) { return p->sndbuf; }
err_t tcp_write(struct tcp_pcb *p, const void *src, u16_t n, int flag) {
    (void)p; (void)src; assert(n<=TCP_MSS && flag==TCP_WRITE_FLAG_COPY);
    ++enqueued;
    return write_result;
}
err_t tcp_output(struct tcp_pcb *p) { (void)p; ++outputs; return output_result; }
err_t tcp_shutdown(struct tcp_pcb *p,int rd,int wr) {
    (void)p; assert(rd==0 && wr==1); ++shutdowns; return shutdown_result;
}

int main(void) {
    assert(NET_SOCKET_TCP_RX_MAX >= TCP_WND);
    int id = net_socket_tcp_alloc();
    assert(id >= 0);
    tcp_slot_t *s=slot_at(id);
    struct tcp_pcb pcb={0};
    s->client=&pcb; s->established=1;
    unsigned char source[TCP_WND];
    for(unsigned i=0;i<sizeof(source);i++) source[i]=(unsigned char)(i % 253);
    struct pbuf p={.payload=source,.len=sizeof(source),.tot_len=sizeof(source)};
    assert(socket_tcp_recv(s,&pcb,&p,ERR_OK)==ERR_OK);
    assert(freed==1 && s->rx_len==TCP_WND && credited==0);
    unsigned char dest[TCP_WND];
    assert(net_socket_tcp_read(id,dest,256)==256);
    assert(!memcmp(dest,source,256) && credited==256);
    /* Backpressure preserves the pbuf until enough bytes are drained. */
    struct pbuf p2={.payload=source,.len=536,.tot_len=536};
    assert(socket_tcp_recv(s,&pcb,&p2,ERR_OK)==ERR_MEM);
    assert(freed==1);
    assert(net_socket_tcp_read(id,dest,536)==536);
    assert(socket_tcp_recv(s,&pcb,&p2,ERR_OK)==ERR_OK);
    assert(freed==2);
    assert(net_socket_tcp_read(id,dest,sizeof(dest))==816);
    assert(!memcmp(dest,source+792,280));
    assert(!memcmp(dest+280,source,536));
    assert(credited==1608);
    /* The asynchronous error callback must clear a stale accept flag. */
    s->pending_accept=1;
    socket_tcp_err(s,ERR_RST);
    assert(s->client==NULL && !s->pending_accept && s->fault);
    /* When normal close fails, the fallback poll retries without a slot. */
    s->client=&pcb; retry_close=1;
    net_socket_tcp_close_client(id);
    assert(s->client==NULL && pcb.poll && closed==1 && aborted==0);
    retry_close=0;
    assert(pcb.poll(NULL,&pcb)==ERR_OK && closed==2);
    /* A late pbuf belongs to the detached closing PCB, not a reused slot. */
    struct pbuf late={.payload=source,.len=4,.tot_len=4};
    unsigned freed_before=freed, credited_before=credited;
    assert(socket_tcp_closing_recv(NULL,&pcb,&late,ERR_VAL)==ERR_OK);
    assert(freed==freed_before+1 && credited==credited_before+4);
    assert(socket_tcp_closing_recv(NULL,&pcb,NULL,ERR_OK)==ERR_OK);
    /* Send queue tests: no write while connecting or after half-close. */
    s->client=&pcb; s->established=0; s->fault=0; s->peer_closed=0;
    pcb.sndbuf=TCP_MSS;
    unsigned writes_before=enqueued;
    assert(net_socket_tcp_write(id,source,20)==-1 && enqueued==writes_before);
    s->established=1; pcb.sndbuf=0;
    assert(net_socket_tcp_write(id,source,20)==0 && enqueued==writes_before);
    pcb.sndbuf=TCP_MSS;
    write_result=ERR_MEM;
    assert(net_socket_tcp_write(id,source,20)==0 && outputs==0);
    write_result=ERR_VAL;
    assert(net_socket_tcp_write(id,source,20)==-1 && outputs==0);
    write_result=ERR_OK; output_result=ERR_MEM;
    assert(net_socket_tcp_write(id,source,1024)==TCP_MSS);
    assert(outputs==1); /* Data is queued even when tcp_output fails. */
    shutdown_result=ERR_MEM;
    assert(net_socket_tcp_shutdown_write(id)==-1 && !s->write_shutdown);
    shutdown_result=ERR_OK;
    assert(net_socket_tcp_shutdown_write(id)==0 && s->write_shutdown);
    writes_before=enqueued;
    assert(net_socket_tcp_write(id,source,1)==-1 && enqueued==writes_before);
    assert(net_socket_tcp_shutdown_write(id)==-1);
    net_socket_tcp_close_client(id);
    assert(!s->write_shutdown);
    /* Telnet must tell lwIP when its receive callback has aborted a PCB. */
    client=&pcb; retry_close=1;
    assert(client_recv(NULL,&pcb,NULL,ERR_OK)==ERR_ABRT);
    assert(client==NULL && aborted==1);
    client=&pcb; retry_close=0;
    assert(client_recv(NULL,&pcb,NULL,ERR_OK)==ERR_OK);
    assert(client==NULL && aborted==1);
    /* Non-OK input with a pbuf is still consumed and must return ERR_OK. */
    struct pbuf bad={.payload=source,.len=1,.tot_len=1};
    unsigned prior_freed=freed;
    assert(client_recv(NULL,&pcb,&bad,ERR_VAL)==ERR_OK);
    assert(freed==prior_freed+1);
    puts("PASS: TCP callbacks, receive/backpressure, send queue, half-close, abort");
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="piconix_tcp_") as temp:
    temp = Path(temp)
    (temp / "lwip").mkdir()
    for name in ("udp.h", "pbuf.h", "ip4_addr.h", "tcp.h"):
        (temp / "lwip" / name).write_text('#include "../mock.h"\n')
    (temp / "mock.h").write_text(HEADER)
    (temp / "test.c").write_text(HARNESS)
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Wno-unused-parameter",
        "-Wno-unused-function", "-ffunction-sections", "-fdata-sections",
        "-DPICONIX_WIFI=1", "-DPICONIX_WIFI_STA=1",
        "-I", str(temp), "-I", str(ROOT / "kernel"),
        str(temp / "test.c"), "-Wl,--gc-sections", "-o", str(temp / "test"),
    ], check=True)
    subprocess.run([str(temp / "test")], check=True)
