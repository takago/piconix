/* Minimal lwIP options for PicoNIX AP bring-up. */
#pragma once
#define NO_SYS 1
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0
#define LWIP_TCP 1
#define LWIP_UDP 1
#define LWIP_DHCP 0
/* Permit DHCP clients to send IPv4 packets from 0.0.0.0 before leasing. */
#define LWIP_IP_ACCEPT_UDP_PORT(port) ((port) == PP_NTOHS(67))
#define LWIP_DNS 0
#define MEM_ALIGNMENT 4
/* Place the lwIP dynamic heap in the released fifth task slot.
 * RP2040 main SRAM: slot 5 = 0x20038000..0x2003ffff.
 * This relocates ONLY lwIP's MEM_SIZE heap, not all CYW43/lwIP globals.
 * The linker/slot-count audit must remain enabled. */
#define MEM_SIZE (4 * 1024)
#define LWIP_RAM_HEAP_POINTER ((void *)0x20038000u)
#define PBUF_POOL_SIZE 4
#define PBUF_POOL_BUFSIZE 512
#define MEMP_NUM_TCP_PCB 2
#define MEMP_NUM_TCP_PCB_LISTEN 2 /* Telnet + user TCP; HTTP will require another */
#define MEMP_NUM_UDP_PCB 3 /* DHCP + device service + user UDP */
#define TCP_MSS 536
#define TCP_WND (2 * TCP_MSS)
#define TCP_SND_BUF (2 * TCP_MSS)
#define TCP_SND_QUEUELEN 8
#define LWIP_NETIF_STATUS_CALLBACK 0
#define LWIP_NETIF_LINK_CALLBACK 0
#define LWIP_PROVIDE_ERRNO 1

