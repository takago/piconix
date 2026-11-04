/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Telnetの接続と端末入出力キューを管理する．
 * ネットワーク受信コールバックとTTYを分離し，接続し直した端末の状態を混ぜない． */

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
static struct tcp_pcb *telnet_listener; /* TelnetはTCP 23番ポートを使用する． */
static unsigned char client_telnet;
static unsigned char telnet_state, telnet_command, telnet_cr;
static unsigned char telnet_hello_pos;
static const unsigned char telnet_hello[] = {255,251,1, 255,251,3, 255,253,3}; /* Telnet交渉でWILL ECHO・WILL SGA・DO SGAを通知する． */
static struct tcp_pcb *client;
/* 切断と再接続が二回のアイドルポーリングの間に起きても，世代番号で別セッションとして区別する． */
static unsigned terminal_epoch;
/* 端末バイトは固定長キューで扱い，ヒープを使用しない．キューを操作するのはnet_pollとlwIPのコールバックである． */
/* RXキューには交渉済みMSS（536バイト）のTCPセグメント全体が入る必要がある．256バイトしかないとlwIPが同じpbufを再送し続け，受信窓が進まなくなる． */
#define NET_TTY_QUEUE_SIZE 2048u
static unsigned char net_rx[NET_TTY_QUEUE_SIZE], net_tx[NET_TTY_QUEUE_SIZE];
static unsigned net_rx_head, net_rx_tail, net_tx_head, net_tx_tail;
/* Telnetのリングキュー内に存在するバイト数を計算する． */
static unsigned queue_count(unsigned head, unsigned tail) {
    return (head - tail) & (NET_TTY_QUEUE_SIZE - 1u);
}

/* キュー末尾を1つ空けたうえで書き込めるバイト数を返す． */
static unsigned queue_space(unsigned head, unsigned tail) {
    return NET_TTY_QUEUE_SIZE - 1u - queue_count(head, tail);
}

/* Telnetの送受信キューを空にし，新規接続へ備える． */
static void net_queues_reset(void) {
    net_rx_head = net_rx_tail = net_tx_head = net_tx_tail = 0;
}

/* Telnetクライアントの接続有無を返す．非Wi-Fi版は常に未接続． */
int net_terminal_connected(void) { return client != NULL; }

/* 端末接続の世代番号を返す．切断・再接続の検知に用いる． */
unsigned net_terminal_epoch(void) { return terminal_epoch; }

/* 受信キューから指定した最大バイト数を取り出す．切断時は-1，データ待ちなら0，成功時は読出しバイト数を返す． */
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

/* TTY送信バイトを固定長キューに積み，積めた量を返す．切断時は-1．CRLFの途中だけを採用しない． */
int net_terminal_write(const void *src, unsigned size) {
    if (!client) return -1;
    /* TTYからのCRLFをCRだけでキューに入れてはならない． */
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

/* ポーリング文脈からネットワークへ送信キューを流す．TCPの送信バッファ容量を超えないようにする． */
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
    /* 一度の送信追加はMSS以内に収める．tcp_sndbufだけではlwIPのTCPセグメント管理数の上限を考慮できない． */
    if (available > TCP_MSS) available = TCP_MSS;
    if (available && tcp_write(client, net_tx + net_tx_tail,
                               (u16_t)available, TCP_WRITE_FLAG_COPY) == ERR_OK) {
        net_tx_tail = (net_tx_tail + available) & (NET_TTY_QUEUE_SIZE - 1u);
        tcp_output(client);
    }
}

/* lwIPが通知したTCP異常を受け，破棄済みPCBへの参照を外す． */
static void client_error(void *arg, err_t err) {
    (void)arg;
    (void)err;
    client = NULL; /* lwIPがPCBを既に破棄している． */
    ++terminal_epoch;
    net_queues_reset();
}

/* Telnetの制御列と通常入力を分ける．未対応オプションは無視し，CR NULとCR LFを正規化する．lwIP送信はこの関数内で行わない． */
static void net_receive_byte(unsigned char c) {
    if (client_telnet) {
        if (telnet_state == 1) {
            telnet_state = 0;
            if (c == 255) { /* IACがエスケープされたデータバイトである． */ }
            else if (c >= 251 && c <= 254) { telnet_command = c; telnet_state = 2; return; }
            else if (c == 250) { telnet_state = 3; return; }
            else return;
        } else if (telnet_state == 2) {
            /* 接続時に提案済みのECHO/SGAに対する相手の応答は読み飛ばす． */
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
            if (c == 0 || c == '\n') return;
        }
        if (c == '\r') telnet_cr = 1;
    }
    net_rx[net_rx_head] = c;
    net_rx_head = (net_rx_head + 1u) & (NET_TTY_QUEUE_SIZE - 1u);
}

/* TCPコールバックで受信を固定キューへ入れる．全体を受け取れないときはlwIPの受信窓を進めず再試行させる． */
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
        /* 未送信データがあるとtcp_closeは失敗し得る．コールバックだけ解除してPCBを忘れる状態を作らない． */
        if (tcp_close(pcb) != ERR_OK) {
            /* 中止済みPCBをコールバック後にlwIPが参照しないようにする． */
            tcp_abort(pcb);
            return ERR_ABRT;
        }
        return ERR_OK;
    }
    if (err != ERR_OK) {
        /* ここでpbufを解放した場合はERR_OKを返す． */
        pbuf_free(p);
        return ERR_OK;
    }
    /* RXリングに入らないバイトの受信確認を進めない．バッファが空くまで受信窓を保持し，過大セグメントを黙って破棄しない． */
    if (p->tot_len > queue_space(net_rx_head, net_rx_tail)) {
        return ERR_MEM; /* 同じpbufは後でlwIPが再試行する． */
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

/* 単一のTelnetクライアントを受け入れ，TTY用の新しいセッション番号を与える． */
static err_t client_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
    /* APは指定した端末だけを許可する．STAでは外部APがアドレスを割り当てたLAN上の端末を受け付ける． */
#if !PICONIX_WIFI_STA
    ip4_addr_t allowed;
    IP4_ADDR(&allowed, 192, 168, 4, 2);
#endif
    if (err != ERR_OK || client ||
        !IP_IS_V4(&pcb->remote_ip)
#if !PICONIX_WIFI_STA
        || !ip4_addr_cmp(ip_2_ip4(&pcb->remote_ip), &allowed)
#endif
        ) {
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

/* TCP 23番のリスナを用意する． */
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

/* 保留されたTelnet送信と切断処理をアイドル側で進める． */
void net_service_telnet_poll(void) { net_terminal_flush(); }
#else
/* TCP 23番のリスナを用意する． */
int net_service_telnet_init(void) { return 0; }

/* 保留されたTelnet送信と切断処理をアイドル側で進める． */
void net_service_telnet_poll(void) {}

/* Telnetクライアントの接続有無を返す．非Wi-Fi版は常に未接続． */
int net_terminal_connected(void) { return 0; }

/* 端末接続の世代番号を返す．切断・再接続の検知に用いる． */
unsigned net_terminal_epoch(void) { return 0; }

/* 受信キューから指定した最大バイト数を取り出す．切断時は-1，データ待ちなら0，成功時は読出しバイト数を返す． */
int net_terminal_read(void *dst, unsigned size) { (void)dst; (void)size; return -1; }

/* TTY送信バイトを固定長キューに積み，積めた量を返す．切断時は-1．CRLFの途中だけを採用しない． */
int net_terminal_write(const void *src, unsigned size) { (void)src; (void)size; return -1; }
#endif
