#!/usr/bin/env python3
"""Compile and execute the actual PTY byte/TTY queue helpers without Pico SDK."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
s = (root / 'kernel/kernel.c').read_text()
devs = (root / 'include/device_names.h').read_text()

def extract(signature):
    start = s.index(signature)
    begin = s.index('{', start)
    while ';' in s[start:begin]:
        start = s.index(signature, start+1)
        begin = s.index('{', start)
    depth = 0
    for pos in range(begin, len(s)):
        if s[pos] == '{':
            depth += 1
        elif s[pos] == '}':
            depth -= 1
            if depth == 0:
                return s[start:pos + 1]
    raise AssertionError(signature)

for name in ('/dev/ptmx', '/dev/pts/0', '/dev/pts/1'):
    assert name in devs
assert 'if(pty_slave_dev(t->fd[0])) t->tty=pty_tty(pty_slot(t->fd[0]));' in s
assert 'int tty=tasks[i].tty;' in s[s.index('static int foreground_child('):s.index('static int tty_owner_pid(')]
assert 'wake_readers(PTY_TTY);' in s
assert 'wake_ptys();' in s
assert 'pty_ref(dev,-1);' in s
assert 'pty_ref(src->fd[from],1);' in s
assert 'pty_master_dev(t->fd[a]) && !writing' in s
assert 'pty_slave_dev(t->fd[a]) && writing' in s

prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#define NO_TRACE
#define OS_EIO (-5)
#define OS_EAGAIN (-11)
#define PTY_TTY 3
#define PTY_TTY1 5
#define PTY_COUNT 2
#define CDC_VCONSOLE_TTY 4
#define PTY_OUT_BYTES 1024u
#define VT_COLS 80u
#define VT_ROWS 24u
#define OS_EINVAL (-22)
#define OS_EBADF (-9)
#define TTY_LINES 8u
#define OS_TTY_COUNT 6
#define DEV_PTY_MASTER 274
#define DEV_PTY_SLAVE 275
#define DEV_PTY_MASTER1 281
#define DEV_PTY_SLAVE1 282
#define DEV_PTY_SCREEN 276
#define DEV_PTY_KEY 277
#define DEV_PTY_SCREEN1 283
#define DEV_PTY_KEY1 284
#define CONSOLE_TX_CAP 512u
static unsigned console_tx_head, console_tx_tail;
static unsigned tty_canonical[6]={1,1,1,1,1,1}, tty_echo[6]={1,1,1,1,1,1}, tty_onlcr[6]={1,1,1,1,1,1}, tty_interrupt[6]={1,1,1,1,1,1};
static unsigned tty_last_cr[6];
static int tty_mode_owner[6], foreground_pid[6], tty_shell_pid[6], tty_claim_pid[6], tty_restore_pid[6];
static int foreground_group[6][3];
typedef struct { uint8_t data[PTY_OUT_BYTES]; } unused_t;
typedef struct { uint8_t out[PTY_OUT_BYTES]; uint16_t head, used; unsigned master_refs, slave_refs; uint8_t ever_slave, screen_only; } pty_t;
static pty_t pty[PTY_COUNT];
typedef struct {
    char cells[VT_ROWS][VT_COLS];
    uint8_t inverse[VT_ROWS][(VT_COLS+7u)/8u];
    uint8_t row,col,esc,params,private_csi,reverse,cursor_hidden,clipped;
    uint16_t arg[3];
    uint32_t revision;
} vt_screen_t;
static vt_screen_t pty_screen[PTY_COUNT];
static uint8_t cdc_active;
static uint8_t cdc_dual_mode;
static struct {uint8_t phase;} cdc_repaint;
static void pty_screen_byte(int slot,uint8_t c);
static void tty_raw_enqueue(int dev,uint8_t c);
static int pty_master_rw(int slot,void *p,size_t n,int writing);
typedef struct {
    char line[128]; unsigned len;
    struct { char text[128]; uint8_t len; } lines[TTY_LINES];
    unsigned head, tail, used, offset;
} tty_buffer_t;
static tty_buffer_t tty_buf[6];
typedef struct { int pid,fd[16]; uint32_t fd_pos[16]; } task_t;
static task_t tasks[1];
static int current=0;
static void cdc_put(int tty,uint8_t c) {(void)tty;(void)c;}
static void wake_readers(int tty) {(void)tty;}
static unsigned char tty1_ctrl_c_pending;
static int interrupt_count;
static void tty_foreground_interrupt(int dev) { assert(dev==PTY_TTY || dev==PTY_TTY1); interrupt_count++; }
'''
functions = [extract(sig) for sig in (
    'NO_TRACE static void tty_mode_reset(',
    'NO_TRACE static void pty_ref(',
    'NO_TRACE static void vt_clear(',
    'NO_TRACE static void vt_scroll(',
    'NO_TRACE static void vt_lf(',
    'NO_TRACE static void vt_cursor_report(',
    'NO_TRACE static unsigned vt_arg(',
    'NO_TRACE static void vt_screen_byte(',
    'NO_TRACE static void pty_screen_byte(',
    'NO_TRACE static int pty_screen_read(',
    'NO_TRACE static int pty_key_write(',
    'NO_TRACE static int pty_output_put(',
    'NO_TRACE static int pty_output_read(',
)]
functions.insert(0, s[s.index('NO_TRACE static int pty_slot('):s.index('/* デバイスIDがパイプ',s.index('NO_TRACE static int pty_slot('))].strip().replace('static pty_t pty[PTY_COUNT];',''))
functions.append('static void tty_put_dev(int dev,uint8_t c) { if(dev==PTY_TTY || dev==PTY_TTY1) assert(pty_output_put(dev==PTY_TTY1,c)); }')
functions += [extract(sig) for sig in (
    'NO_TRACE static void tty_raw_enqueue(',
    'NO_TRACE static void tty_enqueue(',
    'static int tty_dequeue(',
    'NO_TRACE static int tty_tx_char(',
    'NO_TRACE static int pty_slave_write(',
    'static unsigned tty_line_columns(',
    'static void tty_input_char(',
    'static int pty_master_rw(',
)]
main = r'''
int main(void) {
    uint8_t tmp[1200]={0};
    task_t task={.pid=12};
    pty_ref(DEV_PTY_MASTER,1);
    pty_ref(DEV_PTY_SLAVE,1);
    assert(pty[0].master_refs==1 && pty[0].slave_refs==1);
    pty_ref(DEV_PTY_MASTER1,1);
    pty_ref(DEV_PTY_SLAVE1,1);
    assert(pty[1].master_refs==1 && pty[1].slave_refs==1);
    tty_canonical[PTY_TTY1]=0;tty_echo[PTY_TTY1]=0;
    assert(pty_master_rw(1,"slot1",5,1)==5);
    assert(tty_dequeue(PTY_TTY1,tmp,5)==5 && !memcmp(tmp,"slot1",5));
    assert(pty_slave_write(1,&task,(const uint8_t *)"slot1",5)==5);
    assert(pty_master_rw(1,tmp,5,0)==5 && !memcmp(tmp,"slot1",5));
    assert(pty[0].used==0 && pty[1].used==0);
    pty_ref(DEV_PTY_MASTER1,-1);pty_ref(DEV_PTY_SLAVE1,-1);
    assert(!pty[1].master_refs && !pty[1].slave_refs);
    /* canonical+echo: echo is sent to master, line is read by slave */
    assert(pty_master_rw(0,"abc\n",4,1)==4);
    assert(tty_dequeue(3,tmp,4)==4);
    assert(memcmp(tmp,"abc\n",4)==0);
    assert(pty_master_rw(0,tmp,1,0)>0);
    /* raw mode, no echo; Ctrl+C is passed when interrupt handling is off. */
    tty_interrupt[PTY_TTY]=0;
    tty_canonical[3]=0;
    tty_echo[3]=0;
    assert(pty_master_rw(0,"XY",2,1)==2);
    assert(tty_dequeue(3,tmp,2)==2 && memcmp(tmp,"XY",2)==0);
    assert(pty_master_rw(0,"\003",1,1)==1);
    assert(tty_dequeue(3,tmp,1)==1 && tmp[0]==3);
    /* PTY interrupt mode consumes Ctrl+C instead of enqueueing it. */
    tty_interrupt[PTY_TTY]=1;
    assert(pty_master_rw(0,"\003",1,1)==1);
    assert(interrupt_count==1);
    assert(pty_slave_write(0,&task,(const uint8_t *)"ok\n",3)==3);
    int n=pty_master_rw(0,tmp,sizeof(tmp),0);
    assert(n>=4 && memcmp(tmp+n-4,"ok\r\n",4)==0);
    /* Backpressure: full output ring cannot overwrite existing bytes. */
    while(pty[0].used<PTY_OUT_BYTES) assert(pty_output_put(0,'z'));
    assert(!pty_output_put(0,'x'));
    assert(pty_slave_write(0,&task,(const uint8_t *)"!",1)==OS_EAGAIN);
    assert(pty_master_rw(0,tmp,1000,0)==1000);
    assert(pty_slave_write(0,&task,(const uint8_t *)"!",1)==1);
    assert(pty_master_rw(0,tmp,sizeof(tmp),0)==25);
    assert(tmp[24]=='!');
    /* VT80x24: 画面復元，カーソル，ANSIの処理とキー入力を確認する． */
    vt_clear(&pty_screen[0]);pty_screen[0].row=pty_screen[0].col=0;pty_screen[0].esc=0;
    for(const char *p="\033[2J\033[HHello\033[3;1HWorld";*p;p++)pty_screen_byte(0,(uint8_t)*p);
    assert(!memcmp(pty_screen[0].cells[0],"Hello",5));
    assert(!memcmp(pty_screen[0].cells[2],"World",5));
    /* DECTCEM cursor visibility during redraw; final move restores marker. */
    for(const char *p="\033[?25l";*p;p++) pty_screen_byte(0,(uint8_t)*p);
    assert(pty_screen[0].cursor_hidden);
    tasks[0].fd_pos[4]=0;tasks[0].fd[4]=DEV_PTY_SCREEN;
    char hidden_screen[2200];unsigned hidden_size=0;int hn;
    while((hn=pty_screen_read(4,hidden_screen+hidden_size,sizeof hidden_screen-hidden_size))>0) hidden_size+=(unsigned)hn;
    assert(hidden_size<sizeof hidden_screen);
    hidden_screen[hidden_size]=0;
    assert(strstr(hidden_screen,"\033[4m")==NULL);
    for(const char *p="\033[3;2H\033[?25h";*p;p++) pty_screen_byte(0,(uint8_t)*p);
    assert(!pty_screen[0].cursor_hidden && pty_screen[0].row==2 && pty_screen[0].col==1);
    tasks[0].fd_pos[4]=0;tasks[0].fd[4]=DEV_PTY_SCREEN;
    char visible_screen[2200];unsigned visible_size=0;
    while((hn=pty_screen_read(4,visible_screen+visible_size,sizeof visible_screen-visible_size))>0) visible_size+=(unsigned)hn;
    assert(visible_size<sizeof visible_screen);
    visible_screen[visible_size]=0;
    assert(strstr(visible_screen,"\033[4mo\033[24m")!=NULL); /* World[1] */
    for(const char *p="\033[?25l\033c";*p;p++) pty_screen_byte(0,(uint8_t)*p);
    assert(!pty_screen[0].cursor_hidden); /* hard reset defaults to visible */
    for(const char *p="\033[2J\033[HHello\033[3;1HWorld";*p;p++)pty_screen_byte(0,(uint8_t)*p);
    tasks[0].fd_pos[4]=0;tasks[0].fd[4]=DEV_PTY_SCREEN;
    char screen[82];
    assert(pty_screen_read(4,screen,81)==81);
    assert(!memcmp(screen,"Hello",5) && screen[80]=='\n');
    tty_canonical[3]=0; tty_echo[3]=0;
    assert(pty_key_write(0,"SPACE",5)==5);
    assert(tty_dequeue(3,tmp,1)==1 && tmp[0]==' ');
    assert(pty_key_write(0,"SP",2)==2);
    assert(tty_dequeue(3,tmp,1)==1 && tmp[0]==' ');
    assert(pty_key_write(0,"DOWN",4)==4);
    assert(tty_dequeue(3,tmp,3)==3 && !memcmp(tmp,"\033[B",3));
    assert(pty_key_write(0,"CTRL_C",6)==6);
    assert(interrupt_count==2);
    /* Cursor report matches the fixed 80x24 terminal dimensions. */
    tty_mode_reset(PTY_TTY);
    tty_canonical[3]=0;tty_echo[3]=0;
    for(const char *p="\033[999;999H\033[6n";*p;p++)pty_screen_byte(0,(uint8_t)*p);
    assert(tty_dequeue(3,tmp,32)==8);
    assert(!memcmp(tmp,"\033[24;80R",8));
    /* Screen-only mode bypasses the raw output ring and never blocks redraw. */
    pty[0].used=0;pty[0].head=0;pty[0].screen_only=1;
    tty_echo[3]=1;
    assert(pty_slave_write(0,&task,(const uint8_t *)"MCP\n",4)==4);
    assert(pty[0].used==0);
    assert(pty_output_put(0,'x') && pty[0].used==0);
    assert(pty_master_rw(0,tmp,1,0)==OS_EAGAIN);
    assert(pty_master_rw(0,"L",1,1)==1);
    assert(pty[0].used==0);
    assert(!pty_screen[0].cursor_hidden);
    /* Master close disconnects slave; resources reset only after last FD. */
    pty_ref(DEV_PTY_MASTER,-1);
    assert(pty[0].slave_refs==1);
    assert(pty_slave_write(0,&task,(const uint8_t *)"!",1)==OS_EIO);
    pty_ref(DEV_PTY_SLAVE,-1);
    assert(!pty[0].used && !pty[0].master_refs && !pty[0].slave_refs && !pty[0].screen_only);
    assert(tty_canonical[3] && tty_echo[3] && !tty_buf[3].used);
    /* No residual state between sessions. */
    pty_ref(DEV_PTY_MASTER,1);
    assert(pty_master_rw(0,tmp,1,0)==OS_EAGAIN);
    pty_ref(DEV_PTY_MASTER,-1);
    puts("PASS: PTY canonical/raw queues, echo, CRLF, output backpressure, close/reset");
    return 0;
}
'''
with tempfile.TemporaryDirectory() as td:
    src=Path(td)/'pty_test.c'
    binary=Path(td)/'pty_test'
    src.write_text('\n'.join([prefix,*functions,main]))
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-function','-o',str(binary),str(src)],check=True)
    subprocess.run([str(binary)],check=True)
