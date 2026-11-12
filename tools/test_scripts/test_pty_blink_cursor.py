#!/usr/bin/env python3
"""Host regression: PTY screen marks exactly the cursor cell, across split reads."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
s = (root / 'kernel/kernel.c').read_text()
start = s.index('NO_TRACE static int pty_screen_read(int fd, void *p, size_t n) {')
brace = s.index('{', start)
depth = 0
for end in range(brace, len(s)):
    depth += (s[end] == '{') - (s[end] == '}')
    if depth == 0:
        break
func = s[start:end + 1]
source = r'''
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#define NO_TRACE
#define VT_COLS 80u
#define VT_ROWS 24u
#define PTY_COUNT 2
#define DEV_PTY_SCREEN 20
#define DEV_PTY_SCREEN1 21
typedef struct {
    char cells[VT_ROWS][VT_COLS];
    uint8_t inverse[VT_ROWS][(VT_COLS+7u)/8u];
    uint8_t row,col,esc,params,private_csi,reverse,cursor_hidden,clipped;
    uint16_t arg[3];
    uint32_t revision;
} vt_screen_t;
static vt_screen_t pty_screen[PTY_COUNT];
static struct { unsigned fd_pos[8]; int fd[8]; } tasks[1];
static int current;
static int pty_slot(int dev) {
    return dev==DEV_PTY_SCREEN ? 0 : dev==DEV_PTY_SCREEN1 ? 1 : -1;
}
''' + func + r'''
int main(void) {
    tasks[0].fd[3]=DEV_PTY_SCREEN;
    tasks[0].fd[4]=DEV_PTY_SCREEN1;
    memset(pty_screen,0,sizeof pty_screen);
    for(unsigned slot=0;slot<PTY_COUNT;slot++)
        memset(pty_screen[slot].cells,' ',sizeof pty_screen[slot].cells);
    memcpy(pty_screen[0].cells[0],"ABC",3);
    pty_screen[0].inverse[0][0] = 2; /* inverse B */
    pty_screen[0].row=0;pty_screen[0].col=1; /* cursor on reverse B */
    char all[2400], split[2400];size_t used=0;
    int n;
    while((n=pty_screen_read(3,all+used,sizeof all-used))>0) used+=(size_t)n;
    assert(used>1944 && used<sizeof all);all[used]=0;
    assert(!memcmp(all,"A\033[7m\033[4mB\033[24m\033[0mC",sizeof("A\033[7m\033[4mB\033[24m\033[0mC")-1));
    assert(strstr(all,"\033[4mB\033[24m")!=0);
    tasks[0].fd_pos[3]=0;
    size_t done=0;
    while((n=pty_screen_read(3,split+done,7))>0) done+=(size_t)n;
    assert(done==used && !memcmp(all,split,used));
    /* Cursor changes without modifying screen cells. */
    pty_screen[0].col=2;tasks[0].fd_pos[3]=0;
    char moved[2400];size_t count=0;
    while((n=pty_screen_read(3,moved+count,sizeof moved-count))>0) count+=(size_t)n;
    moved[count]=0;assert(strstr(moved,"\033[4mC\033[24m")!=0);
    assert(memcmp(moved,all,used)!=0);
    /* Cursor at a blank cell still has a marker. */
    pty_screen[0].row=1;pty_screen[0].col=0;tasks[0].fd_pos[3]=0;
    count=0;
    while((n=pty_screen_read(3,moved+count,sizeof moved-count))>0) count+=(size_t)n;
    moved[count]=0;assert(strstr(moved,"\n\033[4m \033[24m")!=0);
    pty_screen[0].cursor_hidden=1;tasks[0].fd_pos[3]=0;
    count=0;
    while((n=pty_screen_read(3,moved+count,sizeof moved-count))>0) count+=(size_t)n;
    moved[count]=0;assert(strstr(moved,"\033[4m")==0);
    pty_screen[0].cursor_hidden=0;tasks[0].fd_pos[3]=0;
    count=0;
    while((n=pty_screen_read(3,moved+count,sizeof moved-count))>0) count+=(size_t)n;
    moved[count]=0;assert(strstr(moved,"\033[4m \033[24m")!=0);
    /* The second PTY must return its own screen, not PTY 0's. */
    memcpy(pty_screen[1].cells[0],"XYZ",3);
    pty_screen[1].cursor_hidden=1;
    char other[2400];size_t other_count=0;
    while((n=pty_screen_read(4,other+other_count,sizeof other-other_count))>0)
        other_count+=(size_t)n;
    assert(other_count>=1944 && other_count<sizeof other);
    assert(!memcmp(other,"XYZ",3));
    assert(memcmp(other,all,3)!=0);
    puts("PASS: two PTYs, reverse + underline cursor, hidden cursor, blank cursor and chunked reads");
}
'''
with tempfile.TemporaryDirectory() as td:
    src = Path(td) / 'cursor.c'
    binary = Path(td) / 'cursor'
    src.write_text(source)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-o', str(binary), str(src)], check=True)
    subprocess.run([str(binary)], check=True)
