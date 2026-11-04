/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
void net_init(void);
void net_poll(void);
/* A fixed record interface: no raw socket syscall or dynamic allocation. */
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
/* True while the idle task is running lwIP callbacks and raw operations. */
int net_poll_in_progress(void);

/* TCP transport for TTY2; the kernel owns the remote shell lifecycle. */
int net_terminal_connected(void);
/* Changes on every accepted connection and disconnect. */
unsigned net_terminal_epoch(void);
int net_terminal_read(void *dst, unsigned size);
int net_terminal_write(const void *src, unsigned size);

/* Format the current Wi-Fi interface state for /proc/net/wifi. */
int net_wifi_info(char *out, unsigned size);
/* /proc/net/memory: lwIP heap/pbuf counters and idle stack watermark. */
int net_memory_info(char *out, unsigned size);
/* Read-only diagnostic snapshots.  Type 0: AP associations, 1: ARP. */
int net_diag_info(char *out, unsigned size, int type);

/* Queue an STA reassociation from a syscall; applied in net_poll(). */
int net_wifi_request_restart(void);
