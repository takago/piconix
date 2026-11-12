#!/usr/bin/env python3
"""Exercise the actual CDC/PTY display parser and incremental USB repaint code on a host."""
import subprocess
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[2]
s = (root / 'kernel/kernel.c').read_text()
dev = (root / 'include/device_names.h').read_text()
assert '#define CDC_VCONSOLE_TTY 4' in s
assert '#define OS_TTY_COUNT 5' in s
assert 'X("/dev/ttyV0", DEV_CONSOLE)' in dev
assert 'X("/dev/ttyV1", DEV_CDC_VCONSOLE)' in dev
assert 'X("/dev/console", DEV_CDC_ACTIVE)' in dev
assert 'if(dev==DEV_CDC_ACTIVE) dev=cdc_active ? DEV_CDC_VCONSOLE : DEV_CONSOLE;' in s
assert s.index('if(dev==DEV_CDC_ACTIVE) dev=cdc_active ? DEV_CDC_VCONSOLE : DEV_CONSOLE;') < s.index('fd_set(&tasks[current],fd,dev,')
assert 'if(ch==0x14)' in s and 'cdc_vconsole_switch();' in s
assert 'if(ch==0x07)' in s and 'cdc_dual_mode^=1u;' in s
assert 'cdc_shown_revision[0]!=cdc_screen[0].revision' in s
assert 'wake_readers(CDC_VCONSOLE_TTY)' in s
assert 'if(dev==DEV_CDC_VCONSOLE)' in s or 'dev==DEV_CDC_VCONSOLE' in s
assert 'child->parent_pid=0;' in s
assert 'child->tty=CDC_VCONSOLE_TTY;' in s
assert 'tty_shell_pid[CDC_VCONSOLE_TTY]=pid;' in s
assert 'if(pid<0) {tty_put' in s
assert 'cdc_repaint.revision=cdc_screen[cdc_active].revision;' in s
assert 'if(screen->revision!=cdc_repaint.revision ||' in s
assert 'cdc_repaint.phase=0;' in s
assert 'tasks[i].tty==CDC_VCONSOLE_TTY ? 0u : tasks[i].tty' in s

vt = s[s.index('/* 仮想画面の最小ANSI解釈．'):s.index('/* 80x24の反転表示を再生成')]
queue = s[s.index('/* USB CDCへの送信データを固定長リングへ積む．'):s.index('/* A switch first drops queued bytes')]
paint = s[s.index('/* A switch first drops queued bytes'):s.index('/* アイドルタスク側でUSB送信リングを少量ずつ空にする')]
put = s[s.index('NO_TRACE static void tty_put(uint8_t c)'):s.index('/* TTY番号に応じてUSB CDC')]

harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NO_TRACE
#define PTY_TTY 3
#define CDC_VCONSOLE_TTY 4
#define VT_COLS 80u
#define VT_ROWS 24u
#define CONSOLE_TX_CAP 512u
static uint8_t console_tx[CONSOLE_TX_CAP];
static unsigned console_tx_head,console_tx_tail;
typedef struct {
    char cells[VT_ROWS][VT_COLS];
    uint8_t inverse[VT_ROWS][(VT_COLS+7u)/8u];
    uint8_t row,col,esc,params,private_csi,reverse,cursor_hidden,clipped;
    uint16_t arg[3];
    uint32_t revision;
} vt_screen_t;
static vt_screen_t pty_screen;
static vt_screen_t cdc_screen[2];
static uint8_t cdc_active;
static uint8_t cdc_dual_mode;
static uint32_t cdc_shown_revision[2];
static struct {
    uint8_t phase,row,col,reverse,pending,seq_at,panel;
    uint32_t revision,revision_other;
    char seq[80];
} cdc_repaint;
static unsigned cursor_reply_bytes;
static void tty_raw_enqueue(int tty,uint8_t c) {(void)tty;(void)c;cursor_reply_bytes++;}
static void wake_readers(int tty) {(void)tty;}
'''
source = harness + '\n' + vt + '\n' + queue + '\n' + paint + '\n' + put + r'''
static char sent[20000];
static size_t sent_count;
static void flush_usb(void) {
    while(console_tx_tail!=console_tx_head) {
        assert(sent_count<sizeof(sent));
        sent[sent_count++]=(char)console_tx[console_tx_tail];
        console_tx_tail=(console_tx_tail+1u)%CONSOLE_TX_CAP;
    }
}
static void finish_redraw(void) {
    for(int i=0;i<300 && (cdc_repaint.phase || console_tx_head!=console_tx_tail);i++) {
        cdc_vconsole_draw();
        flush_usb();
    }
    assert(cdc_repaint.phase==0);
    assert(console_tx_head==console_tx_tail);
}
int main(void) {
    assert(sizeof(vt_screen_t)<2300u);
    vt_clear(&pty_screen);
    vt_clear(&cdc_screen[0]);
    vt_clear(&cdc_screen[1]);
    cdc_put(0,'A');
    flush_usb();
    assert(sent_count==1 && sent[0]=='A');
    cdc_put(CDC_VCONSOLE_TTY,'B');
    assert(console_tx_head==console_tx_tail);  /* hidden writes cannot leak */
    assert(cdc_screen[0].cells[0][0]=='A');
    assert(cdc_screen[1].cells[0][0]=='B');
    assert(pty_screen.cells[0][0]==' ');
    cdc_active=1;
    cdc_vconsole_begin_repaint();
    cdc_vconsole_draw();
    cdc_put(CDC_VCONSOLE_TTY,'C'); /* simulated concurrent task output */
    finish_redraw(); /* revision change must trigger another repaint */
    assert(strstr(sent,"\033[2J")!=NULL);
    assert(strstr(sent,"BC")!=NULL);
    assert(cdc_screen[1].cells[0][1]=='C');
    /* Reverse video is retained in the virtual screen state. */
    for(const char *a="\033[7mX\033[0m";*a;a++) cdc_put(0,(uint8_t)*a);
    assert(cdc_screen[0].inverse[0][0] & 2u);
    cdc_active=0;
    cdc_vconsole_begin_repaint();
    sent_count=0;
    finish_redraw();
    assert(strstr(sent,"\033[7m")!=NULL);
    assert(strstr(sent,"A\033[7mX")!=NULL);
    /* The physical host answers active CDC CSI 6n, but hidden CDC needs
     * a synthesized reply because no host terminal can see that request. */
    cursor_reply_bytes=0;
    for(const char *p="\033[6n";*p;p++) cdc_put(0,(uint8_t)*p);
    assert(cursor_reply_bytes==0);
    for(const char *p="\033[6n";*p;p++) cdc_put(CDC_VCONSOLE_TTY,(uint8_t)*p);
    assert(cursor_reply_bytes>0);
    /* Screen capture clips, rather than wraps, lines longer than 80 columns. */
    vt_screen_t wide={0};
    vt_clear(&wide);
    for(int i=0;i<115;i++) vt_screen_byte(&wide,PTY_TTY,(uint8_t)('a'+(i%26)));
    assert(wide.row==0 && wide.col==79 && wide.clipped==1);
    assert(wide.cells[0][0]=='a' && wide.cells[0][79]==('a'+79%26));
    assert(wide.cells[1][0]==' ');
    vt_screen_byte(&wide,PTY_TTY,'\r');
    vt_screen_byte(&wide,PTY_TTY,'Z');
    assert(wide.cells[0][0]=='Z' && wide.clipped==0);
    vt_screen_byte(&wide,PTY_TTY,'\n');
    vt_screen_byte(&wide,PTY_TTY,'Q');
    assert(wide.cells[1][1]=='Q'); /* LF preserves column, as on the existing parser. */
    /* Stacked mode must render both 80x24 views at separate coordinates. */
    cdc_dual_mode=1;
    cdc_vconsole_begin_repaint();
    sent_count=0;
    finish_redraw();
    assert(strstr(sent,"[/dev/ttyV0]")!=NULL);
    assert(strstr(sent,"[/dev/ttyV1]")!=NULL);
    assert(strstr(sent,"C-Left/Right: V0/V1/Dual  C-Up/Down: Focus (Dual)  C-T/G: Legacy")!=NULL);
    assert(strstr(sent,"\033[27;1H")!=NULL);
    /* The cursor must remain hidden until both full-width title bars and
     * the bottom help bar have been written. */
    char *header0=strstr(sent,"\033[1;1H\033[7m\033[1m[/dev/ttyV0] ACTIVE");
    char *header1=strstr(sent,"\033[26;1H\033[7m[/dev/ttyV1]");
    char *footer=strstr(sent,"\033[51;1H\033[7mC-Left/Right: V0/V1/Dual  C-Up/Down: Focus (Dual)  C-T/G: Legacy");
    char *cursor=strstr(sent,"\033[?25h");
    assert(header0 && header1 && footer && cursor);
    assert(header0<header1 && header1<footer && footer<cursor);
    assert(strstr(sent,"\033[?25l")!=NULL);
    /* Width of the bars is exactly 80 columns including their labels. */
    for(int i=0;i<61;i++) assert(header0[sizeof("\033[1;1H\033[7m\033[1m[/dev/ttyV0] ACTIVE")-1+i]==' ');
    for(int i=0;i<68;i++) assert(header1[sizeof("\033[26;1H\033[7m[/dev/ttyV1]")-1+i]==' ');
    /* Switching focus highlights Console 1, without changing either buffer. */
    cdc_active=1;
    cdc_vconsole_begin_repaint();
    sent_count=0;
    finish_redraw();
    assert(strstr(sent,"\033[26;1H\033[7m\033[1m[/dev/ttyV1] ACTIVE")!=NULL);
    /* A hidden task's output triggers repaint without writing raw bytes. */
    cdc_put(CDC_VCONSOLE_TTY,'Z');
    assert(console_tx_head==console_tx_tail);
    assert(cdc_shown_revision[1]!=cdc_screen[1].revision);
    cdc_dual_mode=0;
    cdc_vconsole_begin_repaint();
    sent_count=0;
    finish_redraw();
    assert(strstr(sent,"\033[?7h")!=NULL);
    puts("PASS: independent CDC displays, stacked dual display, background output, reverse video, incremental repaint");
    return 0;
}
'''
with tempfile.TemporaryDirectory() as temp:
    c = Path(temp) / 'vt_test.c'
    exe = Path(temp) / 'vt_test'
    c.write_text(source)
    cc = ['clang', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
          '-Wno-unused-variable', str(c), '-o', str(exe)]
    subprocess.run(cc, check=True)
    subprocess.run([str(exe)], check=True)
