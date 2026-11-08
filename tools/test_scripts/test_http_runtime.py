#!/usr/bin/env python3
"""Compile actual httpd.c with host socket/device doubles and exercise HTTP routes.

Pico SDK is not required; this validates request parsing and response decisions,
not Pico W TCP timing or the hardware drivers.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
src = (ROOT / 'userland/apps/httpd.c').read_text()
# Replace PicoNIX-only includes with host libc and a minimal syscall double.
src = '\n'.join(line for line in src.splitlines()
                if not line.startswith('#include '))
src = src.replace('int main(int argc, char **argv){', 'int pico_main(int argc, char **argv){')

prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define OS_EAGAIN (-11)
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define FB_ENABLE 0x100
#define AF_INET 2
#define SOCK_STREAM 1
struct in_addr { uint32_t s_addr; };
struct sockaddr { uint16_t sa_family; char sa_data[14]; };
struct sockaddr_in { uint16_t sin_family, sin_port; struct in_addr sin_addr; char unused[8]; };
static uint16_t htons(uint16_t v) { return (uint16_t)((v << 8) | (v >> 8)); }
static int stub_send(int,const void *,size_t,int);
static int stub_recv(int,void *,size_t,int);
static int stub_open(const char *,int);
static int stub_close(int);
static int stub_read(int,void *,size_t);
static int stub_write(int,const void *,size_t);
static int stub_ioctl(int,int,int);
static int stub_msleep(unsigned);
static int stub_socket(int,int,int);
static int stub_bind(int,const struct sockaddr *,unsigned);
static int stub_listen(int,int);
static int stub_accept(int,struct sockaddr *,unsigned *);
#define send stub_send
#define recv stub_recv
#define open stub_open
#define close stub_close
#define read stub_read
#define write stub_write
#define ioctl stub_ioctl
#define msleep stub_msleep
#define socket stub_socket
#define bind stub_bind
#define listen stub_listen
#define accept stub_accept
'''

suffix = r'''
static const char *incoming;
static unsigned in_len, in_pos, max_chunk, slept, receives, writes, enabled, opened;
static unsigned send_stall, recv_stall;
static unsigned char fb[8] = {0x3c,0x42,0xa5,0x81,0xa5,0x99,0x42,0x3c};
static char output[8192], written[40];
static unsigned out_len, write_len;
static const char *opened_path;
static int file_offset;
static void reset(const char *data,unsigned len,unsigned chunk) {
    incoming=data;in_len=len;in_pos=0;max_chunk=chunk;
    out_len=slept=receives=writes=opened=enabled=write_len=0;
    send_stall=recv_stall=0;opened_path=NULL;file_offset=0;
    memset(output,0,sizeof(output));memset(written,0,sizeof(written));
}
static void request(const char *data,unsigned chunk) {
    reset(data,(unsigned)strlen(data),chunk);
    handle(4);
}
static void status(int code) {
    char expected[32];sprintf(expected,"HTTP/1.0 %d ",code);
    assert(strncmp(output,expected,strlen(expected))==0);
}
static int stub_recv(int fd,void *dst,size_t cap,int flags) {
    (void)fd;(void)flags;receives++;
    if(recv_stall){recv_stall--;return OS_EAGAIN;}
    if(in_pos==in_len)return 0; /* FIN: do not wait for a timeout. */
    unsigned n=in_len-in_pos;
    if(n>max_chunk)n=max_chunk;
    if(n>cap)n=(unsigned)cap;
    memcpy(dst,incoming+in_pos,n);in_pos+=n;
    return (int)n;
}
static int stub_send(int fd,const void *src,size_t n,int flags) {
    (void)fd;(void)flags;
    if(send_stall){send_stall--;return OS_EAGAIN;}
    assert(out_len+n<sizeof(output));
    memcpy(output+out_len,src,n);out_len+=(unsigned)n;output[out_len]=0;
    return (int)n;
}
static int stub_open(const char *path,int flags) {
    (void)flags;opened++;opened_path=path;file_offset=0;
    if(strncmp(path,"/etc/",5)==0||strncmp(path,"/dev/",5)==0)return 7;
    return -1;
}
static int stub_close(int fd) {(void)fd;return 0;}
static int stub_read(int fd,void *dst,size_t cap) {
    (void)fd;
    const char *s="Test page";
    unsigned n;
    if(strncmp(opened_path,"/etc/",5)==0) {
        unsigned len=(unsigned)strlen(s);
        if(file_offset >= (int)len)return 0;
        n=len-(unsigned)file_offset;
        if(n>cap)n=(unsigned)cap;
        memcpy(dst,s+file_offset,n);file_offset+=(int)n;
        return (int)n;
    }
    if(strcmp(opened_path,"/dev/fb")==0){
        if(cap<8)return -1;
        memcpy(dst,fb,8);return 8;
    }
    if(cap<2)return -1;
    memcpy(dst,"50",2);return 2;
}
static int stub_write(int fd,const void *p,size_t n) {
    (void)fd;writes++;
    assert(n<=sizeof(written));memcpy(written,p,n);write_len=(unsigned)n;
    if(strcmp(opened_path,"/dev/fb")==0) {
        assert(n==8);memcpy(fb,p,8);
    }
    return (int)n;
}
static int stub_ioctl(int fd,int cmd,int value) {
    (void)fd;(void)value;assert(cmd==FB_ENABLE);enabled++;return 0;
}
static int stub_msleep(unsigned t) {slept+=t;return 0;}
static int stub_socket(int x,int y,int z){(void)x;(void)y;(void)z;return -1;}
static int stub_bind(int x,const struct sockaddr *y,unsigned z){(void)x;(void)y;(void)z;return -1;}
static int stub_listen(int x,int y){(void)x;(void)y;return -1;}
static int stub_accept(int x,struct sockaddr *y,unsigned *z){(void)x;(void)y;(void)z;return -1;}
int main(void) {
    request("GET / HTTP/1.1\r\nHost: local\r\n\r\n",2);
    status(200);assert(strstr(output,"Test page")!=NULL);
    assert(strcmp(opened_path,"/etc/index.html")==0);

    request("GET /api/fb HTTP/1.0\r\n\r\n",256);
    status(200);assert(strstr(output,"3C42A581A599423C\n")!=NULL);

    request("POST /api/fb HTTP/1.1\r\nContent-Length: 16\r\n\r\n0001020304050607",3);
    status(200);assert(writes==1&&enabled==1);
    assert(strstr(output,"0001020304050607\n")!=NULL);

    request("POST /api/pwm/a HTTP/1.0\r\ncontent-length: 2\r\n\r\n50",1);
    status(200);assert(writes==1&&write_len==2);
    assert(!memcmp(written,"50",2));

    request("GET /pwm.html HTTP/1.0\r\n\r\n",256);
    status(200);assert(strcmp(opened_path,"/etc/pwm.html")==0);

    request("POST /api/tone HTTP/1.1\r\nContent-Length: 3\r\n\r\n440",256);
    status(200);assert(writes==3);
    assert(strcmp(opened_path,"/dev/pwm/1/a")==0 && !memcmp(written,"50",2));

    request("POST /api/tone HTTP/1.1\r\nContent-Length: 4\r\n\r\n5001",256);
    status(400);assert(writes==0 && opened==0);

    request("POST /api/pwm/a HTTP/1.0\r\nContent-Length: 2  \t\r\n\r\n50",256);
    status(200);assert(writes==1);

    request("POST /api/pwm/a HTTP/1.0\r\nContent-Length: 2\r\nContent-Length: 2\r\n\r\n50",256);
    status(400);assert(opened==0);
    request("POST /api/pwm/a HTTP/1.0\r\nContent-Length: 1oops\r\n\r\n5",256);
    status(400);assert(opened==0);
    request("POST /api/pwm/a HTTP/1.0\r\nContent-Length: 999999999999999999\r\n\r\n",256);
    status(413);assert(opened==0);
    request("POST /api/pwm/a HTTP/1.0\r\n\r\n50",256);
    status(400);assert(opened==0);
    request("POST /api/pwm/a HTTP/1.0\r\nContent-Length: 2\r\n\r\n5",256);
    status(400);assert(slept==0&&opened==0); /* TCP EOF is not EAGAIN */
    request("GET / HTTP/1.1\r\nHost: unfinished",256);
    status(400);assert(slept==0&&opened==0);
    request("PATCH / HTTP/1.1\r\n\r\n",256);
    status(405);assert(opened==0);
    request("GET /missing HTTP/1.0\r\n\r\n",256);
    status(404);
    /* EAGAIN is still retryable for both receive and send. */
    reset("GET / HTTP/1.1\r\n\r\n",18,256);
    recv_stall=2;send_stall=1;handle(4);
    status(200);assert(slept==12);
    puts("PASS: actual HTTP parser, routes, length validation, TCP EOF and EAGAIN");
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix='piconix-http-') as t:
    source = Path(t) / 'http_test.c'
    exe = Path(t) / 'http_test'
    source.write_text(prefix + src + suffix)
    subprocess.run(['cc', '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror',
                    str(source), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
