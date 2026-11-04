/* lwIPのメモリ・プロトコル設定．Pico WのSRAM容量を前提に固定値を選ぶ． */
/* PicoNIXのAP/STAで使う最小限のlwIP設定． */
#pragma once
#define NO_SYS 1
#define LWIP_SOCKET 0
/* lwIPのパケットを変更・消費せずにIPv4受信を観測する． */
#include "net_capture_hook.h"
#define LWIP_HOOK_IP4_INPUT(p,n) piconix_packet_input((p),(n))
#define LWIP_RAW 1
/* 以前の接続終了後にもTCP待受ポートを直ちに再利用できるようにする． */
#define SO_REUSE 1
#define LWIP_NETCONN 0
#define LWIP_TCP 1
#define LWIP_UDP 1
/* ローカルTCP/UDP通信ではlwIP標準の127.0.0.1を使う．NO_SYSではキューを明示的に処理し，tcp_inputの再帰呼出しを避ける． */
#define LWIP_NETIF_LOOPBACK 1
#define LWIP_HAVE_LOOPIF 1
#define LWIP_LOOPBACK_MAX_PBUFS 8
#define LWIP_NETIF_LOOPBACK_MULTITHREADING 0
/* STAはlwIPのDHCPクライアントを使い，APはPicoNIX独自のDHCPサーバを使う． */
#if defined(PICONIX_WIFI_STA) && PICONIX_WIFI_STA
#define LWIP_DHCP 1
#else
#define LWIP_DHCP 0
#endif
/* リース取得前のDHCPクライアントが0.0.0.0を送信元にしたIPv4を送れるようにする． */
#define LWIP_IP_ACCEPT_UDP_PORT(port) ((port) == PP_NTOHS(67))
#define LWIP_DNS 0
#define MEM_ALIGNMENT 4
/* Pico Wのネットワーク予約領域16 KiBはlwIPヒープ10 KiB，境界領域約2 KiB，アイドル／通信スタック4 KiBに分割する．pbuf・TCP PCB・CYW43の静的領域はカーネルRAM側に残り，境界領域には管理情報も含む． */
#define MEM_SIZE (10 * 1024)
#define LWIP_RAM_HEAP_POINTER ((void *)0x2003C000u)
#define PBUF_POOL_SIZE 8
/* lwIPヒープとプールの使用量・確保失敗を軽量に集計する． */
#define LWIP_STATS 1
#define MEM_STATS 1
#define MEMP_STATS 1
#define PBUF_POOL_BUFSIZE 512
#define MEMP_NUM_TCP_PCB 5
#define MEMP_NUM_TCP_PCB_LISTEN 5 /* TelnetとユーザTCPで使用する待受枠を確保する．HTTP等も同じ枠を共有する． */
#define MEMP_NUM_UDP_PCB 4 /* DHCP・デバイス制御・ユーザUDP・mDNSで使う枠． */
#define LWIP_MDNS_RESPONDER 1
/* mDNSの探索・通知ではlwIP基本タイマー以外の枠が必要になるため，STA/APとも一時タイマーを4枠余分に確保する． */
#define MEMP_NUM_SYS_TIMEOUT (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 4)
#define LWIP_IGMP 1
#define LWIP_NUM_NETIF_CLIENT_DATA 1
#define TCP_MSS 536
#define TCP_WND (2 * TCP_MSS)
#define TCP_SND_BUF (2 * TCP_MSS)
#define TCP_SND_QUEUELEN 8
#define LWIP_NETIF_STATUS_CALLBACK 0
#define LWIP_NETIF_LINK_CALLBACK 0
#define LWIP_PROVIDE_ERRNO 1

