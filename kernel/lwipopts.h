/* Minimal lwIP options for PicoNIX AP bring-up. */
#pragma once
#define NO_SYS 1
#define LWIP_SOCKET 0
/* Allow immediate listener rebind after previous TCP connections close. */
#define SO_REUSE 1
#define LWIP_NETCONN 0
#define LWIP_TCP 1
#define LWIP_UDP 1
/* Local TCP/UDP traffic uses lwIP's built-in 127.0.0.1 interface.
 * NO_SYS requires explicit queue polling; never re-enter tcp_input(). */
#define LWIP_NETIF_LOOPBACK 1
#define LWIP_HAVE_LOOPIF 1
#define LWIP_LOOPBACK_MAX_PBUFS 8
#define LWIP_NETIF_LOOPBACK_MULTITHREADING 0
/* STA needs the lwIP DHCP client; AP uses PicoNIX's own DHCP server. */
#if defined(PICONIX_WIFI_STA) && PICONIX_WIFI_STA
#define LWIP_DHCP 1
#else
#define LWIP_DHCP 0
#endif
/* Permit DHCP clients to send IPv4 packets from 0.0.0.0 before leasing. */
#define LWIP_IP_ACCEPT_UDP_PORT(port) ((port) == PP_NTOHS(67))
#define LWIP_DNS 0
#define MEM_ALIGNMENT 4
/* Pico W network bank (16 KiB):
 * 0x2003c000..0x2003e7ff  lwIP heap (10 KiB, plus allocator metadata)
 * 0x2003e800..0x2003efff  unallocated boundary gap (~2 KiB)
 * 0x2003f000..0x2003ffff  idle/network task stack (4 KiB)
 * pbuf pools, TCP PCB pools and CYW43 globals remain in kernel SRAM.
 * The ELF/layout audits must validate this contract. */
#define MEM_SIZE (10 * 1024)
#define LWIP_RAM_HEAP_POINTER ((void *)0x2003C000u)
#define PBUF_POOL_SIZE 8
/* Lightweight counters for heap and pool utilization/failures. */
#define LWIP_STATS 1
#define MEM_STATS 1
#define MEMP_STATS 1
#define PBUF_POOL_BUFSIZE 512
#define MEMP_NUM_TCP_PCB 5
#define MEMP_NUM_TCP_PCB_LISTEN 5 /* Telnet + user TCP; HTTP will require another */
#define MEMP_NUM_UDP_PCB 4 /* DHCP + device service + user UDP + mDNS */
#define LWIP_MDNS_RESPONDER 1
/* mDNS probing/announcements need timeout entries beyond lwIP core timers.
 * Keep four extra entries for transient mDNS timers in both STA and AP. */
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

