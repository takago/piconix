/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "net.h"
#include "net_services.h"
#ifndef PICONIX_WIFI
#define PICONIX_WIFI 0
#endif
#if PICONIX_WIFI
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/ip4_addr.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

/* One DHCP lease and one TCP connection.
 * Stage 18: bounded queues for the independent network TTY. */
static int wifi_ready;
static char wifi_ssid[33];

#if !PICONIX_WIFI_STA
static struct udp_pcb *dhcp_pcb;
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


#endif /* AP DHCP server */
extern uint32_t piconix_flash_id(void);
void net_init(void) {
    if (cyw43_arch_init() != 0) {
        puts("PicoNIX: CYW43 initialization failed");
        return;
    }
#if PICONIX_WIFI_STA
    strncpy(wifi_ssid, PICONIX_STA_SSID, sizeof(wifi_ssid)-1);
    wifi_ssid[sizeof(wifi_ssid)-1] = 0;
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(PICONIX_STA_SSID,
            PICONIX_STA_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 15000) != 0) {
        puts("PicoNIX: STA connection failed");
        return;
    }
#else
    static const char hex[] = "0123456789ABCDEF";
    uint32_t id = piconix_flash_id();
    memcpy(wifi_ssid, "PicoNIX-", 8);
    for (unsigned i=0; i<8; ++i)
        wifi_ssid[8+i] = hex[(id >> (28-4*i)) & 15u];
    wifi_ssid[16] = 0;
    cyw43_arch_enable_ap_mode(wifi_ssid, "piconix1234", CYW43_AUTH_WPA2_AES_PSK);
    ip4_addr_t ip, mask, gw;
    IP4_ADDR(&ip, 192, 168, 4, 1);
    IP4_ADDR(&mask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 4, 1);
    netif_set_addr(&cyw43_state.netif[CYW43_ITF_AP], &ip, &mask, &gw);
#endif
    int telnet_ready = net_service_telnet_init();
#if PICONIX_WIFI_STA
    int dhcp_ready = 1; /* STA uses lwIP's DHCP client. */
#else
    int dhcp_ready = dhcp_start();
#endif
    int device_ready = net_service_remote_devctl_init();
    /* Start servicing the stack only after AP and listener setup. */
    wifi_ready = 1;
    puts(device_ready ? "PicoNIX: UDP device service port 2000" : "PicoNIX: UDP device service failed");
#if PICONIX_WIFI_STA
    puts("PicoNIX: STA connected (DHCP client)");
#else
    puts(dhcp_ready ? "PicoNIX: DHCP server active (192.168.4.2)"
                    : "PicoNIX: DHCP server initialization failed");
#endif
    puts(telnet_ready ? "PicoNIX: Telnet terminal port 23" : "PicoNIX: Telnet listener failed");
}
#if !PICONIX_WIFI_STA
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

#endif /* AP DHCP server */
void net_poll(void) {
    if (!wifi_ready) return;
    cyw43_arch_poll();
    /* Never transmit DHCP replies from a lwIP receive callback. */
#if !PICONIX_WIFI_STA
    dhcp_send_pending();
#endif
    net_service_remote_devctl_poll();
#if !PICONIX_WIFI_STA
    dhcp_expire_offer();
#endif
    net_service_telnet_poll();
}
#else
void net_init(void) {}
void net_poll(void) {}
#endif

/* Read-only /proc/net/wifi snapshot. */
#if PICONIX_WIFI
int net_wifi_info(char *out, unsigned size) {
    if (!size) return 0;
    if (!wifi_ready) return snprintf(out,size,"MODE: %s\nSTATUS: DOWN\n", PICONIX_WIFI_STA ? "STA" : "AP");
    struct netif *iface = &cyw43_state.netif[PICONIX_WIFI_STA ? CYW43_ITF_STA : CYW43_ITF_AP];
    unsigned char mac[6] = {0};
    cyw43_wifi_get_mac(&cyw43_state, PICONIX_WIFI_STA ? CYW43_ITF_STA : CYW43_ITF_AP, mac);
    char rssi[24] = "N/A";
#if PICONIX_WIFI_STA
    int32_t dbm;
    if (cyw43_wifi_get_rssi(&cyw43_state, &dbm) == 0)
        snprintf(rssi, sizeof(rssi), "%ld dBm", (long)dbm);
#endif
    char ip[20], mask[20], gw[20];
    ip4addr_ntoa_r(netif_ip4_addr(iface), ip, sizeof(ip));
    ip4addr_ntoa_r(netif_ip4_netmask(iface), mask, sizeof(mask));
    ip4addr_ntoa_r(netif_ip4_gw(iface), gw, sizeof(gw));
    return snprintf(out,size,
        "MODE: %s\nSSID: %s\nMAC: %02X:%02X:%02X:%02X:%02X:%02X\n"
        "IP: %s\nNETMASK: %s\nGATEWAY: %s\nRSSI: %s\n",
        PICONIX_WIFI_STA ? "STA" : "AP", wifi_ssid,
        mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],
        ip, mask, gw, rssi);
}
#else
int net_wifi_info(char *out, unsigned size) {
    static const char msg[] = "MODE: N/A\n";
    unsigned n = sizeof(msg)-1;
    if (n >= size) n = size ? size-1 : 0;
    for (unsigned i=0; i<n; ++i) out[i]=msg[i];
    if (size) out[n] = 0;
    return (int)n;
}
#endif
