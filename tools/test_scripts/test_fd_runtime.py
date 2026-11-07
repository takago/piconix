#!/usr/bin/env python3
"""Host-execute the kernel's actual pipe/FD helper functions (no Pico SDK)."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'kernel/kernel.c').read_text()

def extract(signature):
    start = source.index(signature)
    pos = source.index('{', start)
    depth = 0
    for i in range(pos, len(source)):
        if source[i] == '{':
            depth += 1
        elif source[i] == '}':
            depth -= 1
            if depth == 0:
                return source[start:i + 1]
    raise AssertionError(f'Unterminated {signature}')

parts = [extract(signature) for signature in (
    'static int pipe_end(',
    'static pipe_t *pipe_for_dev(',
    'static int pipe_is_writer(',
    'static void pipe_ref(',
    'static void fd_set(',
    'static int udp_fd_slot(',
    'static int tcp_fd_slot(',
    'static int tcp_other_fd(',
    'static int fd_is_socket(',
    'static void fd_release(',
    'static void fd_inherit(',
    'static void fd_copy(',
    'static int free_fd(',
)]
assert 'for(int j=0;j<3;j++) fd_inherit(t,j,&tasks[current],j);' in source
spawn = source.split('case SYS_TTY_SPAWN:', 1)[1].split('case SYS_SPAWN_TMP:', 1)[0]
assert spawn.index('fd_release(child,j);') < spawn.index('fd_set(child,j,tty ? DEV_UART1 : DEV_CONSOLE')
assert 'int fd=free_fd();' in source.split('case SYS_SOCKET:', 1)[1].split('case SYS_LISTEN_TCP:', 1)[0]
assert 'int fd=free_fd();' in source.split('case SYS_ACCEPT_TCP:', 1)[1].split('case SYS_CONNECT_TCP:', 1)[0]

prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define OS_FD_MAX 16
#define PIPE_COUNT 2
#define PIPE_BYTES 256
#define PIPE_DEV_BASE 268
#define PIPE_READ_DEV(i) (PIPE_DEV_BASE+(i)*2)
#define PIPE_WRITE_DEV(i) (PIPE_DEV_BASE+(i)*2+1)
#define DEV_SOCKET_UDP 400
#define DEV_SOCKET_TCP 401
#define DEV_SOCKET_TCP_ACCEPTED 402
#define DEV_CONSOLE 0
#define DEV_UART1 30
#define DEV_NET_PACKET 500
#define DEV_NET_ICMP 501
#define TCP_USER_SLOTS 4
#define UDP_USER_SLOTS 3
#define NO_TRACE __attribute__((no_instrument_function))
typedef struct {uint8_t data[PIPE_BYTES];uint16_t head,used;uint8_t readers,writers;} pipe_t;
typedef struct {int fd[OS_FD_MAX];uint32_t fd_pos[OS_FD_MAX];int fd_task_pid[OS_FD_MAX];uint8_t fd_flags[OS_FD_MAX];int pid;} task_t;
static pipe_t pipes[PIPE_COUNT];
static task_t tasks[3];
static int current;
static int udp_owner_pid[UDP_USER_SLOTS];
static int tcp_owner_pid[TCP_USER_SLOTS];
static int udp_closes, tcp_closes, listener_closes, client_closes;
static unsigned icmp_fd_refs;
static void net_packet_reader(int delta) {(void)delta;}
static void net_echo_close(void) {}
static void net_socket_udp_close(int id) {(void)id;udp_closes++;}
static void net_socket_tcp_close(int id) {(void)id; tcp_closes++;}
static void net_socket_tcp_close_listener(int id) {(void)id;listener_closes++;}
static void net_socket_tcp_close_client(int id) {(void)id;client_closes++;}
'''
main = r'''
static void reset(void) {
    for(int i=0;i<3;i++) {
        tasks[i].pid=100+i;
        for(int j=0;j<OS_FD_MAX;j++) fd_set(&tasks[i],j,-1,0,0,0);
    }
    for(int i=0;i<PIPE_COUNT;i++) pipes[i]=(pipe_t){0};
    for(int i=0;i<TCP_USER_SLOTS;i++) tcp_owner_pid[i]=0;
    for(int i=0;i<UDP_USER_SLOTS;i++) udp_owner_pid[i]=0;
    udp_closes=tcp_closes=listener_closes=client_closes=0;
    current=0;
}
int main(void) {
    reset();
    assert(free_fd()==3);
    fd_set(&tasks[0],3,DEV_UART1,40,4,13);
    assert(free_fd()==4);
    fd_copy(&tasks[0],4,3);
    assert(tasks[0].fd[4]==DEV_UART1 && tasks[0].fd_pos[4]==40);
    assert(tasks[0].fd_flags[4]==4 && tasks[0].fd_task_pid[4]==13);
    fd_release(&tasks[0],3);
    assert(tasks[0].fd[3]==-1 && tasks[0].fd_pos[3]==0 && tasks[0].fd_flags[3]==0);
    assert(tasks[0].fd[4]==DEV_UART1);
    assert(free_fd()==3);

    /* A shell-to-child pipeline copy increments exactly once. */
    reset();
    pipes[0].readers=1; pipes[0].writers=1;
    pipes[0].used=7;
    fd_set(&tasks[0],0,PIPE_READ_DEV(0),9,1,22);
    fd_inherit(&tasks[1],0,&tasks[0],0);
    assert(pipes[0].readers==2 && pipes[0].writers==1);
    assert(tasks[1].fd_pos[0]==9 && tasks[1].fd_task_pid[0]==22);
    fd_release(&tasks[1],0); /* getty remapping stdin */
    fd_set(&tasks[1],0,DEV_UART1,0,0,0);
    assert(pipes[0].readers==1 && pipes[0].used==7);
    fd_release(&tasks[0],0);
    assert(pipes[0].readers==0 && pipes[0].writers==1);
    fd_set(&tasks[0],1,PIPE_WRITE_DEV(0),0,1,0);
    fd_release(&tasks[0],1);
    assert(pipes[0].readers==0 && pipes[0].writers==0 && pipes[0].used==0);

    /* dup2-style replacement does not destroy the remaining pipe endpoint. */
    reset();
    pipes[1].writers=1; pipes[1].used=33;
    fd_set(&tasks[0],1,PIPE_WRITE_DEV(1),0,1,0);
    fd_copy(&tasks[0],5,1);
    assert(pipes[1].writers==2);
    fd_release(&tasks[0],1);
    assert(pipes[1].writers==1 && pipes[1].used==33);
    fd_release(&tasks[0],5);
    assert(pipes[1].writers==0 && pipes[1].used==0);

    /* Closed socket endpoints do not leak the owning slot. */
    reset();
    udp_owner_pid[0]=tasks[0].pid;
    fd_set(&tasks[0],4,DEV_SOCKET_UDP,0,2,0);
    assert(fd_is_socket(DEV_SOCKET_UDP));
    fd_release(&tasks[0],4);
    assert(udp_closes==1 && udp_owner_pid[0]==0);
    tcp_owner_pid[0]=tasks[0].pid;
    fd_set(&tasks[0],3,DEV_SOCKET_TCP,0,2,0);
    fd_set(&tasks[0],5,DEV_SOCKET_TCP_ACCEPTED,0,2,0);
    fd_release(&tasks[0],5);
    assert(client_closes==1 && listener_closes==0 && tcp_closes==0);
    assert(tcp_owner_pid[0]==tasks[0].pid);
    fd_release(&tasks[0],3);
    assert(listener_closes==1 && tcp_closes==1 && tcp_owner_pid[0]==0);
    puts("PASS: real FD helpers: inheritance, pipes, dup, release, sockets");
    return 0;
}
'''
with tempfile.TemporaryDirectory() as work:
    file = Path(work) / 'fd_test.c'
    output = Path(work) / 'fd_test'
    file.write_text(prefix + '\n'.join(parts) + main)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-o', str(output), str(file)], check=True)
    subprocess.run([str(output)], check=True)
