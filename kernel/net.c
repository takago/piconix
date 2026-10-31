/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "net.h"
#ifndef PICONIX_WIFI
#define PICONIX_WIFI 0
#endif
#if PICONIX_WIFI
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

/* One DHCP lease and one TCP connection.
 * Stage 18: bounded queues for the independent network TTY. */
static int wifi_ready;
static struct tcp_pcb *listener; /* raw TCP :2323 */
static struct tcp_pcb *telnet_listener; /* Telnet :23 */
static unsigned char client_telnet;
static unsigned char telnet_state, telnet_command, telnet_cr;
static unsigned char telnet_hello_pos;
static const unsigned char telnet_hello[] = {255,251,1, 255,251,3, 255,253,3}; /* WILL ECHO, WILL SGA, DO SGA */
static struct tcp_pcb *client;
/* Distinguish sessions even if a disconnect/reconnect occurs between idle polls. */
static unsigned terminal_epoch;
static struct udp_pcb *dhcp_pcb;
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

#define DHCP_OPTIONS_OFFSET 240u
/* Reply options are fixed; avoid a padded 300-byte stack buffer. */
static const unsigned char dhcp_reply_options[] = {
    53,1,0, 54,4,192,168,4,1,
    51,4,0,0,14,16, 1,4,255,255,255,0,
    28,4,192,168,4,255, 255
};
/* BOOTP/DHCP replies must be at least 300 bytes for broad client compatibility. */
#define DHCP_REPLY_LEN 300u
/* Stage a single DHCP reply during receive processing.  Transmit only
 * after cyw43_arch_poll() returns, as with the TCP terminal TX queue. */
static unsigned char dhcp_pending_reply[DHCP_REPLY_LEN];
static unsigned char dhcp_pending_mac[6];
static unsigned char dhcp_pending_type;
static unsigned dhcp_pending;

static const unsigned char dhcp_cookie[4] = {99, 130, 83, 99};
static unsigned char lease_mac[6];
static int lease_valid;
static unsigned char offer_mac[6];
static int offer_valid;
static uint64_t offer_deadline_us;
#define DHCP_OFFER_TIMEOUT_US (15ull * 1000000ull)

/* A failed/incomplete DHCP handshake must not lock out the next PC. */
static void dhcp_expire_offer(void) {
    if (offer_valid && !lease_valid &&
        (int64_t)(time_us_64() - offer_deadline_us) >= 0)
        offer_valid = 0;
}

/* DHCP packets are BOOTP messages; reply only to a valid Ethernet client.
 * A single client MAC owns the lease until reboot (no expiration state).
 * A renewal from that same client is accepted. */
static void dhcp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                      const ip_addr_t *addr, u16_t port) {
    (void)arg; (void)pcb; (void)addr;
    unsigned char request[DHCP_OPTIONS_OFFSET];
    unsigned char reply[DHCP_REPLY_LEN];
    unsigned char message_type = 0;
    unsigned char requested_ip[4] = {0};
    unsigned char server_id[4] = {0};
    int has_requested_ip = 0, has_server_id = 0;
    unsigned int pos;
    if (!p) return;
    dhcp_expire_offer();
    if (dhcp_pending) goto done; /* Preserve the first staged reply. */
    if (port != 68 || p->tot_len < DHCP_OPTIONS_OFFSET ||
        pbuf_copy_partial(p, request, sizeof(request), 0) != sizeof(request) ||
        request[0] != 1 || request[1] != 1 || request[2] != 6 ||
        memcmp(request + 236, dhcp_cookie, 4) != 0) goto done;
    if (lease_valid && memcmp(lease_mac, request + 28, 6) != 0) goto done;
    /* Do not let a second DISCOVER steal a pending single-client offer. */
    if (!lease_valid && offer_valid &&
        memcmp(offer_mac, request + 28, 6) != 0) goto done;
    pos = DHCP_OPTIONS_OFFSET;
    while (pos < p->tot_len) {
        unsigned char code, len;
        if (pbuf_copy_partial(p, &code, 1, (u16_t)pos) != 1) break;
        ++pos;
        if (code == 255) break;
        if (code == 0) continue;
        if (pos >= p->tot_len ||
            pbuf_copy_partial(p, &len, 1, (u16_t)pos) != 1) break;
        ++pos;
        if (len > p->tot_len - pos) break;
        if (code == 53 && len == 1) {
            /* Ignore ambiguous packets with multiple message-type options. */
            if (message_type) goto done;
            pbuf_copy_partial(p, &message_type, 1, (u16_t)pos);
        }
        if (code == 50 && len == 4) {
            pbuf_copy_partial(p, requested_ip, 4, (u16_t)pos);
            has_requested_ip = 1;
        }
        if (code == 54 && len == 4) {
            pbuf_copy_partial(p, server_id, 4, (u16_t)pos);
            has_server_id = 1;
        }
        pos += len;
    }
    if (message_type != 1 && message_type != 3) goto done;
    /* Only commit a lease after REQUEST/ACK, not after DISCOVER/OFFER.
     * Before commitment, only the client offered the address may REQUEST. */
    if (message_type == 3 && !lease_valid &&
        (!offer_valid || memcmp(offer_mac, request + 28, 6) != 0)) goto done;
    if (has_requested_ip &&
        memcmp(requested_ip, (const unsigned char[]){192,168,4,2}, 4) != 0)
        goto done;
    if (has_server_id &&
        memcmp(server_id, (const unsigned char[]){192,168,4,1}, 4) != 0)
        goto done;
    /* Renewals without option 50 must refer to the leased address. */
    if (message_type == 3 && !has_requested_ip &&
        memcmp(request + 12, (const unsigned char[]){192,168,4,2}, 4) != 0)
        goto done;
    memset(reply, 0, sizeof(reply));
    reply[0] = 2; reply[1] = 1; reply[2] = 6;
    memcpy(reply + 4, request + 4, 4); /* xid */
    memcpy(reply + 10, request + 10, 2); /* flags */
    memcpy(reply + 28, request + 28, 16); /* chaddr */
    reply[16] = 192; reply[17] = 168; reply[18] = 4; reply[19] = 2;
    reply[20] = 192; reply[21] = 168; reply[22] = 4; reply[23] = 1;
    memcpy(reply + 236, dhcp_cookie, 4);
    memcpy(reply + DHCP_OPTIONS_OFFSET, dhcp_reply_options,
           sizeof(dhcp_reply_options));
    reply[DHCP_OPTIONS_OFFSET + 2] = message_type == 1 ? 2 : 5;
    /* Do not transmit from the lwIP receive callback. */
    memcpy(dhcp_pending_reply, reply, sizeof(reply));
    memcpy(dhcp_pending_mac, request + 28, 6);
    dhcp_pending_type = message_type;
    dhcp_pending = 1;
    goto done;
done:
    pbuf_free(p);
}

static int dhcp_start(void) {
    ip_addr_t any;
    IP_ADDR4(&any, 0, 0, 0, 0);
    dhcp_pcb = udp_new_ip_type(IPADDR_TYPE_V4);
    if (!dhcp_pcb) return 0;
    if (udp_bind(dhcp_pcb, &any, 67) != ERR_OK) {
        udp_remove(dhcp_pcb);
        dhcp_pcb = NULL;
        return 0;
    }
    /* DHCP OFFER/ACK are sent to the IPv4 broadcast address. */
    ip_set_option(dhcp_pcb, SOF_BROADCAST);
    udp_recv(dhcp_pcb, dhcp_recv, NULL);
    return 1;
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

extern uint32_t piconix_flash_id(void);
void net_init(void) {
    if (cyw43_arch_init() != 0) {
        puts("PicoNIX: CYW43 initialization failed");
        return;
    }
    char ssid[20];
    static const char hex[] = "0123456789ABCDEF";
    uint32_t id = piconix_flash_id();
    memcpy(ssid, "PicoNIX-", 8);
    for (unsigned i=0; i<8; ++i)
        ssid[8+i] = hex[(id >> (28-4*i)) & 15u];
    ssid[16] = 0;
    cyw43_arch_enable_ap_mode(ssid, "piconix1234", CYW43_AUTH_WPA2_AES_PSK);
    ip4_addr_t ip, mask, gw;
    IP4_ADDR(&ip, 192, 168, 4, 1);
    IP4_ADDR(&mask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 4, 1);
    netif_set_addr(&cyw43_state.netif[CYW43_ITF_AP], &ip, &mask, &gw);
    /* Both ports share one TTY and one active client. */
    for (unsigned which = 0; which < 2; ++which) {
        struct tcp_pcb **slot = which ? &telnet_listener : &listener;
        *slot = tcp_new_ip_type(IPADDR_TYPE_V4);
        if (!*slot) continue;
        if (tcp_bind(*slot, IP_ANY_TYPE, which ? 23 : 2323) != ERR_OK) {
            tcp_close(*slot);
            *slot = NULL;
            continue;
        }
        struct tcp_pcb *listening = tcp_listen_with_backlog(*slot, 1);
        *slot = listening; /* tcp_listen frees its input on failure. */
        if (listening) {
            tcp_arg(listening, (void *)(uintptr_t)which);
            tcp_accept(listening, client_accept);
        }
    }
    int dhcp_ready = dhcp_start();
    /* Start servicing the stack only after AP and listener setup. */
    wifi_ready = 1;
    puts(dhcp_ready ? "PicoNIX: DHCP server active (192.168.4.2)"
                    : "PicoNIX: DHCP server initialization failed");
    puts(listener ? "PicoNIX: raw TCP terminal port 2323" : "PicoNIX: raw TCP listener failed");
    puts(telnet_listener ? "PicoNIX: Telnet terminal port 23" : "PicoNIX: Telnet listener failed");
}
/* Called outside cyw43_arch_poll(), never from an lwIP callback. */
static void dhcp_send_pending(void) {
    if (!dhcp_pending || !dhcp_pcb) return;
    struct pbuf *out = pbuf_alloc(PBUF_TRANSPORT, DHCP_REPLY_LEN, PBUF_RAM);
    if (!out) {
        dhcp_pending = 0;
        return;
    }
    err_t result = pbuf_take(out, dhcp_pending_reply, DHCP_REPLY_LEN);
    if (result == ERR_OK) {
        ip_addr_t broadcast;
        IP_ADDR4(&broadcast, 255, 255, 255, 255);
        result = udp_sendto(dhcp_pcb, out, &broadcast, 68);
    }
    pbuf_free(out);
    if (result == ERR_OK) {
        if (dhcp_pending_type == 1) {
            if (!lease_valid) {
                memcpy(offer_mac, dhcp_pending_mac, 6);
                offer_valid = 1;
                offer_deadline_us = time_us_64() + DHCP_OFFER_TIMEOUT_US;
            }
        } else {
            memcpy(lease_mac, dhcp_pending_mac, 6);
            lease_valid = 1;
            offer_valid = 0;
        }
    }
    dhcp_pending = 0;
}

void net_poll(void) {
    if (!wifi_ready) return;
    cyw43_arch_poll();
    /* Never transmit DHCP replies from a lwIP receive callback. */
    dhcp_send_pending();
    dhcp_expire_offer();
    net_terminal_flush();
}
#else
void net_init(void) {}
void net_poll(void) {}
int net_terminal_connected(void) { return 0; }
unsigned net_terminal_epoch(void) { return 0; }
int net_terminal_read(void *dst, unsigned size) { (void)dst; (void)size; return -1; }
int net_terminal_write(const void *src, unsigned size) { (void)src; (void)size; return -1; }
#endif
