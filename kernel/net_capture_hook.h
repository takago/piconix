/* Included by lwIP through lwipopts.h before pbuf/netif definitions. */
struct pbuf;
struct netif;
int piconix_packet_input(struct pbuf *p, struct netif *n);
