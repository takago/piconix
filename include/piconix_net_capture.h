/* PicoNIX educational IPv4 capture and ICMP Echo device records. */
#ifndef PICONIX_NET_CAPTURE_H
#define PICONIX_NET_CAPTURE_H
#include <stdint.h>
/* One complete record per read; host-order numeric fields, IPv4 octets in order. */
typedef struct {
    uint8_t src[4], dst[4];
    uint16_t sport, dport, ip_len, data_len;
    uint8_t protocol, flags, direction, sample_len;
    uint8_t sample[16];
} piconix_packet_t;
/* write() request, read() reply. One outstanding Echo per system. */
typedef struct {
    uint8_t address[4];
    uint16_t id, sequence;
    uint32_t milliseconds; /* filled by the kernel on reply */
} piconix_echo_t;
#endif
