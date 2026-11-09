/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 固定数のネットワークソケットを操作するための内部インターフェース． */

#pragma once
#include <stdint.h>
/* UDP内部通信API．システムコールまたはアイドル側から呼ぶが，割込み内では呼ばない．lwIPポーリングとユーザソケットSVCを同時実行させない． */
#define NET_SOCKET_UDP_MAX 512u
#define NET_SOCKET_UDP_SLOTS 3
/* UDPソケットはFDごとの管理スロットに対応する． */
int net_socket_udp_bind(int id, uint16_t port);
int net_socket_udp_read_ready(int id);
/* データグラムなしは-3を返し，長さ0の正常なデータグラムと区別する． */
int net_socket_udp_recv(int id, void *buf, unsigned capacity, uint32_t *peer_ip, uint16_t *peer_port);
int net_socket_udp_send(int id, const void *buf, unsigned length, uint32_t peer_ip, uint16_t peer_port);
void net_socket_udp_close(int id);

/* 固定数のユーザTCPソケット枠．バックエンドとFD所有情報で共有する． */
#define NET_SOCKET_TCP_SLOTS 4
/* TCP待受およびクライアント用の内部API． */
int net_socket_tcp_alloc(void);
int net_socket_tcp_listen(int id, uint16_t port);
int net_socket_tcp_connected(int id);
/* peer_ipはlwIPのネットワーク順IPv4表現を使う． */
int net_socket_tcp_connect(int id, uint32_t peer_ip, uint16_t peer_port);
/* 接続状態は1=接続済み，0=接続中／未接続，-1=エラー，-2=相手側EOF． */
int net_socket_tcp_status(int id);
/* 正値は受信バイト数，0はデータなし，-2はEOF，-1は不正な要求． */
int net_socket_tcp_read(int id, void *dst, unsigned capacity);
/* 正値は送信バイト数，0はキュー満杯，-1は切断中． */
int net_socket_tcp_write(int id, const void *src, unsigned length);
/* 送信方向のみを半閉鎖し，受信方向は維持する． */
int net_socket_tcp_shutdown_write(int id);
void net_socket_tcp_close(int id);
int net_socket_tcp_accept_ready(int id);
/* select専用: 受信キューやaccept受付フラグを消費しない． */
int net_socket_tcp_accept_peek(int id);
int net_socket_tcp_recv_ready(int id);
int net_socket_tcp_write_ready(int id);
int net_socket_tcp_listening(int id);
void net_socket_tcp_close_listener(int id);
void net_socket_tcp_close_client(int id);

/* 使用中ソケットのローカルポート．種別0はUDP，1はTCP待受，2はTCP接続先． */
uint16_t net_socket_local_port(int kind, int id);

/* TCP送信・接続・終了の異常経路カウンタを/proc/net/memoryに表示する． */
int net_socket_tcp_diagnostics(char *out, unsigned size);
