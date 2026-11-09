#!/usr/bin/env python3
"""Execute real kernel process-exit and blocking-I/O helpers without the Pico SDK."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'kernel/kernel.c').read_text()

def extract(signature):
    start = source.index(signature)
    pos = source.index('{', start)
    while ';' in source[start:pos]:
        start = source.index(signature, start + len(signature))
        pos = source.index('{', start)
    level = 0
    for i in range(pos, len(source)):
        if source[i] == '{': level += 1
        elif source[i] == '}':
            level -= 1
            if not level: return source[start:i + 1]
    raise AssertionError(f'Unterminated function {signature}')

functions = '\n'.join(extract(name) for name in (
    'static int pipe_end(',
    'static pipe_t *pipe_for_dev(',
    'static int pipe_transfer(',
    'static int take_exit(',
    'static int task_active(',
    'static int tty_foreground_survivor(',
    'static void finish_task(',
    'static void task_unblock(',
    'static void wake_pipes(',
    'static void wake_waiters(',
    'static void wake_readers(',
))

prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define OS_MAX_TASKS 6
#define TOTAL_SLOTS 7
#define OS_FD_MAX 16
#define OS_TTY_COUNT 3
#define PIPE_COUNT 2
#define PIPE_BYTES 256
#define PIPE_DEV_BASE 268
#define SYS_WAITPID 8
#define SYS_READ 4
#define TRACE_SVC_EXIT 2
#define OS_EIO -5
#define UNUSED 0
#define READY 1
#define SLEEPING 2
#define DEAD 3
#define BLOCKED 4
typedef struct { uint8_t data[PIPE_BYTES]; uint16_t head,used; uint8_t readers,writers; } pipe_t;
typedef struct {
    uint32_t *sp;
    uint8_t state;
    int fd[OS_FD_MAX];
    int pid,parent_pid;
    int wait_dev,wait_fd;
    void *wait_buf;
    size_t wait_size;
    uint8_t wait_frame_ready;
    int wait_pid,*wait_status;
    uint8_t tty,program_id;
} task_t;
static task_t tasks[TOTAL_SLOTS];
static pipe_t pipes[PIPE_COUNT];
static int foreground_group[OS_TTY_COUNT][3];
static int foreground_pid[OS_TTY_COUNT], tty_shell_pid[OS_TTY_COUNT];
static int tty_claim_pid[OS_TTY_COUNT], tty_restore_pid[OS_TTY_COUNT], tty_mode_owner[OS_TTY_COUNT];
static uint8_t tty_canonical[OS_TTY_COUNT], tty_echo[OS_TTY_COUNT], tty_onlcr[OS_TTY_COUNT];
static uint8_t tty_interrupt[OS_TTY_COUNT], tty_saved_interrupt[OS_TTY_COUNT];
static uint8_t tty_last_cr[OS_TTY_COUNT], tty_saved_canon[OS_TTY_COUNT];
static uint8_t tty_saved_echo[OS_TTY_COUNT], tty_saved_onlcr[OS_TTY_COUNT];
static int tty_saved_owner[OS_TTY_COUNT];
static struct { int pid,parent_pid,status; } exited[OS_MAX_TASKS];
static int udp_owner_pid[3], tcp_owner_pid[4];
static int fd_release_count, tty_reset_count, record_count, pend_count;
static int read_data_ready, read_bytes;
static void fd_release(task_t *t,int fd) {if(t->fd[fd]>=0)fd_release_count++;t->fd[fd]=-1;}
static void net_socket_udp_close(int id) {(void)id;assert(0);}
static void net_socket_tcp_close(int id) {(void)id;assert(0);}
#define TCP_USER_SLOTS 4
#define UDP_USER_SLOTS 3
static void tty_mode_reset(int tty) {tty_reset_count++;tty_mode_owner[tty]=0;}
static int tty_owner_pid(int tty) { return tty_claim_pid[tty] ? tty_claim_pid[tty] :
    (foreground_pid[tty] ? foreground_pid[tty] : tty_shell_pid[tty]); }
static int tty_rx_ready(int tty) {(void)tty;return read_data_ready;}
static int tty_rx_read(int tty,void *dst,size_t count) {
    (void)tty;
    assert(read_bytes>=0 && (size_t)read_bytes<=count);
    memset(dst,'X',(size_t)read_bytes);
    return read_bytes;
}
static void pend(void) {pend_count++;}
static void trace_record(uint8_t kind,uint8_t pid,uint32_t num,uint32_t result,
                         uint8_t program_id,int8_t file_id,const void *buf,unsigned size) {
    (void)kind;(void)pid;(void)num;(void)result;(void)program_id;
    (void)file_id;(void)buf;(void)size;
    record_count++;
}
'''

main = r'''
static void reset(void) {
    memset(tasks,0,sizeof tasks);
    memset(pipes,0,sizeof pipes);
    memset(exited,0,sizeof exited);
    memset(foreground_group,0,sizeof foreground_group);
    memset(foreground_pid,0,sizeof foreground_pid);
    memset(tty_shell_pid,0,sizeof tty_shell_pid);
    memset(tty_claim_pid,0,sizeof tty_claim_pid);
    memset(tty_restore_pid,0,sizeof tty_restore_pid);
    memset(tty_mode_owner,0,sizeof tty_mode_owner);
    memset(tcp_owner_pid,0,sizeof tcp_owner_pid);
    fd_release_count=tty_reset_count=record_count=pend_count=0;
    memset(udp_owner_pid,0,sizeof udp_owner_pid);
    read_data_ready=read_bytes=0;
    for(int i=0;i<TOTAL_SLOTS;i++)
        for(int fd=0;fd<OS_FD_MAX;fd++) tasks[i].fd[fd]=-1;
}
int main(void) {
    reset();
    tasks[0].pid=100;tasks[0].state=BLOCKED;tasks[0].tty=1;
    tasks[1].pid=101;tasks[1].parent_pid=100;tasks[1].state=READY;tasks[1].tty=1;
    tasks[1].fd[3]=23;
    tasks[2].pid=102;tasks[2].parent_pid=100;tasks[2].state=READY;tasks[2].tty=1;
    foreground_group[1][0]=101;foreground_group[1][1]=102;
    foreground_pid[1]=101;tty_shell_pid[1]=100;
    finish_task(1,7);
    assert(tasks[1].state==DEAD);
    assert(fd_release_count==1);
    assert(foreground_group[1][0]==0 && foreground_group[1][1]==102);
    assert(foreground_pid[1]==102 && tty_shell_pid[1]==100);
    assert(tty_owner_pid(1)==102);
    assert(exited[0].pid==101 && exited[0].parent_pid==100 && exited[0].status==7);
    finish_task(1,7); assert(fd_release_count==1);
    /* A completion record survives immediate slot reuse. */
    tasks[1].pid=103;tasks[1].state=READY;
    int reaped=-1;
    assert(take_exit(100,101,&reaped)==101 && reaped==7);
    assert(take_exit(100,101,&reaped)==0);
    assert(task_active(&tasks[1]) && !task_active(&tasks[6]));

    /* Exit codes are retained as 8 bits, including forced termination. */
    reset();
    tasks[0].pid=100;tasks[0].state=READY;tasks[0].tty=1;
    tasks[1].pid=101;tasks[1].parent_pid=100;tasks[1].state=READY;tasks[1].tty=1;
    finish_task(1,511);
    int truncated=-1;
    assert(take_exit(100,101,&truncated)==101 && truncated==255);

    /* Last stage exits first: input moves to the preceding live stage. */
    reset();
    tasks[0].pid=100;tasks[0].state=BLOCKED;tasks[0].tty=1;
    for(int i=1;i<=3;i++) {
        tasks[i].pid=100+i;tasks[i].state=READY;tasks[i].tty=1;
        tasks[i].parent_pid=100;
        foreground_group[1][i-1]=100+i;
    }
    tty_shell_pid[1]=100;
    foreground_pid[1]=103;
    finish_task(3,0);
    assert(foreground_pid[1]==102 && tty_owner_pid(1)==102);
    finish_task(2,0);
    assert(foreground_pid[1]==101 && tty_owner_pid(1)==101);
    finish_task(1,0);
    assert(foreground_pid[1]==0 && tty_owner_pid(1)==100);

    /* The first stage saved the original TTY mode and exits early.
     * Do not restore it until the remaining foreground stage has finished. */
    reset();
    tasks[0].pid=100;tasks[0].state=BLOCKED;tasks[0].tty=1;
    tasks[1].pid=101;tasks[1].state=READY;tasks[1].tty=1;
    tasks[2].pid=102;tasks[2].state=READY;tasks[2].tty=1;
    foreground_group[1][0]=101;foreground_group[1][1]=102;
    foreground_pid[1]=102;tty_shell_pid[1]=100;
    tty_restore_pid[1]=101;tty_mode_owner[1]=101;
    tty_canonical[1]=0;tty_echo[1]=0;tty_onlcr[1]=0;
    tty_saved_canon[1]=1;tty_saved_echo[1]=1;tty_saved_onlcr[1]=1;
    finish_task(1,0);
    assert(foreground_pid[1]==102 && tty_restore_pid[1]==102);
    assert(tty_mode_owner[1]==102 && tty_canonical[1]==0 && tty_echo[1]==0);
    finish_task(2,0);
    assert(!tty_restore_pid[1] && !tty_mode_owner[1]);
    assert(tty_canonical[1]==1 && tty_echo[1]==1 && tty_onlcr[1]==1);

    /* A three-stage pipeline transfers restoration more than once. */
    reset();
    tasks[0].pid=100;tasks[0].state=BLOCKED;tasks[0].tty=1;
    for(int i=1;i<=3;i++) {
        tasks[i].pid=100+i;tasks[i].state=READY;tasks[i].tty=1;
        foreground_group[1][i-1]=100+i;
    }
    foreground_pid[1]=103;tty_shell_pid[1]=100;
    tty_restore_pid[1]=101;
    tty_saved_canon[1]=1;tty_canonical[1]=0;
    finish_task(1,0);
    assert(tty_restore_pid[1]==103 && tty_canonical[1]==0);
    finish_task(3,0);
    assert(foreground_pid[1]==102 && tty_restore_pid[1]==102);
    assert(tty_canonical[1]==0);
    finish_task(2,0);
    assert(foreground_pid[1]==0 && tty_restore_pid[1]==0);
    assert(tty_canonical[1]==1);

    reset();
    tasks[0].pid=100; tasks[0].state=BLOCKED;
    tasks[0].wait_dev=-2; tasks[0].wait_pid=101; tasks[0].wait_frame_ready=0;
    int status=-1; uint32_t frame[16]={0};
    tasks[0].sp=frame;tasks[0].wait_status=&status;
    exited[0].pid=101;exited[0].parent_pid=100;exited[0].status=39;
    wake_waiters(); assert(tasks[0].state==BLOCKED && exited[0].pid==101);
    tasks[0].wait_frame_ready=1;
    wake_waiters();
    assert(tasks[0].state==READY && frame[8]==101 && status==39);
    assert(tasks[0].wait_dev==-1 && tasks[0].wait_status==NULL && tasks[0].wait_pid==0);
    assert(!tasks[0].wait_frame_ready && !exited[0].pid && record_count==1);

    reset();
    uint8_t data[3]={1,2,3}; uint32_t read_frame[16]={0};
    tasks[0].pid=100;tasks[0].state=BLOCKED;tasks[0].sp=read_frame;
    tasks[0].wait_dev=PIPE_DEV_BASE;tasks[0].wait_fd=0;
    tasks[0].wait_buf=data;tasks[0].wait_size=3;tasks[0].wait_frame_ready=1;
    pipes[0].writers=1;pipes[0].data[0]='a';pipes[0].used=1;
    wake_pipes();
    assert(tasks[0].state==READY && read_frame[8]==1 && data[0]=='a');
    assert(tasks[0].wait_dev==-1 && tasks[0].wait_buf==NULL && tasks[0].wait_size==0);
    assert(pipes[0].used==0);
    tasks[0].state=BLOCKED;tasks[0].wait_dev=PIPE_DEV_BASE;
    tasks[0].wait_frame_ready=1;tasks[0].wait_buf=data;tasks[0].wait_size=3;
    pipes[0].writers=0;
    wake_pipes(); assert(read_frame[8]==0 && tasks[0].state==READY);

    reset();
    uint32_t tty_frame[16]={0}; char tty_buf[3]={0};
    tasks[0].pid=100;tasks[0].state=BLOCKED;tasks[0].sp=tty_frame;
    tasks[0].fd[4]=30;tasks[0].wait_dev=1;tasks[0].wait_fd=4;
    tasks[0].wait_buf=tty_buf;tasks[0].wait_size=sizeof tty_buf;
    tasks[0].wait_frame_ready=1;
    tty_shell_pid[1]=100;read_data_ready=1;read_bytes=2;
    wake_readers(1);
    assert(tasks[0].state==READY && tty_frame[8]==2 && tty_buf[0]=='X');
    assert(tasks[0].wait_dev==-1 && tasks[0].wait_fd==-1);
    assert(tasks[0].wait_buf==NULL && tasks[0].wait_size==0);
    assert(record_count==1 && pend_count==1);

    /* Uncollected notices for a dead parent must be removed. */
    reset();
    tasks[0].pid=100;tasks[0].state=READY;
    exited[0].pid=101; exited[0].parent_pid=100;
    finish_task(0,0);
    assert(exited[0].pid==0);
    puts("PASS: actual task lifecycle helpers, completion, blocked wakeups, foreground cleanup");
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix='piconix-lifecycle-') as tmp:
    c_path = Path(tmp) / 'test.c'
    exe_path = Path(tmp) / 'test'
    c_path.write_text(prefix + functions + main)
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(c_path),'-o',str(exe_path)],check=True)
    subprocess.run([str(exe_path)],check=True)
