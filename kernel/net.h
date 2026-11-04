/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Wi-Fi初期化，ポーリング，診断表示の内部インターフェース． */

#pragma once
void net_init(void);
void net_poll(void);
/* 固定形式のパケット観測インターフェース．rawソケットシステムコールや動的確保は行わない． */
#include "../include/piconix_net_capture.h"
uint32_t net_packet_serial(void);
int net_packet_read(uint32_t *serial, piconix_packet_t *record);
int net_echo_write(const piconix_echo_t *request);
int net_echo_read(piconix_echo_t *reply);
void net_echo_close(void);
void net_packet_reader(int delta);
struct netif;
void net_capture_attach(struct netif *iface);
void net_capture_init(void);
void net_capture_poll(void);
/* アイドルタスクがlwIPコールバックやraw APIを処理中かを示す． */
int net_poll_in_progress(void);

/* TTY2のTCP通信入口．リモートシェルの生存期間はカーネルが管理する． */
int net_terminal_connected(void);
/* 接続の確立／切断ごとに変化するセッション番号． */
unsigned net_terminal_epoch(void);
int net_terminal_read(void *dst, unsigned size);
int net_terminal_write(const void *src, unsigned size);

/* /proc/net/wifi用に現在の無線状態を整形する． */
int net_wifi_info(char *out, unsigned size);
/* /proc/net/memory用のlwIP統計とアイドルスタック高水位． */
int net_memory_info(char *out, unsigned size);
/* 読出し専用の診断スナップショット．種別0はAP接続，1はARPを表す． */
int net_diag_info(char *out, unsigned size, int type);

/* SVCからSTA再接続要求を予約し，実際の処理はnet_pollで行う． */
int net_wifi_request_restart(void);
