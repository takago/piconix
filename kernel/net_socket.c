/* SPDX-License-Identifier: GPL-3.0-or-later */
/* lwIPのUDP/TCPエンドポイントとユーザFDとの橋渡しを行う．
 * パケットと接続状態は固定長領域に保持し，lwIPコールバックで動的確保しない． */

#include "net_socket.h"

/* 関数トレース対象以外にはコンパイラの計測フックを挿入しない． */
#define NO_TRACE __attribute__((no_instrument_function))
#ifndef PICONIX_WIFI
#define PICONIX_WIFI 0
#endif
#if PICONIX_WIFI
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/ip4_addr.h"
#include <string.h>
/* 各ソケットが受信データグラムを1件保持する固定長領域． */
typedef struct {
    struct udp_pcb *pcb;
    unsigned char rx_data[NET_SOCKET_UDP_MAX];
    unsigned rx_length;
    uint32_t rx_peer_ip;
    uint16_t rx_peer_port;
    unsigned rx_ready;
} udp_slot_t;
static udp_slot_t udp_slots[NET_SOCKET_UDP_SLOTS];
/* UDPソケットの番号を検証して対応する固定スロットを返す． */
NO_TRACE static udp_slot_t *udp_slot_at(int id) {
    return id>=0 && id<NET_SOCKET_UDP_SLOTS ? &udp_slots[id] : NULL;
}

/* lwIPコールバックは登録時に渡されたスロットの受信領域だけを書き換える． */
NO_TRACE static void udp_received(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                         const ip_addr_t *addr, u16_t port) {
    (void)pcb;
    udp_slot_t *s=(udp_slot_t *)arg;
    if (!p) return;
    if (s && !s->rx_ready && IP_IS_V4(addr) && p->tot_len <= sizeof(s->rx_data)) {
        s->rx_length=pbuf_copy_partial(p,s->rx_data,p->tot_len,0);
        s->rx_peer_ip=ip4_addr_get_u32(ip_2_ip4(addr));
        s->rx_peer_port=port;
        s->rx_ready=1;
    }
    pbuf_free(p);
}

/* 指定ソケットが読み取り可能か調べる． */
NO_TRACE int net_socket_udp_read_ready(int id) {
    udp_slot_t *s=udp_slot_at(id);
    return s && s->rx_ready;
}

/* UDP PCBの生成・bind・受信コールバック設定を一か所にまとめる．
 * 失敗時はPCBを解放し，スロットの所有状態を変更しない． */
NO_TRACE static int udp_slot_open(udp_slot_t *s, uint16_t port) {
    struct udp_pcb *pcb = udp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb) return -1;
    if (udp_bind(pcb, IP_ANY_TYPE, port) != ERR_OK) {
        udp_remove(pcb);
        return -1;
    }
    s->pcb = pcb;
    s->rx_ready = 0;
    udp_recv(pcb, udp_received, s);
    return 0;
}

/* 指定スロットのUDP待受ポートを登録する． */
int net_socket_udp_bind(int id, uint16_t port) {
    udp_slot_t *s=udp_slot_at(id);
    if (!s || s->pcb || !port) return -1;
    return udp_slot_open(s, port);
}

/* スロット固有の保留データグラムを取り出す． */
int net_socket_udp_recv(int id,void *buf,unsigned capacity,uint32_t *peer_ip,uint16_t *peer_port) {
    udp_slot_t *s=udp_slot_at(id);
    if (!s || !buf) return -1;
    if (!s->pcb || !s->rx_ready) return -3;
    if (capacity<s->rx_length) return -2;
    memcpy(buf,s->rx_data,s->rx_length);
    if (peer_ip) *peer_ip=s->rx_peer_ip;
    if (peer_port) *peer_port=s->rx_peer_port;
    unsigned n=s->rx_length;
    s->rx_ready=0;
    return (int)n;
}

/* UDPクライアントは初回送信時に自身のPCBと送信元ポートを取得する． */
int net_socket_udp_send(int id,const void *buf,unsigned length,uint32_t peer_ip,uint16_t peer_port) {
    udp_slot_t *s=udp_slot_at(id);
    if (!s || (!buf && length) || length>NET_SOCKET_UDP_MAX || !peer_port) return -1;
    if (!s->pcb) {
        if (udp_slot_open(s, 0) != 0) return -1;
    }
    struct pbuf *pkt=pbuf_alloc(PBUF_TRANSPORT,length,PBUF_RAM);
    if (!pkt) return -1;
    if (length && pbuf_take(pkt,buf,length)!=ERR_OK) { pbuf_free(pkt); return -1; }
    ip_addr_t dest;
    ip_addr_set_ip4_u32(&dest,peer_ip);
    err_t err=udp_sendto(s->pcb,pkt,&dest,peer_port);
    pbuf_free(pkt);
    return err==ERR_OK ? (int)length : -1;
}

/* ソケットを閉じるときは対象スロットだけを破棄する． */
NO_TRACE void net_socket_udp_close(int id) {
    udp_slot_t *s=udp_slot_at(id);
    if (!s) return;
    if (s->pcb) { udp_remove(s->pcb); s->pcb=NULL; }
    s->rx_ready=0;
}
/* PicoNIXのTCPシステムコールを支えるlwIP側の実装． */
#include "lwip/tcp.h"
#include <stdio.h>
/* 異常経路だけを数える固定長診断．lwIPコールバック中にprintfしない． */
static struct {
    unsigned pcb_alloc_fail, accept_busy, accept_error, rx_backpressure;
    unsigned write_backpressure, write_error, output_error;
    unsigned close_deferred, close_retry, async_error;
} tcp_diag;
/* 拒否したpbuf一個で受信窓が満杯になる場合もある．アプリが読出した後に全体を受け取れるよう，適切な領域を確保する． */
#define NET_SOCKET_TCP_RX_MAX TCP_WND
typedef struct {
    struct tcp_pcb *listener, *client;
    unsigned char rx[NET_SOCKET_TCP_RX_MAX];
    unsigned rx_len;
    unsigned char peer_closed, fault, connecting, established, pending_accept, allocated;
    unsigned char write_shutdown;
} tcp_slot_t;
static tcp_slot_t slots[NET_SOCKET_TCP_SLOTS];
/* IDが有効な範囲にある場合だけ固定TCPスロットのポインタを返す． */
NO_TRACE static tcp_slot_t *slot_at(int id) {
    return id>=0 && id<NET_SOCKET_TCP_SLOTS ? &slots[id] : NULL;
}

/* 固定TCP管理スロットを確保する．空きがなければ-1を返す． */
int net_socket_tcp_alloc(void) {
    for(int i=0;i<NET_SOCKET_TCP_SLOTS;i++)
        if(!slots[i].allocated) {
            memset(&slots[i],0,sizeof slots[i]);
            slots[i].allocated=1;
            return i;
        }
    return -1;
}

/* 非同期TCP接続の完了を受け，接続状態とエラーを更新する． */
NO_TRACE static err_t socket_tcp_connected_cb(void *arg, struct tcp_pcb *pcb, err_t err) {
    (void)pcb;
    tcp_slot_t *s=(tcp_slot_t *)arg;
    s->connecting = 0;
    if (err == ERR_OK) { s->established = 1; return ERR_OK; }
    s->fault = 1;
    return err;
}

/* lwIPがTCP PCBを破棄したときに管理スロットの参照を解除する． */
NO_TRACE static void socket_tcp_err(void *arg, err_t err) {
    (void)err;
    tcp_diag.async_error++;
    tcp_slot_t *s=(tcp_slot_t *)arg;
    s->client = NULL; /* lwIPが既にPCBを破棄した後である． */
    s->pending_accept = 0;
    s->fault = 1;
    s->connecting = s->established = 0;
    s->write_shutdown = 0;
}

/* TCP受信コールバック．受信窓に空きがないときはERR_MEMとし，ユーザが読み終える前に受信済み扱いしない． */
NO_TRACE static err_t socket_tcp_recv(void *arg, struct tcp_pcb *pcb,
                             struct pbuf *p, err_t err) {
    tcp_slot_t *s=(tcp_slot_t *)arg;
    if (err != ERR_OK)
        return err;
    if (!p) { s->peer_closed = 1; return ERR_OK; }
    /* ERR_MEMではpbufの所有権をlwIPに残し，受信窓が空くまで再試行してもらう．ユーザが読む前に受信確認を進めない． */
    if (p->tot_len > NET_SOCKET_TCP_RX_MAX - s->rx_len) {
        tcp_diag.rx_backpressure++;
        return ERR_MEM;
    }
    pbuf_copy_partial(p, s->rx + s->rx_len, p->tot_len, 0);
    s->rx_len += p->tot_len;
    pbuf_free(p);
    (void)pcb;
    return ERR_OK;
}

/* 受け付けたTCP接続を管理スロットに登録し，accept側へ新しい接続の到着を通知する． */
NO_TRACE static err_t socket_tcp_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
    tcp_slot_t *s=(tcp_slot_t *)arg;
    if (err != ERR_OK || s->client) {
        if (s->client) tcp_diag.accept_busy++;
        else tcp_diag.accept_error++;
        return ERR_VAL;
    }
    s->client = pcb;
    s->rx_len = 0;
    s->peer_closed = s->fault = s->connecting = s->write_shutdown = 0;
    s->established = 1;
    s->pending_accept = 1;
    tcp_arg(pcb, s);
    tcp_recv(pcb, socket_tcp_recv);
    tcp_err(pcb, socket_tcp_err);
    return ERR_OK;
}

/* 指定スロットでTCP待受を始める．最大接続数は固定のlwIP資源に制限される． */
int net_socket_tcp_listen(int id, uint16_t port) {
    tcp_slot_t *s=slot_at(id);
    if(!s || !s->allocated || s->listener || s->client || !port)
        return -1;
    struct tcp_pcb *pcb=tcp_new_ip_type(IPADDR_TYPE_V4);
    if(!pcb) { tcp_diag.pcb_alloc_fail++; return -1; }
    /* 既存接続を閉じた後，サーバをすぐ再起動できるようにする． */
    ip_set_option(pcb, SOF_REUSEADDR);
    if(tcp_bind(pcb,IP_ANY_TYPE,port)!=ERR_OK) { tcp_close(pcb); return -1; }
    struct tcp_pcb *listener=tcp_listen_with_backlog(pcb,1);
    if(!listener) { tcp_close(pcb); return -1; }
    s->listener=listener;
    tcp_arg(listener,s);
    tcp_accept(listener,socket_tcp_accept);
    return 0;
}

/* TCP接続を非同期に開始する．結果は接続状態の問い合わせで確認する． */
int net_socket_tcp_connect(int id,uint32_t ip,uint16_t port) {
    tcp_slot_t *s=slot_at(id);
    if(!s || !s->allocated || s->listener || s->client || !port)
        return -1;
    struct tcp_pcb *pcb=tcp_new_ip_type(IPADDR_TYPE_V4);
    if(!pcb) { tcp_diag.pcb_alloc_fail++; return -1; }
    s->client=pcb;
    s->rx_len=0;
    s->peer_closed=s->fault=s->established=s->write_shutdown=0;
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

/* selectは受付フラグを消さず，accept()に引き継ぐ． */
NO_TRACE int net_socket_tcp_accept_peek(int id) {
    tcp_slot_t *s=slot_at(id);
    return s && s->listener && s->pending_accept && s->client;
}
/* TCP受信データまたは切断・エラーの通知があるか調べる． */
NO_TRACE int net_socket_tcp_recv_ready(int id) {
    tcp_slot_t *s=slot_at(id);
    return s && (s->rx_len || s->peer_closed || s->fault);
}
/* TCP送信バッファに空きがあり書き込み可能か調べる． */
NO_TRACE int net_socket_tcp_write_ready(int id) {
    tcp_slot_t *s=slot_at(id);
    return s && s->client && s->established && !s->fault &&
           !s->peer_closed && !s->write_shutdown && tcp_sndbuf(s->client)>0;
}

/* 待受スロットに受け取り可能な接続があるか確認する． */
NO_TRACE int net_socket_tcp_accept_ready(int id) {
    tcp_slot_t *s=slot_at(id);
    if(!s || !s->listener || !s->pending_accept || !s->client)
        return 0;
    s->pending_accept=0;
    return 1;
}

/* 指定TCPスロットが待受状態なら真を返す． */
NO_TRACE int net_socket_tcp_listening(int id) { tcp_slot_t *s=slot_at(id); return s && s->listener; }

/* TCP待受PCBを閉じる．送受信中の接続とは別に後始末する． */
NO_TRACE void net_socket_tcp_close_listener(int id) {
    tcp_slot_t *s=slot_at(id);
    if(!s || !s->listener)
        return;
    struct tcp_pcb *pcb=s->listener;
    s->listener=NULL;
    tcp_arg(pcb,NULL); tcp_accept(pcb,NULL);
    if(tcp_close(pcb)!=ERR_OK)
        tcp_abort(pcb);
}

/* 保留したtcp_closeはTCPタイマーコールバックからだけ再試行する．ACK処理中にcloseするとlwIPの送信キュー処理と競合するためである． */
NO_TRACE static err_t socket_tcp_closing_poll(void *arg,struct tcp_pcb *pcb) {
    (void)arg;
    if(tcp_close(pcb)==ERR_MEM) tcp_diag.close_retry++; /* ACK処理中には閉じない． */
    return ERR_OK;
}

/* 終了処理中に到着した受信バッファを破棄して受信窓を進める． */
NO_TRACE static err_t socket_tcp_closing_recv(void *arg,struct tcp_pcb *pcb,struct pbuf *p,err_t err) {
    (void)arg; (void)err;
    /* アプリから切り離した接続に遅延到着したデータは消費する．pbuf解放後はERR_MEMでなくERR_OKを返す． */
    if(p) { tcp_recved(pcb,p->tot_len); pbuf_free(p); }
    return ERR_OK;
}

/* アプリから切り離したTCP接続を閉じる．lwIP送信残がある場合はタイマーでcloseを再試行する． */
NO_TRACE void net_socket_tcp_close_client(int id) {
    tcp_slot_t *s=slot_at(id);
    if(!s)
        return;
    if(s->client) {
        struct tcp_pcb *pcb=s->client;
        s->client=NULL;
        tcp_arg(pcb,NULL); tcp_err(pcb,NULL);
        tcp_recv(pcb,socket_tcp_closing_recv);
        if(tcp_close(pcb)!=ERR_OK) {
            tcp_diag.close_deferred++;
            /* ACKの処理はlwIPに任せ，closeはタイマーで再試行する． */
            tcp_sent(pcb,NULL);
            tcp_poll(pcb,socket_tcp_closing_poll,2);
        }
    }
    s->rx_len=0;
    s->pending_accept=s->peer_closed=s->fault=s->connecting=s->established=s->write_shutdown=0;
}

/* 指定TCPスロットの接続が確立済みか返す． */
NO_TRACE int net_socket_tcp_connected(int id) { tcp_slot_t *s=slot_at(id); return s && s->client && s->established; }

/* TCP接続の状態を返す．無効なスロットや通信異常ならエラー． */
NO_TRACE int net_socket_tcp_status(int id) {
    tcp_slot_t *s=slot_at(id);
    if(!s)
        return -1;
    if(s->fault)
        return -1;
    if(s->peer_closed && !s->rx_len)
        return -2;
    return s->established ? 1 : 0;
}

/* 固定受信キューから最大capacityバイトをコピーする．消費した分だけlwIPに受信窓の解放を通知する． */
int net_socket_tcp_read(int id,void *dst,unsigned capacity) {
    tcp_slot_t *s=slot_at(id);
    if(!s || !dst || !capacity)
        return -1;
    if(s->rx_len) {
        unsigned n=capacity<s->rx_len ? capacity : s->rx_len;
        memcpy(dst,s->rx,n);
        memmove(s->rx,s->rx+n,s->rx_len-n);
        s->rx_len-=n;
        if(s->client)
            tcp_recved(s->client,(u16_t)n);
        return (int)n;
    }
    return s->peer_closed || s->fault ? -2 : 0;
}

/* 送信バッファとMSSに収まる範囲だけをlwIPへ渡す．部分成功バイト数を返し，上位側で残りを再送する． */
int net_socket_tcp_write(int id,const void *src,unsigned length) {
    tcp_slot_t *s=slot_at(id);
    if(!s || !s->client || !s->established || s->peer_closed || s->fault ||
       s->write_shutdown || (!src && length)) return -1;
    if(!length)
        return 0;
    unsigned n=length;
    if(n>tcp_sndbuf(s->client))
        n=tcp_sndbuf(s->client);
    if(n>TCP_MSS) n=TCP_MSS;
    if(!n)
        return 0;
    err_t err=tcp_write(s->client,src,(u16_t)n,TCP_WRITE_FLAG_COPY);
    if(err==ERR_MEM) { tcp_diag.write_backpressure++; return 0; } /* ACKを待って再試行する． */
    if(err!=ERR_OK) { tcp_diag.write_error++; return -1; } /* メモリ不足以外のエラーは単なる流量制限とは扱わない． */
    /* tcp_outputに失敗してもtcp_writeで受け付けたバイトはキューに残る．呼出側が同じバイトを再送すると重複するため，成功長を返す． */
    if(tcp_output(s->client)!=ERR_OK) tcp_diag.output_error++;
    return (int)n;
}

/* TCPの送信方向だけを終了し，受信方向は維持する． */
NO_TRACE int net_socket_tcp_shutdown_write(int id) {
    tcp_slot_t *s=slot_at(id);
    if(!s || !s->client || !s->established || s->fault || s->write_shutdown)
        return -1;
    if(tcp_shutdown(s->client,0,1)!=ERR_OK)
        return -1;
    s->write_shutdown=1;
    return 0;
}

/* 指定TCPスロットのクライアントと待受を閉じ，管理スロットを解放する． */
void net_socket_tcp_close(int id) {
    net_socket_tcp_close_client(id);
    net_socket_tcp_close_listener(id);
    tcp_slot_t *s=slot_at(id);
    if(s)
        s->allocated=0;
}

/* /proc/net/memoryから参照する累積TCPエラー統計．正常な接続は数えない． */
NO_TRACE int net_socket_tcp_diagnostics(char *out, unsigned size) {
    if(!size) return 0;
    return snprintf(out,size,
        "TCP_ALLOC_FAIL: %u\nTCP_ACCEPT_BUSY: %u\nTCP_ACCEPT_ERROR: %u\n"
        "TCP_RX_BACKPRESSURE: %u\nTCP_WRITE_BACKPRESSURE: %u\n"
        "TCP_WRITE_ERROR: %u\nTCP_OUTPUT_ERROR: %u\n"
        "TCP_CLOSE_DEFERRED: %u\nTCP_CLOSE_RETRY: %u\nTCP_ASYNC_ERROR: %u\n",
        tcp_diag.pcb_alloc_fail,tcp_diag.accept_busy,tcp_diag.accept_error,
        tcp_diag.rx_backpressure,tcp_diag.write_backpressure,
        tcp_diag.write_error,tcp_diag.output_error,
        tcp_diag.close_deferred,tcp_diag.close_retry,tcp_diag.async_error);
}

/* UDPまたはTCPの指定ソケットに割り当てられたローカルポートを返す． */
NO_TRACE uint16_t net_socket_local_port(int kind,int id) {
    if(kind==0) {
        udp_slot_t *u = udp_slot_at(id);
        return u && u->pcb ? u->pcb->local_port : 0;
    }
    tcp_slot_t *s=slot_at(id);
    if(!s)
        return 0;
    if(kind==1)
        return s->listener ? s->listener->local_port : (s->client ? s->client->local_port : 0);
    if(kind==2)
        return s->client ? s->client->local_port : 0;
    return 0;
}

#else
/* UDP待受ポートをlwIPへ登録する．不正ポート・既存状態などの失敗時は負値を返す． */
int net_socket_udp_bind(int id,uint16_t port) { (void)id; (void)port; return -1; }
/* UDPの受信データがあるか確認する． */
NO_TRACE int net_socket_udp_read_ready(int id) { (void)id; return 0; }

/* 保持中の一つのデータグラムを取り出す．データがなければ再試行可能な状態を返す． */
int net_socket_udp_recv(int id,void *b, unsigned n, uint32_t *a, uint16_t *p) {
    (void)id; (void)b; (void)n; (void)a; (void)p; return -1;
}

/* 宛先IPv4とポートを指定してUDPデータグラムを送る．必要なら最初の送信時にPCBを用意する． */
int net_socket_udp_send(int id,const void *b, unsigned n, uint32_t a, uint16_t p) {
    (void)id; (void)b; (void)n; (void)a; (void)p; return -1;
}

/* UDPソケットを解放し，受信状態をリセットする．非Wi-Fi版では何もしない． */
NO_TRACE void net_socket_udp_close(int id) { (void)id; }

/* 固定TCP管理スロットを確保する．空きがなければ-1を返す． */
int net_socket_tcp_alloc(void) { return -1; }

/* 指定スロットでTCP待受を始める．最大接続数は固定のlwIP資源に制限される． */
int net_socket_tcp_listen(int id,uint16_t port) { (void)id; (void)port; return -1; }

/* 待受スロットに受け取り可能な接続があるか確認する． */
NO_TRACE int net_socket_tcp_accept_ready(int id) { (void)id; return 0; }
/* TCP待受スロットに受け付け可能な接続があるか確認する． */
NO_TRACE int net_socket_tcp_accept_peek(int id) { (void)id; return 0; }
/* TCP受信データまたは切断・エラーの通知があるか調べる． */
NO_TRACE int net_socket_tcp_recv_ready(int id) { (void)id; return 0; }
/* TCP送信バッファに空きがあり書き込み可能か調べる． */
NO_TRACE int net_socket_tcp_write_ready(int id) { (void)id; return 0; }

/* 指定TCPスロットが待受状態なら真を返す． */
NO_TRACE int net_socket_tcp_listening(int id) { (void)id; return 0; }

/* TCP待受PCBを閉じる．送受信中の接続とは別に後始末する． */
NO_TRACE void net_socket_tcp_close_listener(int id) { (void)id; }

/* アプリから切り離したTCP接続を閉じる．lwIP送信残がある場合はタイマーでcloseを再試行する． */
NO_TRACE void net_socket_tcp_close_client(int id) { (void)id; }

/* 指定TCPスロットの接続が確立済みか返す． */
NO_TRACE int net_socket_tcp_connected(int id) { (void)id; return 0; }

/* TCP接続の状態を返す．無効なスロットや通信異常ならエラー． */
NO_TRACE int net_socket_tcp_status(int id) { (void)id; return -1; }

/* TCP接続を非同期に開始する．結果は接続状態の問い合わせで確認する． */
int net_socket_tcp_connect(int id,uint32_t ip,uint16_t port) { (void)id; (void)ip; (void)port; return -1; }

/* 固定受信キューから最大capacityバイトをコピーする．消費した分だけlwIPに受信窓の解放を通知する． */
int net_socket_tcp_read(int id,void *dst,unsigned n) { (void)id; (void)dst; (void)n; return -1; }

/* 送信バッファとMSSに収まる範囲だけをlwIPへ渡す．部分成功バイト数を返し，上位側で残りを再送する． */
int net_socket_tcp_write(int id,const void *src,unsigned n) { (void)id; (void)src; (void)n; return -1; }

/* TCPの送信方向だけを終了し，受信方向は維持する． */
NO_TRACE int net_socket_tcp_shutdown_write(int id) { (void)id; return -1; }

/* 指定TCPスロットのクライアントと待受を閉じ，管理スロットを解放する． */
void net_socket_tcp_close(int id) { (void)id; }

/* 非Wi-Fi版ではTCP資源を確保しない． */
NO_TRACE int net_socket_tcp_diagnostics(char *out,unsigned size) { (void)out;(void)size;return 0; }

/* UDPまたはTCPの指定ソケットに割り当てられたローカルポートを返す． */
NO_TRACE uint16_t net_socket_local_port(int kind,int id) { (void)kind; (void)id; return 0; }
#endif
