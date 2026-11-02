/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include <stdint.h>
/* Internal UDP transport. Called from syscalls or the idle task; never IRQs.
 * lwIP polling and user socket syscalls must not run concurrently. */
#define NET_SOCKET_UDP_MAX 512u
int net_socket_udp_bind(uint16_t port);
/* Returns -3 if no datagram; 0 is a valid empty UDP datagram. */
int net_socket_udp_recv(void *buf, unsigned capacity, uint32_t *peer_ip, uint16_t *peer_port);
int net_socket_udp_send(const void *buf, unsigned length, uint32_t peer_ip, uint16_t peer_port);
void net_socket_udp_close(void);

/* Fixed-size user TCP socket pool (shared by backend and FD ownership). */
#define NET_SOCKET_TCP_SLOTS 4
/* TCP listener and client backend. */
int net_socket_tcp_alloc(void);
int net_socket_tcp_listen(int id, uint16_t port);
int net_socket_tcp_connected(int id);
/* peer_ip uses lwIP network-order IPv4 representation. */
int net_socket_tcp_connect(int id, uint32_t peer_ip, uint16_t peer_port);
/* 1=connected, 0=connecting/not connected, -1=error, -2=peer EOF. */
int net_socket_tcp_status(int id);
/* Positive bytes, 0 = no data, -2 = EOF, -1 = invalid request. */
int net_socket_tcp_read(int id, void *dst, unsigned capacity);
/* Positive bytes, 0 = send queue full, -1 = disconnected. */
int net_socket_tcp_write(int id, const void *src, unsigned length);
/* Half-close TCP send side, preserving receive side. */
int net_socket_tcp_shutdown_write(int id);
void net_socket_tcp_close(int id);
int net_socket_tcp_accept_ready(int id);
int net_socket_tcp_listening(int id);
void net_socket_tcp_close_listener(int id);
void net_socket_tcp_close_client(int id);

/* Local port for a live socket: UDP=0, TCP listener=1, TCP peer=2. */
uint16_t net_socket_local_port(int kind, int id);
