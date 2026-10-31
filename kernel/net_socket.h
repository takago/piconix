/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include <stdint.h>
/* Internal UDP transport. All calls must run in the lwIP polling context. */
#define NET_SOCKET_UDP_MAX 512u
int net_socket_udp_bind(uint16_t port);
/* Returns -3 if no datagram; 0 is a valid empty UDP datagram. */
int net_socket_udp_recv(void *buf, unsigned capacity, uint32_t *peer_ip, uint16_t *peer_port);
int net_socket_udp_send(const void *buf, unsigned length, uint32_t peer_ip, uint16_t peer_port);
void net_socket_udp_close(void);

/* TCP listener backend (experimental; not yet exposed to userland). */
int net_socket_tcp_listen(uint16_t port);
int net_socket_tcp_connected(void);
/* peer_ip uses lwIP network-order IPv4 representation. */
int net_socket_tcp_connect(uint32_t peer_ip, uint16_t peer_port);
/* 1=connected, 0=connecting/not connected, -1=error, -2=peer EOF. */
int net_socket_tcp_status(void);
/* Positive bytes, 0 = no data, -2 = EOF, -1 = invalid request. */
int net_socket_tcp_read(void *dst, unsigned capacity);
/* Positive bytes, 0 = send queue full, -1 = disconnected. */
int net_socket_tcp_write(const void *src, unsigned length);
void net_socket_tcp_close(void);
int net_socket_tcp_accept_ready(void);
int net_socket_tcp_listening(void);
void net_socket_tcp_close_listener(void);
void net_socket_tcp_close_client(void);

/* Local port for a live socket: UDP=0, TCP listener=1, TCP peer=2. */
uint16_t net_socket_local_port(int kind);
