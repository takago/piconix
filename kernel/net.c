/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Pico WのWi-Fi接続，AP側DHCP，STA側再接続とmDNS，ネットワーク診断を扱う．
 * CYW43/lwIPの処理は原則アイドルタスク側のポーリングへ集約する． */

#include "net.h"
#include "net_services.h"
#include "piconix.h"
#include <stdio.h>
#ifndef PICONIX_WIFI
#define PICONIX_WIFI 0
#endif
#if PICONIX_WIFI
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/etharp.h"
#include "lwip/apps/mdns.h"
#include "lwip/ip4_addr.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/stats.h"
#include "lwip/memp.h"
#include <string.h>
#include <stdint.h>

/* 固定資源を使い，AP側のDHCPリースと独立したネットワークTTYを管理する． */
static int wifi_ready;
static char wifi_ssid[33];
/* ドライバ操作はnet_pollに集約し，/procのreadからは呼び出さない． */
static uint8_t wifi_mac[6];
static int wifi_mac_valid;
static uint64_t wifi_mac_next_us;
#define WIFI_MAC_RETRY_US 5000000ull
/* CYW43から取得したMACアドレスをキャッシュする．失敗時は次回のアイドルポーリングで再試行し，/proc読出し中にドライバを呼ばない． */
static void wifi_cache_mac(uint64_t now) {
    if (wifi_mac_valid || (int64_t)(now - wifi_mac_next_us) < 0) return;
    int itf = PICONIX_WIFI_STA ? CYW43_ITF_STA : CYW43_ITF_AP;
    if (cyw43_wifi_get_mac(&cyw43_state, itf, wifi_mac) == 0)
        wifi_mac_valid = 1;
    else
        wifi_mac_next_us = now + WIFI_MAC_RETRY_US;
}

/* 以前取得したMACアドレスを表示文字列にする．未取得ならN/Aを返す． */
static void wifi_mac_text(char *out, unsigned size) {
    if (!wifi_mac_valid) {
        snprintf(out, size, "N/A");
        return;
    }
    snprintf(out, size, "%02X:%02X:%02X:%02X:%02X:%02X",
        wifi_mac[0], wifi_mac[1], wifi_mac[2],
        wifi_mac[3], wifi_mac[4], wifi_mac[5]);
}

#if !PICONIX_WIFI_STA
#define AP_STAS_MAX 8
static uint8_t ap_stas[AP_STAS_MAX][6];
static int ap_sta_count;
static uint64_t ap_stas_next_us;
static int ap_dhcp_active;
#endif
/* CYW43/lwIPはタイマー割込みではなくアイドルタスクからポーリングする．STA/AP共通で最短間隔は250 µsとする．これは目標下限であり，正確な周期を保証しない． */
#define NET_POLL_INTERVAL_US 250ull
/* アイドル側とタスク側からlwIP raw APIへ同時進入しないようにする． */
/* lwIP初期化で作られるlo0を，既存の直列化されたアイドルポーリングから処理する． */
static struct netif *loop_iface;
static volatile unsigned net_poll_active;
/* lwIPのポーリング処理中か返す．並行するraw API操作を避ける判定に使う． */
int net_poll_in_progress(void) { return net_poll_active != 0; }
static uint64_t net_poll_last_us;
static uint32_t net_poll_count;
static uint32_t net_poll_max_gap_us;
#if PICONIX_WIFI_STA
/* 起動時にUSB CDCが未接続でも診断情報を残す． */
static int sta_error;
static int sta_pm_error = -1;
static volatile int sta_manual_restart;
static int sta_link;
static int sta_tcpip_link;
static int sta_dhcp_bound;
/* STAのRSSIは最大5秒に一度取得し，診断時にはキャッシュを使う． */
#define STA_RSSI_INTERVAL_US 5000000ull
static uint64_t sta_rssi_next_us;
static int32_t sta_rssi_dbm;
static int sta_rssi_valid;
/* STAのRSSIキャッシュを無効化し，次回の取得を許可する． */
static void sta_rssi_clear(void) {
    sta_rssi_valid = 0;
    sta_rssi_next_us = 0;
}

/* STA接続時のRSSIを低頻度に取得し，診断表示にはキャッシュを使う． */
static void sta_rssi_poll(uint64_t now) {
    if ((int64_t)(now - sta_rssi_next_us) < 0) return;
    int32_t value = 0;
    if (cyw43_wifi_get_rssi(&cyw43_state, &value) == 0 &&
            value < 0 && value >= -127) {
        sta_rssi_dbm = value;
        sta_rssi_valid = 1;
    } else {
        sta_rssi_valid = 0;
    }
    sta_rssi_next_us = now + STA_RSSI_INTERVAL_US;
}

/* RSSIを使った電波強度の推定値であり，通信品質の実測値ではない．-100 dBmを0%，-50 dBm以上を100%に対応させる． */
static void sta_rssi_text(char *out, unsigned size) {
    if (!sta_rssi_valid) {
        snprintf(out, size, "N/A");
        return;
    }
    int quality = ((int)sta_rssi_dbm + 100) * 2;
    if (quality < 0) quality = 0;
    if (quality > 100) quality = 100;
    snprintf(out, size, "%ld dBm (%d%%)", (long)sta_rssi_dbm, quality);
}

/* 機器ごとに変わらない.local名を使用し，DHCP完了後だけ提供する． */
static char sta_mdns_name[18]; /* piconix-と8桁の16進数と終端NUL． */
static int sta_mdns_registered;
static int sta_mdns_error;
static uint32_t sta_mdns_ip;
static int sta_mdns_link_was_down;
/* STA側のmDNS登録を取り消し，再接続時の重複登録を避ける． */
static void sta_mdns_remove(void) {
    if (sta_mdns_registered) {
        mdns_resp_remove_netif(&cyw43_state.netif[CYW43_ITF_STA]);
        sta_mdns_registered = 0;
    }
    sta_mdns_ip = 0;
}

/* IPアドレス確立後のmDNS提供状態を管理する．接続前に名前を広告しない． */
static void sta_mdns_ready(struct netif *iface) {
    uint32_t ip = ip4_addr_get_u32(netif_ip4_addr(iface));
    if (!sta_mdns_registered) {
        sta_mdns_error = (int)mdns_resp_add_netif(iface, sta_mdns_name);
        sta_mdns_registered = (sta_mdns_error == ERR_OK);
        sta_mdns_ip = sta_mdns_registered ? ip : 0;
        sta_mdns_link_was_down = 0;
    } else if (ip != sta_mdns_ip) {
        sta_mdns_ip = ip;
        mdns_resp_announce(iface);
    } else if (sta_mdns_link_was_down) {
        mdns_resp_restart(iface);
        sta_mdns_link_was_down = 0;
    }
}

/* STAでのみ試すGARP．lwIPポーリングの文脈から送る． */
#define STA_GARP_INTERVAL_US 60000000ull
static uint64_t sta_garp_next_us;
static uint32_t sta_garp_ok;
static uint32_t sta_garp_error;
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

/* CYW43のリンク状態コードを診断表示用の文字列に変換する．未知の値はUNKNOWN． */
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

/* STA接続の状態と関連する情報を初期化して再接続可能にする． */
static void sta_reset_association(char reason) {
    sta_last_reset = reason;
    sta_rssi_clear();
    cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
    sta_connecting = 0;
    sta_connected = 0;
    sta_link_lost_us = 0;
    sta_garp_next_us = 0;
    sta_mdns_remove();
}

/* STA接続を非同期に開始する．起動処理やシステムコール内で接続完了を待たない． */
static void sta_begin_connect(void) {
    sta_rssi_clear();
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

/* STAのリンク・DHCP取得と再接続の状態遷移を進める．呼出し元はアイドルタスクのnet_pollである． */
static void sta_poll(void) {
    uint64_t now = time_us_64();
    if (sta_manual_restart) {
        sta_manual_restart = 0;
        sta_reset_association('M');
        sta_retry_us = now;
        sta_begin_connect();
        return;
    }
    struct netif *iface = &cyw43_state.netif[CYW43_ITF_STA];
    if (!sta_attempts) {
        /* STAの省電力設定は最初のCYW43ポーリング後，接続開始前にアイドル側で行う． */
        sta_pm_error = cyw43_wifi_pm(&cyw43_state, CYW43_NONE_PM);
        if (sta_pm_error != 0)
            puts("PicoNIX: STA power management setup failed");
        sta_begin_connect();
        return;
    }
    sta_link = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    sta_tcpip_link = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    sta_dhcp_bound = dhcp_supplied_address(iface) ? 1 : 0;
    /* DHCP完了後もWi-Fiの状態がJOINを示すことがあるため，STAの準備完了はIP状態と有効なDHCPリースで判定する． */
    if (sta_tcpip_link == CYW43_LINK_UP && sta_dhcp_bound) {
        sta_connecting = 0;
        sta_connected = 1;
        sta_link_lost_us = 0;
        sta_error = 0;
        sta_rssi_poll(now);
        if (!ip4_addr_isany_val(*netif_ip4_addr(iface))) {
            sta_start_services();
            sta_mdns_ready(iface);
            /* IP取得から一定時間後に最初のアドレス通知を行う．システムコール・タイマー割込みから無線ドライバを呼ばない． */
            if (!sta_garp_next_us)
                sta_garp_next_us = now + STA_GARP_INTERVAL_US;
            if ((int64_t)(now - sta_garp_next_us) >= 0) {
                err_t err = etharp_gratuitous(iface);
                if (err == ERR_OK) ++sta_garp_ok;
                else ++sta_garp_error;
                sta_garp_next_us = now + STA_GARP_INTERVAL_US;
            }
        }
        return;
    }

    sta_rssi_valid = 0; /* 接続喪失時には古いRSSIを表示しない． */
    sta_rssi_next_us = 0;
    sta_garp_next_us = 0; /* DHCPとリンクが復旧してから次の通知を準備する． */
    if (sta_mdns_registered) sta_mdns_link_was_down = 1;
    if (!sta_dhcp_bound) sta_mdns_remove();
    if (sta_connected) {
        /* 再接続中の一時的なリンク状態変化を無視する． */
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

/* STAのリンク確立後にネットワークサービスとmDNS等を起動する． */
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
static int ap_mdns_registered;
static int ap_mdns_error;
static struct udp_pcb *dhcp_pcb;
#define DHCP_OPTIONS_OFFSET 240u
/* DHCP応答のオプションは固定なので，大きな300バイトのスタック配列を避ける． */
static const unsigned char dhcp_reply_options[] = {
    53,1,0, 54,4,192,168,4,1,
    51,4,0,0,14,16, 1,4,255,255,255,0,
    28,4,192,168,4,255, 255
};
/* 一般的なクライアントとの互換性のため，BOOTP/DHCP応答は300バイト以上にする． */
#define DHCP_REPLY_LEN 300u
/* DHCP受信時は一件だけ返信を保留し，cyw43_arch_pollから戻った後に送信する．TCP端末の送信と同じ方針である． */
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

/* 未完了のDHCP交渉が次の端末の接続を永久に妨げないようにする． */
static void dhcp_expire_offer(void) {
    if (offer_valid && !lease_valid &&
        (int64_t)(time_us_64() - offer_deadline_us) >= 0)
        offer_valid = 0;
}

/* AP側のDHCP/BOOTP要求を検査する．再起動までのリースはMACアドレス1件に限定し，応答は受信コールバック内で送らず保留する． */
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
    if (dhcp_pending) goto done; /* 保留中の最初の返信を上書きしない． */
    if (port != 68 || p->tot_len < DHCP_OPTIONS_OFFSET ||
        pbuf_copy_partial(p, request, sizeof(request), 0) != sizeof(request) ||
        request[0] != 1 || request[1] != 1 || request[2] != 6 ||
        memcmp(request + 236, dhcp_cookie, 4) != 0) goto done;
    if (lease_valid && memcmp(lease_mac, request + 28, 6) != 0) goto done;
    /* 次のDISCOVERにより先着端末の保留中オファーを奪わせない． */
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
            /* メッセージ種別が複数含まれる曖昧なパケットを拒否する． */
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
    /* DISCOVER/OFFERでは割当てを確定しない．REQUEST/ACKを経たときだけリースとし，確定前はオファー相手からのREQUESTだけ認める． */
    if (message_type == 3 && !lease_valid &&
        (!offer_valid || memcmp(offer_mac, request + 28, 6) != 0)) goto done;
    if (has_requested_ip &&
        memcmp(requested_ip, (const unsigned char[]){192,168,4,2}, 4) != 0)
        goto done;
    if (has_server_id &&
        memcmp(server_id, (const unsigned char[]){192,168,4,1}, 4) != 0)
        goto done;
    /* オプション50を含まない更新要求でも，使用中リースのIPを指している必要がある． */
    if (message_type == 3 && !has_requested_ip &&
        memcmp(request + 12, (const unsigned char[]){192,168,4,2}, 4) != 0)
        goto done;
    memset(reply, 0, sizeof(reply));
    reply[0] = 2; reply[1] = 1; reply[2] = 6;
    memcpy(reply + 4, request + 4, 4); /* トランザクション識別子（xid） */
    memcpy(reply + 10, request + 10, 2); /* BOOTPフラグ */
    memcpy(reply + 28, request + 28, 16); /* クライアントのハードウェアアドレス（chaddr） */
    reply[16] = 192; reply[17] = 168; reply[18] = 4; reply[19] = 2;
    reply[20] = 192; reply[21] = 168; reply[22] = 4; reply[23] = 1;
    memcpy(reply + 236, dhcp_cookie, 4);
    memcpy(reply + DHCP_OPTIONS_OFFSET, dhcp_reply_options,
           sizeof(dhcp_reply_options));
    reply[DHCP_OPTIONS_OFFSET + 2] = message_type == 1 ? 2 : 5;
    /* lwIP受信コールバックからは送信しない． */
    memcpy(dhcp_pending_reply, reply, sizeof(reply));
    memcpy(dhcp_pending_mac, request + 28, 6);
    dhcp_pending_type = message_type;
    dhcp_pending = 1;
    goto done;
done:
    pbuf_free(p);
}

/* APモードのDHCPサービスを開始し，固定長の情報でアドレスを配布する． */
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
    /* DHCPのOFFER/ACKはIPv4ブロードキャストへ送信する． */
    ip_set_option(dhcp_pcb, SOF_BROADCAST);
    udp_recv(dhcp_pcb, dhcp_recv, NULL);
    return 1;
}


#endif /* AP向けDHCPサーバ */
extern uint32_t piconix_flash_id(void);
/* Wi-Fi/LwIPを初期化する．STAの接続作業は起動時に待たず，後続のnet_pollへ委ねる． */
void net_init(void) {
    if (cyw43_arch_init() != 0) {
        puts("PicoNIX: CYW43 initialization failed");
        return;
    }
    net_capture_init();
    loop_iface = netif_find("lo0");
    net_capture_attach(loop_iface);
    net_capture_attach(&cyw43_state.netif[PICONIX_WIFI_STA ? CYW43_ITF_STA : CYW43_ITF_AP]);
    if (!loop_iface)
        puts("PicoNIX: loopback interface unavailable");
#if PICONIX_WIFI_STA
    strncpy(wifi_ssid, PICONIX_STA_SSID, sizeof(wifi_ssid)-1);
    wifi_ssid[sizeof(wifi_ssid)-1] = 0;
    cyw43_arch_enable_sta_mode();
    static const char hex[] = "0123456789abcdef";
    uint32_t fid = piconix_flash_id();
    memcpy(sta_mdns_name,"piconix-",8);
    for (unsigned i=0;i<8;i++) sta_mdns_name[8+i] = hex[(fid >> (28-4*i)) & 15u];
    sta_mdns_name[16] = 0;
    mdns_resp_init();
    /* STAの省電力設定はアイドルの初回net_pollまで遅らせ，起動時のスタックでドライバ操作しない． */
    /* 非同期接続中もWi-Fiとシェルの処理を継続する． */
    wifi_ready = 1;
    /* 起動時ではなくアイドルループで接続を開始する． */
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
    struct netif *ap_iface = &cyw43_state.netif[CYW43_ITF_AP];
    netif_set_addr(ap_iface, &ip, &mask, &gw);
    mdns_resp_init();
    ap_mdns_error = (int)mdns_resp_add_netif(ap_iface, "piconix");
    ap_mdns_registered = (ap_mdns_error == ERR_OK);
    int telnet_ready = net_service_telnet_init();
    int dhcp_ready = ap_dhcp_server_start();
    ap_dhcp_active = dhcp_ready;
    int device_ready = net_service_remote_devctl_init();
    wifi_ready = 1;
    puts(device_ready ? "PicoNIX: UDP device service port 2000" : "PicoNIX: UDP device service failed");
    puts(dhcp_ready ? "PicoNIX: DHCP server active (192.168.4.2)"
                    : "PicoNIX: DHCP server initialization failed");
    puts(telnet_ready ? "PicoNIX: Telnet terminal port 23" : "PicoNIX: Telnet listener failed");
#endif /* PICONIX_WIFI_STAの設定に応じたAP側初期化 */
}
#if !PICONIX_WIFI_STA
/* cyw43_arch_pollとlwIPコールバックの外側から呼ぶ． */
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

#endif /* AP側DHCPサーバ */
/* アイドルタスクから無線・TCP/IP・保留送信・ループバックを順次処理する．同じlwIP文脈への再入を避ける． */
void net_poll(void) {
    if (!wifi_ready) return;
    /* ネットワークを処理できなかった最長間隔を測る．ユーザタスクがアイドル処理を先延ばしにした時間も含む． */
    uint64_t now = time_us_64();
    if (net_poll_last_us &&
            now - net_poll_last_us < NET_POLL_INTERVAL_US) return;
    if (net_poll_last_us) {
        uint64_t gap = now - net_poll_last_us;
        if (gap > net_poll_max_gap_us)
            net_poll_max_gap_us = gap > UINT32_MAX ? UINT32_MAX : (uint32_t)gap;
    }
    net_poll_last_us = now;
    ++net_poll_count;
    net_poll_active = 1;
    cyw43_arch_poll();
    net_capture_poll();
    wifi_cache_mac(now);
    /* 127.0.0.1宛てパケットを再帰的なlwIP入力呼出しをせず処理する．返信がコールバック内で発生した場合は次回のアイドルポーリングで渡す． */
    if (loop_iface) netif_poll(loop_iface);
    /* 端末自身のWi-Fiインターフェース宛てパケットも処理する． */
    netif_poll(&cyw43_state.netif[PICONIX_WIFI_STA ? CYW43_ITF_STA : CYW43_ITF_AP]);
#if PICONIX_WIFI_STA
    sta_poll();
#endif
    /* DHCP受信コールバックから直接送信しない． */
#if !PICONIX_WIFI_STA
    dhcp_send_pending();
#endif
    net_service_remote_devctl_poll();
#if !PICONIX_WIFI_STA
    dhcp_expire_offer();
    if (now >= ap_stas_next_us) {
        int count = AP_STAS_MAX;
        cyw43_wifi_ap_get_stas(&cyw43_state, &count, (uint8_t *)ap_stas);
        ap_sta_count = count < 0 ? 0 : count > AP_STAS_MAX ? AP_STAS_MAX : count;
        ap_stas_next_us = now + 1000000ull;
    }
#endif
    net_service_telnet_poll();
    net_poll_active = 0;
}
#else
/* Wi-Fi/LwIPを初期化する．STAの接続作業は起動時に待たず，後続のnet_pollへ委ねる． */
void net_init(void) {}

/* アイドルタスクから無線・TCP/IP・保留送信・ループバックを順次処理する．同じlwIP文脈への再入を避ける． */
void net_poll(void) {}

/* 通常Picoビルドでもスケジューラから呼ばれる． */
int net_poll_in_progress(void) { return 0; }
#endif

/* lwIPヒープやプールの診断情報をテキスト化する．値はネットワーク関連領域の観測値であり総RAM使用量ではない． */
int net_memory_info(char *out, unsigned size) {
    if (!size) return 0;
#if PICONIX_WIFI
    const struct stats_mem *pool = lwip_stats.memp[MEMP_PBUF_POOL];
    int len = snprintf(out, size,
        "LWIP_HEAP: %u/%u/%u\nLWIP_HEAP_ERRORS: %u\n"
        "PBUF_POOL: %u/%u/%u\nPBUF_POOL_ERRORS: %u\n"
        "IDLE_STACK: %u/%u\n",
        (unsigned)lwip_stats.mem.used, (unsigned)lwip_stats.mem.max,
        (unsigned)MEM_SIZE, (unsigned)lwip_stats.mem.err,
        pool ? (unsigned)pool->used : 0u,
        pool ? (unsigned)pool->max : 0u,
        (unsigned)PBUF_POOL_SIZE,
        pool ? (unsigned)pool->err : 0u,
        os_idle_stack_used_bytes(), os_idle_stack_capacity_bytes());
#else
    int len = snprintf(out, size, "MODE: N/A\nIDLE_STACK: %u/%u\n",
        os_idle_stack_used_bytes(), os_idle_stack_capacity_bytes());
#endif
    if (len < 0) return 0;
    return (unsigned)len >= size ? (int)(size - 1u) : len;
}

/* /proc/net/wifiの読出し専用スナップショット． */
#if PICONIX_WIFI
/* /proc/net/wifi用の状態をテキストへ整形する．システムコール中にCYW43へ同期問い合わせしない． */
int net_wifi_info(char *out, unsigned size) {
    if (!size) return 0;
    if (!wifi_ready) return snprintf(out,size,"MODE: %s\nSTATUS: DOWN\nERROR: CYW43 initialization failed\n", PICONIX_WIFI_STA ? "STA" : "AP");
    struct netif *iface = &cyw43_state.netif[PICONIX_WIFI_STA ? CYW43_ITF_STA : CYW43_ITF_AP];
    /* /procのreadはSVC内で行うため，同期的なCYW43問い合わせはしない．ドライバの進行にはポーリングが必要な場合がある． */
    char mac[18];
    wifi_mac_text(mac, sizeof(mac));
#if PICONIX_WIFI_STA
    char rssi[32];
    sta_rssi_text(rssi, sizeof(rssi));
#else
    const char *rssi = "N/A (AP mode)"; /* APモードには上流APからの電波強度を測る対象がない． */
#endif
    char ip[20], mask[20], gw[20];
    ip4addr_ntoa_r(netif_ip4_addr(iface), ip, sizeof(ip));
    ip4addr_ntoa_r(netif_ip4_netmask(iface), mask, sizeof(mask));
    ip4addr_ntoa_r(netif_ip4_gw(iface), gw, sizeof(gw));
#if PICONIX_WIFI_STA
    int len = snprintf(out, size,
        "MODE: STA\nIFACE: %c%c%u\nSTATUS: %s\nSSID: %s\nDHCP: Client\n"
        "ATTEMPTS: %u\nCONNECT_ERROR: %d\nLINK_CODE: %d\n"
        "WIFI_LINK: %d\nTCPIP_LINK: %d\nDHCP_BOUND: %d\n"
        "LAST_RESET: %c\nIP: %s\nNETMASK: %s\nGATEWAY: %s\nMAC: %s\nRSSI: %s\n"
        "PM_ERROR: %d\nPOLL_COUNT: %lu\nPOLL_MAX_GAP_MS: %lu\n"
        "GARP_INTERVAL_S: 60\nGARP_OK: %lu\nGARP_ERROR: %lu\n"
        "MDNS_NAME: %s.local\nMDNS_ACTIVE: %d\nMDNS_ERROR: %d\n"
        "IDLE_STACK: %u/%u\n",
        iface->name[0],iface->name[1],iface->num,
        sta_link_name(sta_tcpip_link), wifi_ssid, sta_attempts, sta_error,
        sta_link, sta_link, sta_tcpip_link, sta_dhcp_bound,
        sta_last_reset, ip, mask, gw, mac, rssi,
        sta_pm_error, (unsigned long)net_poll_count,
        (unsigned long)(net_poll_max_gap_us / 1000u),
        (unsigned long)sta_garp_ok, (unsigned long)sta_garp_error,
        sta_mdns_name, sta_mdns_registered, sta_mdns_error,
        os_idle_stack_used_bytes(), os_idle_stack_capacity_bytes());
    if (len < 0) return 0;
    if ((unsigned)len >= size) return (int)(size - 1u);
    return len;
#else
    return snprintf(out,size,
        "MODE: AP\nIFACE: %c%c%u\nSSID: %s\nDHCP: %s\nCLIENTS: %d\nMAC: %s\n"
        "IP: %s\nNETMASK: %s\nGATEWAY: %s\nRSSI: %s\n"
        "MDNS_NAME: piconix.local\nMDNS_ACTIVE: %d\nMDNS_ERROR: %d\n",
        iface->name[0],iface->name[1],iface->num,
        wifi_ssid, ap_dhcp_active ? "Server" : "Inactive", ap_sta_count, mac,
        ip, mask, gw, rssi,
        ap_mdns_registered, ap_mdns_error);
#endif
}
#else
/* /proc/net/wifi用の状態をテキストへ整形する．システムコール中にCYW43へ同期問い合わせしない． */
int net_wifi_info(char *out, unsigned size) {
    static const char msg[] = "MODE: N/A\n";
    unsigned n = sizeof(msg)-1;
    if (n >= size) n = size ? size-1 : 0;
    for (unsigned i=0; i<n; ++i) out[i]=msg[i];
    if (size) out[n] = 0;
    return (int)n;
}
#endif

/* 再起動要求のフラグを立てる．実際のドライバ再接続はアイドル側へ委ねる． */
int net_wifi_request_restart(void) {
#if PICONIX_WIFI && PICONIX_WIFI_STA
    if (!wifi_ready) return -1;
    sta_manual_restart = 1;
    return 0;
#else
    return -1;
#endif
}

/* AP/ARPの診断データを取得する．接続状態はポーリング側で更新したキャッシュを利用する． */
int net_diag_info(char *out, unsigned size, int type) {
    if (!size) return 0;
#if PICONIX_WIFI
    unsigned used = 0;
    int n;
    if (!wifi_ready) return snprintf(out,size,"Network unavailable\n");
    if (type == 0) {
#if PICONIX_WIFI_STA
        return snprintf(out,size,"Not in AP mode\n");
#else
        n=snprintf(out,size,"Connected clients: %d\n",ap_sta_count);
        if(n<0 || (unsigned)n>=size) return (int)(size-1);
        used=(unsigned)n;
        for(int i=0;i<ap_sta_count;i++) {
            const uint8_t *m=ap_stas[i];
            n=snprintf(out+used,size-used,"%02X:%02X:%02X:%02X:%02X:%02X\n",
                m[0],m[1],m[2],m[3],m[4],m[5]);
            if(n<0 || (unsigned)n>=size-used) break;
            used+=(unsigned)n;
        }
        return (int)used;
#endif
    }
    n=snprintf(out,size,"ARP table:\n");
    if(n<0 || (unsigned)n>=size) return (int)(size-1);
    used=(unsigned)n;
    for(int i=0;i<ARP_TABLE_SIZE;i++) {
        ip4_addr_t *ip;
        struct netif *iface;
        struct eth_addr *mac;
        if(!etharp_get_entry(i,&ip,&iface,&mac)) continue;
        char addr[20];
        ip4addr_ntoa_r(ip,addr,sizeof addr);
        n=snprintf(out+used,size-used,"%s %02X:%02X:%02X:%02X:%02X:%02X\n",
            addr,mac->addr[0],mac->addr[1],mac->addr[2],
            mac->addr[3],mac->addr[4],mac->addr[5]);
        if(n<0 || (unsigned)n>=size-used) break;
        used+=(unsigned)n;
    }
    return (int)used;
#else
    (void)type;
    return snprintf(out,size,"Network unavailable (Pico W only)\n");
#endif
}
