/* パケット観測フックの接続に必要な宣言を集める． */
/* lwipopts.hからpbuf/netifの定義より前に読み込まれる． */
struct pbuf;
struct netif;
int piconix_packet_input(struct pbuf *p, struct netif *n);
