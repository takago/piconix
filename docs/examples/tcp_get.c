/* Pico W: minimal TCP client for an HTTP server on the host PC. */
#include "unistd.h"
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
        puts("usage: tcp_get IPv4 PORT");
        return 1;
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { printf("socket: %d\n", fd); return 1; }
    struct sockaddr_in peer = {0};
    peer.sin_family = AF_INET;
    peer.sin_port = htons((uint16_t)port);
    peer.sin_addr.s_addr = address;
    int r = connect(fd, (const struct sockaddr *)&peer, sizeof(peer));
    if (r < 0) { printf("connect: %d\n", r); close(fd); return 1; }
    int status = 0;
    for (int i = 0; i < 300; ++i) {
        status = tcp_status(fd);
        if (status != 0) break;
        msleep(10);
    }
    if (status != 1) { printf("not connected: %d\n", status); close(fd); return 1; }
    static const char request[] = "GET / HTTP/1.0\r\nHost: piconix-lab\r\n\r\n";
    unsigned off = 0;
    while (off < sizeof(request)-1u) {
        r = send(fd, request + off, sizeof(request)-1u-off, 0);
        if (r == OS_EAGAIN || r == 0) { msleep(10); continue; }
        if (r < 0) { printf("send: %d\n", r); close(fd); return 1; }
        off += (unsigned)r;
    }
    shutdown(fd, SHUT_WR);
    char buf[128];
    int idle = 0;
    for (;;) {
        r = recv(fd, buf, sizeof(buf), 0);
        if (r == 0) break;
        if (r == OS_EAGAIN) {
            if (++idle > 300) { puts("timeout reading response"); close(fd); return 1; }
            msleep(10); continue;
        }
        if (r < 0) { printf("recv: %d\n", r); close(fd); return 1; }
        idle = 0;
        int n = r, used = 0;
        while (used < n) {
            int k = write(1, buf + used, (unsigned)(n-used));
            if (k <= 0) { puts("write failed"); close(fd); return 1; }
            used += k;
        }
    }
    close(fd);
    return 0;
}
