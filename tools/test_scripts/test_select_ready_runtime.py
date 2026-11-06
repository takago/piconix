#!/usr/bin/env python3
"""Run the actual kernel select_ready/wake_selects helpers with deterministic FD mocks."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
src=(root/'kernel/kernel.c').read_text()
start=src.index('static int select_ready(task_t *t')
end=src.index('/* 子の終了を待っていた親を起こす．',start)
code=src[start:end]
header=r'''
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#define NO_TRACE __attribute__((no_instrument_function))
#define OS_MAX_TASKS 2
#define OS_FD_MAX 16
#define TCP_USER_SLOTS 4
#define DEV_SOCKET_UDP 400
#define DEV_SOCKET_TCP 401
#define DEV_SOCKET_TCP_ACCEPTED 402
#define DEV_CONSOLE 100
#define DEV_UART1 101
#define DEV_WIFI_TTY 102
#define PIPE_DEV_BASE 268
#define PIPE_BYTES 256
#define BLOCKED 4
#define READY 1
#define WAIT_SELECT (-6)
#define OS_EINVAL (-22)
#define OS_EBADF (-9)
#define TRACE_SVC_EXIT 2
#define SYS_SELECT 40
typedef struct {uint16_t read_mask,write_mask,ready_read,ready_write;int32_t timeout_ms;} piconix_select_request_t;
typedef struct {unsigned used,readers,writers;} pipe_t;
typedef struct {
 int fd[OS_FD_MAX],pid,program_id,wait_dev,wait_frame_ready,state;
 uint32_t fd_pos[OS_FD_MAX],wake;
 void *wait_buf;size_t wait_size;
 int result;
} task_t;
static task_t tasks[OS_MAX_TASKS];
static int tcp_owner_pid[TCP_USER_SLOTS],udp_owner_pid,accept_pending,rx_bytes,udp_pending,tty_pending,poll_busy,can_write;
static uint32_t ticks;
static pipe_t pipe_storage;
static int pipe_end(int d) {return d==PIPE_DEV_BASE || d==PIPE_DEV_BASE+1;}
static pipe_t *pipe_for_dev(int d) {(void)d;return &pipe_storage;}
static int pipe_is_writer(int d) {return d==PIPE_DEV_BASE+1;}
static int net_poll_in_progress(void) {return poll_busy;}
static int net_socket_udp_read_ready(void) {return udp_pending;}
static int net_socket_tcp_listening(int id) {(void)id;return 1;}
static int net_socket_tcp_accept_peek(int id) {(void)id;return accept_pending;}
static int net_socket_tcp_recv_ready(int id) {(void)id;return rx_bytes>0;}
static int net_socket_tcp_write_ready(int id) {(void)id;return can_write;}
static int tty_rx_ready(int tty) {(void)tty;return tty_pending;}
static void trace_record(int kind,uint8_t pid,uint32_t op,uint32_t result,uint8_t program,int dev,void *data,int length) {
 (void)kind;(void)pid;(void)op;(void)result;(void)program;(void)dev;(void)data;(void)length;
}
static void task_unblock(task_t *t,int n) {t->result=n;t->state=READY;t->wait_frame_ready=0;}
'''
checks=r'''
int main(void) {
 task_t *t=&tasks[0];t->pid=7;
 for(int i=0;i<OS_FD_MAX;i++)t->fd[i]=-1;
 t->fd[3]=DEV_SOCKET_TCP; t->fd_pos[3]=0;tcp_owner_pid[0]=7;
 t->fd[4]=DEV_SOCKET_TCP_ACCEPTED;t->fd_pos[4]=1;tcp_owner_pid[1]=7;
 t->fd[5]=DEV_SOCKET_UDP;udp_owner_pid=7;
 t->fd[0]=DEV_CONSOLE;
 t->fd[6]=PIPE_DEV_BASE;pipe_storage.writers=1;
 piconix_select_request_t req={.read_mask=(1u<<3)|(1u<<4),.timeout_ms=100};
 assert(select_ready(t,7,&req)==0);
 accept_pending=1;rx_bytes=5;
 assert(select_ready(t,7,&req)==2);
 assert(req.ready_read==((1u<<3)|(1u<<4)));
 assert(accept_pending==1); /* peek must not consume the TCP accept */
 req.read_mask=(1u<<5)|(1u<<0)|(1u<<6);
 udp_pending=1;tty_pending=1;pipe_storage.used=1;
 assert(select_ready(t,7,&req)==3);
 req.read_mask=(1u<<2);assert(select_ready(t,7,&req)==OS_EBADF);
 req.read_mask=(1u<<6);assert(select_ready(t,3,&req)==OS_EINVAL);
 req.read_mask=(1u<<3);accept_pending=0;t->state=BLOCKED;t->wait_dev=WAIT_SELECT;
 t->wait_frame_ready=1;t->wait_buf=&req;t->wait_size=7;t->wake=25;
 poll_busy=1;ticks=30;wake_selects();assert(t->state==BLOCKED);
 poll_busy=0;ticks=24;wake_selects();assert(t->state==BLOCKED);
 ticks=25;wake_selects();assert(t->state==READY&&t->result==0);
 t->state=BLOCKED;t->wait_frame_ready=1;req.timeout_ms=-1;ticks=50;
 wake_selects();assert(t->state==BLOCKED);
 accept_pending=1;wake_selects();assert(t->state==READY&&t->result==1);
 return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
 c=Path(d)/'test.c'; exe=Path(d)/'test';c.write_text(header+code+checks)
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-function',str(c),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS: kernel select readiness and wake on TCP, UDP, TTY, pipe, timeout, poll lock')
