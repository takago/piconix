/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "net.h"
#include "net_services.h"
#ifndef PICONIX_WIFI
#define PICONIX_WIFI 0
#endif
#if PICONIX_WIFI
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/ip4_addr.h"
#include <stdint.h>
#include <string.h>
static struct tcp_pcb *telnet_listener; /* Telnet :23 */
static unsigned char client_telnet;
static unsigned char telnet_state, telnet_command, telnet_cr;
static unsigned char telnet_hello_pos;
static const unsigned char telnet_hello[] = {255,251,1, 255,251,3, 255,253,3}; /* WILL ECHO, WILL SGA, DO SGA */
static struct tcp_pcb *client;
/* Distinguish sessions even if a disconnect/reconnect occurs between idle polls. */
static unsigned terminal_epoch;
/* Small fixed-size queues; no heap allocation for terminal bytes.
 * Only net_poll / lwIP callbacks access these queues in this stage. */
/* The RX queue must fit a full negotiated TCP segment (TCP_MSS=536).
 * A 256-byte queue can never accept a 536-byte pbuf, causing a permanent
 * zero-window stall because lwIP retries that same pbuf unchanged. */
#define NET_TTY_QUEUE_SIZE 1024u
static unsigned char net_rx[NET_TTY_QUEUE_SIZE], net_tx[NET_TTY_QUEUE_SIZE];
static unsigned net_rx_head, net_rx_tail, net_tx_head, net_tx_tail;
static unsigned queue_count(unsigned head, unsigned tail) {
    return (head - tail) & (NET_TTY_QUEUE_SIZE - 1u);
}
static unsigned queue_space(unsigned head, unsigned tail) {
    return NET_TTY_QUEUE_SIZE - 1u - queue_count(head, tail);
}
static void net_queues_reset(void) {
    net_rx_head = net_rx_tail = net_tx_head = net_tx_tail = 0;
}
int net_terminal_connected(void) { return client != NULL; }
unsigned net_terminal_epoch(void) { return terminal_epoch; }
/* Returns -1 when disconnected, 0 when empty, otherwise bytes read. */
int net_terminal_read(void *dst, unsigned size) {
    if (!client) return -1;
    unsigned char *out = dst;
    unsigned n = 0;
    while (n < size && net_rx_tail != net_rx_head) {
        out[n++] = net_rx[net_rx_tail];
        net_rx_tail = (net_rx_tail + 1u) & (NET_TTY_QUEUE_SIZE - 1u);
    }
    return (int)n;
}
/* Returns -1 when disconnected; otherwise number of queued bytes. */
int net_terminal_write(const void *src, unsigned size) {
    if (!client) return -1;
    /* A TTY write must never queue only the CR of a CRLF pair. */
    unsigned needed = size;
    if (client_telnet) {
        const unsigned char *b = src;
        for (unsigned i=0; i<size; ++i) if (b[i] == 255) ++needed;
    }
    if (needed > queue_space(net_tx_head, net_tx_tail)) return 0;
    const unsigned char *in = src;
    unsigned n = 0;
    while (n < size && queue_space(net_tx_head, net_tx_tail)) {
        unsigned char c = in[n++];
        net_tx[net_tx_head] = c;
        net_tx_head = (net_tx_head + 1u) & (NET_TTY_QUEUE_SIZE - 1u);
        if (client_telnet && c == 255) {
            net_tx[net_tx_head] = 255;
            net_tx_head = (net_tx_head + 1u) & (NET_TTY_QUEUE_SIZE - 1u);
        }
    }
    return (int)n;
}
static void net_terminal_flush(void) {
    if (!client) return;
    if (client_telnet && telnet_hello_pos < sizeof(telnet_hello)) {
        unsigned n = sizeof(telnet_hello) - telnet_hello_pos;
        if (n > tcp_sndbuf(client)) n = tcp_sndbuf(client);
        if (n && tcp_write(client, telnet_hello + telnet_hello_pos,
                           (u16_t)n, TCP_WRITE_FLAG_COPY) == ERR_OK) {
            telnet_hello_pos += n;
            tcp_output(client);
        }
        return;
    }
    if (net_tx_tail == net_tx_head) return;
    unsigned available = queue_count(net_tx_head, net_tx_tail);
    unsigned contiguous = NET_TTY_QUEUE_SIZE - net_tx_tail;
    if (available > contiguous) available = contiguous;
    if (available > tcp_sndbuf(client)) available = tcp_sndbuf(client);
    /* Keep each enqueue within a single MSS.  tcp_sndbuf() alone does not
     * account for the limited number of lwIP TCP segment descriptors. */
    if (available > TCP_MSS) available = TCP_MSS;
    if (available && tcp_write(client, net_tx + net_tx_tail,
                               (u16_t)available, TCP_WRITE_FLAG_COPY) == ERR_OK) {
        net_tx_tail = (net_tx_tail + available) & (NET_TTY_QUEUE_SIZE - 1u);
        tcp_output(client);
    }
}

static void client_error(void *arg, err_t err) {
    (void)arg;
    (void)err;
    client = NULL; /* lwIP already destroyed the PCB. */
    ++terminal_epoch;
    net_queues_reset();
}

/* Telnet negotiation is handled here; no lwIP output inside callbacks.
 * Ignore unsupported options, and normalize Telnet CR NUL / CR LF. */
static void net_receive_byte(unsigned char c) {
    if (client_telnet) {
        if (telnet_state == 1) {
            telnet_state = 0;
            if (c == 255) { /* escaped IAC data */ }
            else if (c >= 251 && c <= 254) { telnet_command = c; telnet_state = 2; return; }
            else if (c == 250) { telnet_state = 3; return; }
            else return;
        } else if (telnet_state == 2) {
            /* ECHO/SGA were proposed on connection; ignore peer replies. */
            (void)telnet_command;
            telnet_state = 0;
            return;
        } else if (telnet_state == 3) {
            if (c == 255) telnet_state = 4;
            return;
        } else if (telnet_state == 4) {
            telnet_state = c == 240 ? 0 : 3;
            return;
        } else if (c == 255) { telnet_state = 1; return; }
        if (telnet_cr) {
            telnet_cr = 0;
            if (c == 0 || c == '\\n') return;
        }
        if (c == '\\r') telnet_cr = 1;
    }
    net_rx[net_rx_head] = c;
    net_rx_head = (net_rx_head + 1u) & (NET_TTY_QUEUE_SIZE - 1u);
}

static err_t client_recv(void *arg, struct tcp_pcb *pcb,
                         struct pbuf *p, err_t err) {
    (void)arg;
    if (!p) {
        tcp_arg(pcb, NULL);
        tcp_recv(pcb, NULL);
        tcp_err(pcb, NULL);
        client = NULL;
        ++terminal_epoch;
        net_queues_reset();
        /* tcp_close may fail while unsent data remains. The PCB must not
         * survive with callbacks removed and the session forgotten. */
        if (tcp_close(pcb) != ERR_OK) tcp_abort(pcb);
        return ERR_OK;
    }
    if (err != ERR_OK) {
        pbuf_free(p);
        return err;
    }
    /* Do not acknowledge bytes that cannot fit the RX ring.
     * lwIP retains its TCP receive window until these bytes can be accepted.
     * For now reject an oversized segment instead of silently losing data. */
    if (p->tot_len > queue_space(net_rx_head, net_rx_tail)) {
        return ERR_MEM; /* lwIP retries the same pbuf later. */
    }
    for (struct pbuf *q = p; q; q = q->next) {
        const unsigned char *data = q->payload;
        for (unsigned i = 0; i < q->len; ++i) {
            net_receive_byte(data[i]);
        }
    }
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static err_t client_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
    ip4_addr_t allowed;
    IP4_ADDR(&allowed, 192, 168, 4, 2);
    /* Static-IP clients must be allowed even without a DHCP lease. */
    if (err != ERR_OK || client ||
        !IP_IS_V4(&pcb->remote_ip) ||
        !ip4_addr_cmp(ip_2_ip4(&pcb->remote_ip), &allowed)) {
        tcp_abort(pcb);
        return ERR_ABRT;
    }
    client = pcb;
    client_telnet = (unsigned char)(uintptr_t)arg;
    telnet_state = telnet_command = telnet_cr = telnet_hello_pos = 0;
    ++terminal_epoch;
    net_queues_reset();
    tcp_arg(pcb, NULL);
    tcp_err(pcb, client_error);
    tcp_recv(pcb, client_recv);
    return ERR_OK;
}


int net_service_telnet_init(void) {
    telnet_listener = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (telnet_listener) {
        if (tcp_bind(telnet_listener, IP_ANY_TYPE, 23) != ERR_OK) {
            tcp_close(telnet_listener);
            telnet_listener = NULL;
        } else {
            struct tcp_pcb *listening = tcp_listen_with_backlog(telnet_listener, 1);
            telnet_listener = listening;
            if (listening) {
                tcp_arg(listening, (void *)(uintptr_t)1);
                tcp_accept(listening, client_accept);
            }
        }
    }
    return telnet_listener != NULL;
}
void net_service_telnet_poll(void) { net_terminal_flush(); }
#else
int net_service_telnet_init(void) { return 0; }
void net_service_telnet_poll(void) {}
int net_terminal_connected(void) { return 0; }
unsigned net_terminal_epoch(void) { return 0; }
int net_terminal_read(void *dst, unsigned size) { (void)dst; (void)size; return -1; }
int net_terminal_write(const void *src, unsigned size) { (void)src; (void)size; return -1; }
#endif
