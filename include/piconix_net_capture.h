/* パケット観測とICMP Echoに使う共通データ構造． */
/* IPv4パケット観測とICMP Echo向けの教育用データ形式． */
#ifndef PICONIX_NET_CAPTURE_H
#define PICONIX_NET_CAPTURE_H
#include <stdint.h>
/* 1回のreadで完全な1レコードを取得する．数値はホスト順，IPv4オクテットは表示順に並べる． */
typedef struct {
    uint8_t src[4], dst[4];
    uint16_t sport, dport, ip_len, data_len;
    uint8_t protocol, flags, direction, sample_len;
    uint8_t sample[16];
} piconix_packet_t;
/* writeで要求しreadで応答を読む．同時に保持するEcho要求は一つ． */
typedef struct {
    uint8_t address[4];
    uint16_t id, sequence;
    uint32_t milliseconds; /* 応答受信時にカーネルが埋める． */
} piconix_echo_t;
#endif
