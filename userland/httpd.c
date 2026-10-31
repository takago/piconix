/* Minimal HTTP/1.0 server using PicoNIX user-space BSD-style sockets. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include <stdint.h>

static int write_all(int fd,const char *buf,unsigned n) {
    while(n) {
        int r=send(fd,buf,n,0);
        if(r==OS_EAGAIN || r==0) { msleep(2); continue; }
        if(r<0) return -1;
        buf+=r; n-=(unsigned)r;
    }
    return 0;
}
/* Consume the complete HTTP header before replying. Browsers commonly send
 * more header bytes than curl, and closing with unread data may cause RST. */
static int read_request(int fd, char *line, unsigned capacity) {
    char chunk[128];
    unsigned used = 0, matched = 0, elapsed = 0;
    const char terminator[] = "\r\n\r\n";
    while (elapsed < 3000) {
        int n = recv(fd, chunk, sizeof(chunk), 0);
        if (n == OS_EAGAIN || n == 0) { msleep(5); elapsed += 5; continue; }
        if (n < 0) return -1;
        for (int i = 0; i < n; ++i) {
            char c = chunk[i];
            if (used + 1 < capacity) line[used++] = c;
            if (c == terminator[matched]) ++matched;
            else matched = (c == '\r') ? 1u : 0u;
            if (matched == 4) {
                line[used] = 0;
                return 0;
            }
        }
    }
    return -1;
}
static void reply(int fd) {
    char request[256];
    if (read_request(fd, request, sizeof(request)) < 0) return;
    if (strncmp(request, "GET / ", 6)) {
        static const char notfound[] = "HTTP/1.0 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 10\r\nConnection: close\r\n\r\nNot Found\n";
        write_all(fd, notfound, sizeof(notfound)-1);
        return;
    }
    int file = open("/etc/index.html", O_RDONLY);
    if (file < 0) {
        static const char missing[] = "HTTP/1.0 404 Not Found\r\nConnection: close\r\nContent-Length: 19\r\n\r\nMissing index.html\n";
        write_all(fd, missing, sizeof(missing)-1);
        return;
    }
    /* HTTP/1.0 close-delimited body, with explicit close and content type. */
    static const char header[] = "HTTP/1.0 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n";
    if (write_all(fd, header, sizeof(header)-1) == 0) {
        char buf[128];
        int n;
        while ((n = read(file, buf, sizeof(buf))) > 0)
            if (write_all(fd, buf, (unsigned)n) < 0) break;
    }
    close(file);
}

int main(void) {
    int server=socket(AF_INET,SOCK_STREAM,0);
    if(server<0) { puts("httpd: socket failed"); return 1; }
    struct sockaddr_in addr={0};
    addr.sin_family=AF_INET;
    addr.sin_port=htons(80);
    if(bind(server,(const struct sockaddr *)&addr,sizeof(addr))<0 || listen(server,1)<0) {
        puts("httpd: cannot listen on port 80"); close(server); return 1;
    }
    puts("httpd: listening on TCP 80");
    for(;;) {
        int client=accept(server,0,0);
        if(client==OS_EAGAIN) { msleep(10); continue; }
        if(client<0) break;
        reply(client);
        close(client);
    }
    close(server);
    return 0;
}
