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
#if PICONIX_WIFI_STA
/* Preserve diagnostics even if USB CDC was not attached during boot. */
static int sta_error;
static int sta_link;
static int sta_tcpip_link;
static int sta_dhcp_bound;
static unsigned sta_attempts;
static uint64_t sta_started_us;
static uint64_t sta_retry_us;
static int sta_services_ready;
static int sta_connecting;
static int sta_connected;
static uint64_t sta_link_lost_us;
static char sta_last_reset = '-';
#define STA_CONNECT_TIMEOUT_US (60000000ull)
#define STA_RETRY_DELAY_US (5000000ull)
#define STA_LINK_LOSS_GRACE_US (5000000ull)

static const char *sta_link_name(int status) {
    switch (status) {
        case CYW43_LINK_UP: return "UP";
        case CYW43_LINK_NOIP: return "NO_IP";
        case CYW43_LINK_JOIN: return "JOINING";
        case CYW43_LINK_DOWN: return "DOWN";
        case CYW43_LINK_FAIL: return "FAILED";
        case CYW43_LINK_NONET: return "NO_NETWORK";
        case CYW43_LINK_BADAUTH: return "BAD_AUTH";
        default: return "UNKNOWN";
    }
}

/* Only the idle/network poll context may touch the CYW43 association. */
static void sta_reset_association(char reason) {
    sta_last_reset = reason;
    cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
    sta_connecting = 0;
    sta_connected = 0;
    sta_link_lost_us = 0;
}

static void sta_begin_connect(void) {
    ++sta_attempts;
    sta_started_us = time_us_64();
    sta_error = cyw43_arch_wifi_connect_async(PICONIX_STA_SSID,
                    PICONIX_STA_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
    sta_link = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    sta_connecting = (sta_error == 0);
    if (!sta_connecting)
        sta_retry_us = sta_started_us + STA_RETRY_DELAY_US;
}

static void sta_start_services(void);

/* Run only from idle/net_poll, after cyw43_arch_poll().  Never wait for
 * association or DHCP in the boot path or while servicing a syscall. */
static void sta_poll(void) {
    uint64_t now = time_us_64();
    struct netif *iface = &cyw43_state.netif[CYW43_ITF_STA];
    if (!sta_attempts) {
        sta_begin_connect();
        return;
    }
    sta_link = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    sta_tcpip_link = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    sta_dhcp_bound = dhcp_supplied_address(iface) ? 1 : 0;
    /* The Wi-Fi status may remain JOIN after DHCP completes.  Use the
     * TCP/IP status and confirmed DHCP lease for STA readiness. */
    if (sta_tcpip_link == CYW43_LINK_UP && sta_dhcp_bound) {
        sta_connecting = 0;
        sta_connected = 1;
        sta_link_lost_us = 0;
        sta_error = 0;
        if (!ip4_addr_isany_val(*netif_ip4_addr(iface)))
            sta_start_services();
        return;
    }

    if (sta_connected) {
        /* Ignore short-lived link-status changes during reassociation. */
        if (!sta_link_lost_us) sta_link_lost_us = now;
        if ((int64_t)(now - sta_link_lost_us) <
                (int64_t)STA_LINK_LOSS_GRACE_US) return;
        sta_reset_association('D');
        sta_retry_us = now + STA_RETRY_DELAY_US;
        return;
    }

    if (sta_connecting) {
        int failed = sta_link == CYW43_LINK_FAIL ||
                     sta_link == CYW43_LINK_NONET ||
                     sta_link == CYW43_LINK_BADAUTH;
        if (!failed && (int64_t)(now - sta_started_us) <
                       (int64_t)STA_CONNECT_TIMEOUT_US) return;
        sta_reset_association(failed ? 'F' : 'T');
        sta_retry_us = now + STA_RETRY_DELAY_US;
        return;
    }
    if ((int64_t)(now - sta_retry_us) >= 0)
        sta_begin_connect();
}

static void sta_start_services(void) {
    if (sta_services_ready) return;
    sta_services_ready = 1;
    int telnet_ready = net_service_telnet_init();
    int device_ready = net_service_remote_devctl_init();
    puts(device_ready ? "PicoNIX: UDP device service port 2000" : "PicoNIX: UDP device service failed");
    puts(telnet_ready ? "PicoNIX: Telnet terminal port 23" : "PicoNIX: Telnet listener failed");
}
#endif

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

static int ap_dhcp_server_start(void) {
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
    /* Nonblocking: continue servicing Wi-Fi and the shell while joining. */
    wifi_ready = 1;
    /* Start association from the idle loop, not during boot. */
    return;
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
    int dhcp_ready = ap_dhcp_server_start();
    int device_ready = net_service_remote_devctl_init();
    wifi_ready = 1;
    puts(device_ready ? "PicoNIX: UDP device service port 2000" : "PicoNIX: UDP device service failed");
    puts(dhcp_ready ? "PicoNIX: DHCP server active (192.168.4.2)"
                    : "PicoNIX: DHCP server initialization failed");
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
#if PICONIX_WIFI_STA
    sta_poll();
#endif
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
    if (!wifi_ready) return snprintf(out,size,"MODE: %s\nSTATUS: DOWN\nERROR: CYW43 initialization failed\n", PICONIX_WIFI_STA ? "STA" : "AP");
    struct netif *iface = &cyw43_state.netif[PICONIX_WIFI_STA ? CYW43_ITF_STA : CYW43_ITF_AP];
    /* /proc reads run inside a syscall.  Never issue synchronous CYW43
     * commands here: the driver may require polling to make progress. */
    const char *rssi = "N/A";
    char ip[20], mask[20], gw[20];
    ip4addr_ntoa_r(netif_ip4_addr(iface), ip, sizeof(ip));
    ip4addr_ntoa_r(netif_ip4_netmask(iface), mask, sizeof(mask));
    ip4addr_ntoa_r(netif_ip4_gw(iface), gw, sizeof(gw));
#if PICONIX_WIFI_STA
    int len = snprintf(out, size,
        "MODE: STA\nSTATUS: %s\nSSID: %s\n"
        "ATTEMPTS: %u\nCONNECT_ERROR: %d\nLINK_CODE: %d\n"
        "WIFI_LINK: %d\nTCPIP_LINK: %d\nDHCP_BOUND: %d\n"
        "LAST_RESET: %c\nIP: %s\nNETMASK: %s\nGATEWAY: %s\nMAC: N/A\nRSSI: %s\n",
        sta_link_name(sta_tcpip_link), wifi_ssid, sta_attempts, sta_error,
        sta_link, sta_link, sta_tcpip_link, sta_dhcp_bound,
        sta_last_reset, ip, mask, gw, rssi);
    if (len < 0) return 0;
    if ((unsigned)len >= size) return (int)(size - 1u);
    return len;
#else
    return snprintf(out,size,
        "MODE: %s\nSSID: %s\nMAC: N/A\n"
        "IP: %s\nNETMASK: %s\nGATEWAY: %s\nRSSI: %s\n",
        PICONIX_WIFI_STA ? "STA" : "AP", wifi_ssid,
        ip, mask, gw, rssi);
#endif
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
