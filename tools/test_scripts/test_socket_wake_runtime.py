#!/usr/bin/env python3
"""Execute the actual socket wake helper against deterministic host-side mocks."""
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[2]
src=(root/'kernel/kernel.c').read_text()
start=src.index('static void wake_sockets(void) {')
end=src.index('/* 選択したFDの読書き可能性を調べる．',start)
wake=src[start:end]
header=r'''
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#define OS_MAX_TASKS 2
#define OS_FD_MAX 16
#define TCP_USER_SLOTS 2
#define WAIT_TCP_ACCEPT (-3)
#define WAIT_TCP_READ (-4)
#define WAIT_UDP_READ (-5)
#define DEV_SOCKET_TCP 401
#define DEV_SOCKET_TCP_ACCEPTED 402
#define DEV_SOCKET_UDP 400
#define OS_O_RDWR 2
#define OS_EBADF (-9)
#define OS_EMFILE (-24)
#define OS_EIO (-5)
#define OS_EINVAL (-22)
#define BLOCKED 4
#define READY 1
#define SYS_ACCEPT_TCP 17
#define SYS_RECVFROM 18
#define SYS_READ 19
#define TRACE_SVC_EXIT 2
#define OS_FD_MAX 16
typedef struct { const void *data; uint32_t length, peer_ip; uint16_t peer_port, spare; } piconix_udp_packet_t;
typedef struct {
 int fd[OS_FD_MAX], wait_dev, wait_fd, state, pid, program_id;
 uint32_t fd_pos[OS_FD_MAX];
 int wait_frame_ready;
 void *wait_buf; size_t wait_size;
 int result;
} task_t;
static task_t tasks[OS_MAX_TASKS];
static int tcp_owner_pid[TCP_USER_SLOTS], udp_owner_pid;
static int net_busy, accept_ready, tcp_status, tcp_bytes, udp_bytes;
static int net_poll_in_progress(void) { return net_busy; }
static int net_socket_tcp_listening(int id) { return id==0; }
static int net_socket_tcp_accept_ready(int id) { (void)id; if(!accept_ready)return 0; accept_ready=0; return 1; }
static int net_socket_tcp_status(int id) { (void)id; return tcp_status; }
static int net_socket_tcp_read(int id, void *dst, unsigned size) { (void)id; (void)size; if(tcp_bytes>0) ((char *)dst)[0]='X'; return tcp_bytes; }
static int net_socket_udp_recv(void *buf, unsigned capacity, uint32_t *ip, uint16_t *port) {
 (void)buf; (void)capacity;
 if(udp_bytes>=0) { *ip=1; *port=2; }
 return udp_bytes;
}
static void fd_set(task_t *t,int fd,int dev,uint32_t pos,int flags,int pid) {
 (void)flags;(void)pid;t->fd[fd]=dev;t->fd_pos[fd]=pos;
}
static void trace_record(int kind,uint8_t pid,uint32_t op,uint32_t result,uint8_t program,int dev,void *data,int length) {
 (void)kind;(void)pid;(void)op;(void)result;(void)program;(void)dev;(void)data;(void)length;
}
static void task_unblock(task_t *t,int rc) { t->state=READY; t->result=rc; t->wait_frame_ready=0; }
'''
checks=r'''
int main(void) {
 task_t *t=&tasks[0];
 for(int j=0;j<OS_FD_MAX;j++)t->fd[j]=-1;
 t->fd[3]=DEV_SOCKET_TCP;t->fd_pos[3]=0;
 t->state=BLOCKED;t->wait_frame_ready=1;t->wait_dev=WAIT_TCP_ACCEPT;t->wait_fd=3;t->pid=3;
 tcp_owner_pid[0]=3;
 net_busy=1;accept_ready=1;wake_sockets();assert(t->state==BLOCKED);
 net_busy=0;accept_ready=0;wake_sockets();assert(t->state==BLOCKED);
 accept_ready=1;wake_sockets();assert(t->state==READY&&t->result==4&&t->fd[4]==DEV_SOCKET_TCP_ACCEPTED);
 char data[4]={0}; t->state=BLOCKED;t->wait_frame_ready=1;t->wait_dev=WAIT_TCP_READ;t->wait_fd=4;t->wait_buf=data;t->wait_size=4;
 tcp_status=1;tcp_bytes=0;wake_sockets();assert(t->state==BLOCKED);
 tcp_bytes=1;wake_sockets();assert(t->state==READY&&t->result==1&&data[0]=='X');
 t->state=BLOCKED;t->wait_frame_ready=1;tcp_status=-2;tcp_bytes=-2;wake_sockets();assert(t->state==READY&&t->result==0);
 t->fd[5]=DEV_SOCKET_UDP;t->state=BLOCKED;t->wait_frame_ready=1;t->wait_dev=WAIT_UDP_READ;t->wait_fd=5;
 piconix_udp_packet_t pkt={0};t->wait_buf=&pkt;udp_owner_pid=3;
 udp_bytes=-3;wake_sockets();assert(t->state==BLOCKED);
 udp_bytes=0;wake_sockets();assert(t->state==READY&&t->result==0&&pkt.peer_ip==1);
 return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
 c=Path(d)/'mock.c'; ex=Path(d)/'test';c.write_text(header+wake+checks)
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-function',str(c),'-o',str(ex)],check=True)
 subprocess.run([str(ex)],check=True)
print('PASS: actual socket wake helper (accept, TCP read, EOF, UDP, poll safety)')
