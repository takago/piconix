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
static struct tcp_pcb *tcp_listener;
static struct tcp_pcb *tcp_client;
static unsigned char tcp_rx[NET_SOCKET_TCP_RX_MAX];
static unsigned tcp_rx_len;
static unsigned char tcp_peer_closed;
static unsigned char tcp_fault;
static unsigned char tcp_connecting;
static unsigned char tcp_established;
static unsigned char tcp_pending_accept;
static err_t socket_tcp_connected_cb(void *arg, struct tcp_pcb *pcb, err_t err) {
    (void)arg; (void)pcb;
    tcp_connecting = 0;
    if (err == ERR_OK) { tcp_established = 1; return ERR_OK; }
    tcp_fault = 1;
    return err;
}
static void socket_tcp_err(void *arg, err_t err) {
    (void)arg; (void)err;
    tcp_client = NULL; /* lwIP already freed the PCB. */
    tcp_fault = 1;
    tcp_connecting = tcp_established = 0;
}
static err_t socket_tcp_recv(void *arg, struct tcp_pcb *pcb,
                             struct pbuf *p, err_t err) {
    (void)arg;
    if (err != ERR_OK) return err;
    if (!p) { tcp_peer_closed = 1; return ERR_OK; }
    /* ERR_MEM leaves ownership with lwIP; it retries when the receive
     * window opens. Never acknowledge bytes before the application reads. */
    if (p->tot_len > NET_SOCKET_TCP_RX_MAX - tcp_rx_len) return ERR_MEM;
    pbuf_copy_partial(p, tcp_rx + tcp_rx_len, p->tot_len, 0);
    tcp_rx_len += p->tot_len;
    pbuf_free(p);
    (void)pcb;
    return ERR_OK;
}
static err_t socket_tcp_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
    (void)arg;
    if (err != ERR_OK || tcp_client) return ERR_VAL;
    tcp_client = pcb;
    tcp_rx_len = 0;
    tcp_peer_closed = tcp_fault = tcp_connecting = 0;
    tcp_established = 1;
    tcp_pending_accept = 1;
    tcp_arg(pcb, NULL);
    tcp_recv(pcb, socket_tcp_recv);
    tcp_err(pcb, socket_tcp_err);
    return ERR_OK;
}
int net_socket_tcp_listen(uint16_t port) {
    if (tcp_listener || !port) return -1;
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb) return -1;
    if (tcp_bind(pcb, IP_ANY_TYPE, port) != ERR_OK) {
        tcp_close(pcb);
        return -1;
    }
    struct tcp_pcb *listener = tcp_listen_with_backlog(pcb, 1);
    if (!listener) { tcp_close(pcb); return -1; }
    tcp_listener = listener;
    tcp_accept(listener, socket_tcp_accept);
    return 0;
}
int net_socket_tcp_connect(uint32_t peer_ip, uint16_t peer_port) {
    if (tcp_listener || tcp_client || !peer_port) return -1;
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb) return -1;
    tcp_client = pcb;
    tcp_rx_len = 0;
    tcp_peer_closed = tcp_fault = tcp_established = 0;
    tcp_connecting = 1;
    tcp_arg(pcb, NULL);
    tcp_recv(pcb, socket_tcp_recv);
    tcp_err(pcb, socket_tcp_err);
    ip_addr_t dest;
    ip_addr_set_ip4_u32(&dest, peer_ip);
    err_t err = tcp_connect(pcb, &dest, peer_port, socket_tcp_connected_cb);
    if (err != ERR_OK) {
        tcp_client = NULL;
        tcp_connecting = 0;
        tcp_abort(pcb);
        return -1;
    }
    return 0;
}
int net_socket_tcp_accept_ready(void) {
    if (!tcp_listener || !tcp_pending_accept || !tcp_client) return 0;
    tcp_pending_accept = 0;
    return 1;
}
int net_socket_tcp_listening(void) { return tcp_listener != NULL; }
void net_socket_tcp_close_listener(void) {
    if (tcp_listener) {
        struct tcp_pcb *pcb = tcp_listener;
        tcp_listener = NULL;
        tcp_accept(pcb, NULL);
        if (tcp_close(pcb) != ERR_OK) tcp_abort(pcb);
    }
}
/* tcp_close() may return ERR_MEM while data is queued. Do not abort in
 * that case: abort sends RST and browsers reject the response. Retry from
 * lwIP's poll/sent callbacks until the graceful FIN can be queued. */
static err_t socket_tcp_closing_poll(void *arg, struct tcp_pcb *pcb) {
    (void)arg;
    if (tcp_close(pcb) == ERR_OK) return ERR_OK;
    return ERR_OK;
}
static err_t socket_tcp_closing_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    (void)arg; (void)len;
    return socket_tcp_closing_poll(NULL, pcb);
}
static err_t socket_tcp_closing_recv(void *arg, struct tcp_pcb *pcb,
                                     struct pbuf *p, err_t err) {
    (void)arg;
    if (p) {
        tcp_recved(pcb, p->tot_len);
        pbuf_free(p);
    }
    return err == ERR_OK ? ERR_OK : err;
}
void net_socket_tcp_close_client(void) {
    if (tcp_client) {
        struct tcp_pcb *pcb = tcp_client;
        tcp_client = NULL;
        tcp_arg(pcb, NULL);
        tcp_err(pcb, NULL);
        tcp_recv(pcb, socket_tcp_closing_recv);
        if (tcp_close(pcb) != ERR_OK) {
            tcp_sent(pcb, socket_tcp_closing_sent);
            tcp_poll(pcb, socket_tcp_closing_poll, 2);
        }
    }
    tcp_rx_len = 0;
    tcp_pending_accept = tcp_peer_closed = tcp_fault = tcp_connecting = tcp_established = 0;
}

int net_socket_tcp_connected(void) { return tcp_client != NULL && tcp_established; }
int net_socket_tcp_status(void) {
    if (tcp_fault) return -1;
    if (tcp_peer_closed && !tcp_rx_len) return -2;
    return tcp_established ? 1 : 0;
}
int net_socket_tcp_read(void *dst, unsigned capacity) {
    if (!dst || !capacity) return -1;
    if (tcp_rx_len) {
        unsigned n = capacity < tcp_rx_len ? capacity : tcp_rx_len;
        memcpy(dst, tcp_rx, n);
        memmove(tcp_rx, tcp_rx + n, tcp_rx_len - n);
        tcp_rx_len -= n;
        if (tcp_client) tcp_recved(tcp_client, (u16_t)n);
        return (int)n;
    }
    if (tcp_peer_closed || tcp_fault) return -2; /* EOF / disconnected */
    return 0; /* Would block. */
}
int net_socket_tcp_write(const void *src, unsigned length) {
    if (!tcp_client || tcp_peer_closed || tcp_fault || (!src && length)) return -1;
    if (!length) return 0;
    unsigned n = length;
    if (n > tcp_sndbuf(tcp_client)) n = tcp_sndbuf(tcp_client);
    if (n > TCP_MSS) n = TCP_MSS;
    if (!n) return 0;
    if (tcp_write(tcp_client, src, (u16_t)n, TCP_WRITE_FLAG_COPY) != ERR_OK) return 0;
    tcp_output(tcp_client);
    return (int)n;
}
void net_socket_tcp_close(void) {
    net_socket_tcp_close_client();
    net_socket_tcp_close_listener();
}

uint16_t net_socket_local_port(int kind) {
    if (kind == 0) return udp_socket ? udp_socket->local_port : 0;
    if (kind == 1) return tcp_listener ? tcp_listener->local_port :
                          (tcp_client ? tcp_client->local_port : 0);
    if (kind == 2) return tcp_client ? tcp_client->local_port : 0;
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
int net_socket_tcp_listen(uint16_t port) { (void)port; return -1; }
int net_socket_tcp_accept_ready(void) { return 0; }
int net_socket_tcp_listening(void) { return 0; }
void net_socket_tcp_close_listener(void) {}
void net_socket_tcp_close_client(void) {}
int net_socket_tcp_connected(void) { return 0; }
int net_socket_tcp_status(void) { return -1; }
int net_socket_tcp_connect(uint32_t ip, uint16_t port) { (void)ip; (void)port; return -1; }
int net_socket_tcp_read(void *dst, unsigned n) { (void)dst; (void)n; return -1; }
int net_socket_tcp_write(const void *src, unsigned n) { (void)src; (void)n; return -1; }
void net_socket_tcp_close(void) {}
uint16_t net_socket_local_port(int kind) { (void)kind; return 0; }
#endif
