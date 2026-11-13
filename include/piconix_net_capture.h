/* パケット観測とICMP Echoに使う共通データ構造． */
/* IPv4パケット観測とICMP Echo向けの教育用データ形式． */
#ifndef PICONIX_NET_CAPTURE_H
#define PICONIX_NET_CAPTURE_H
#include <stdint.h>
/* 1回のreadで完全な1レコードを取得する．数値はホスト順，IPv4オクテットは表示順に並べる． */
typedef struct {
    uint8_t src[4];     /* 送信元IPv4アドレス． */
    uint8_t dst[4];     /* 宛先IPv4アドレス． */
    uint16_t sport;     /* 送信元ポート． */
    uint16_t dport;     /* 宛先ポート． */
    uint16_t ip_len;    /* IPパケットの長さ． */
    uint16_t data_len;  /* データ部分の長さ． */
    uint8_t protocol;   /* IPプロトコル番号． */
    uint8_t flags;      /* 観測したパケットのフラグ． */
    uint8_t direction;  /* 送受信の方向． */
    uint8_t sample_len; /* sample[]内の有効バイト数． */
    uint8_t sample[16]; /* パケット先頭部分のサンプル． */
} piconix_packet_t;
/* writeで要求しreadで応答を読む．同時に保持するEcho要求は一つ． */
typedef struct {
    uint8_t address[4];    /* Echo要求の宛先IPv4アドレス． */
    uint16_t id;           /* ICMP Echoの識別子． */
    uint16_t sequence;     /* ICMP Echoのシーケンス番号． */
    uint32_t milliseconds; /* 応答時間（ms）．応答受信時にカーネルが設定． */
} piconix_echo_t;
#endif
