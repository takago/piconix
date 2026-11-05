/* Pico W: send one UDP datagram to an echo server on the PC and read the reply. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include <stdint.h>

static int ipv4(const char *s, uint32_t *result) {
    unsigned char *p = (unsigned char *)result;
    for (int i = 0; i < 4; ++i) {
        unsigned value = 0, digits = 0;
        while (*s >= '0' && *s <= '9') {
            value = value * 10u + (unsigned)(*s++ - '0');
            if (value > 255u || ++digits > 3) return -1;
        }
        if (!digits || (i < 3 ? *s++ != '.' : *s != '\0')) return -1;
        p[i] = (unsigned char)value;
    }
    return 0;
}

static int port_number(const char *s) {
    unsigned n = 0;
    if (!*s) return -1;
    for (; *s; ++s) {
        if (*s < '0' || *s > '9') return -1;
        n = n * 10u + (unsigned)(*s - '0');
        if (n > 65535u) return -1;
    }
    return n ? (int)n : -1;
}

int main(int argc, char **argv) {
    uint32_t address = 0;
    int port = argc == 3 ? port_number(argv[2]) : -1;
    if (argc != 3 || port < 0 || ipv4(argv[1], &address) < 0) {
        puts("usage: udp_once IPv4 PORT");
        return 1;
    }
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { printf("socket: %d\n", fd); return 1; }
    /* 受信期限を維持するため，ソケット受信は非ブロッキング． */
    fcntl(fd, F_SETFL, O_NONBLOCK);
    struct sockaddr_in peer = {0};
    peer.sin_family = AF_INET;
    peer.sin_port = htons((uint16_t)port);
    peer.sin_addr.s_addr = address;
    static const char msg[] = "hello from PicoNIX";
    int sent = sendto(fd, msg, sizeof(msg)-1u, 0,
                      (const struct sockaddr *)&peer, sizeof(peer));
    if (sent != (int)sizeof(msg)-1) {
        printf("sendto: %d\n", sent); close(fd); return 1;
    }
    char reply[80];
    for (int i = 0; i < 200; ++i) {
        int n = recvfrom(fd, reply, sizeof(reply), 0, 0, 0);
        if (n >= 0) {
            printf("received %d bytes: ", n);
            if (n > 0) write(1, reply, (unsigned)n);
            puts(""); close(fd); return 0;
        }
        if (n != OS_EAGAIN) {
            printf("recvfrom: %d\n", n); close(fd); return 1;
        }
        msleep(10);
    }
    puts("timeout (check the PC echo server and its firewall)");
    close(fd);
    return 1;
}
