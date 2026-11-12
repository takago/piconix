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
static struct {
    char cells[VT_ROWS][VT_COLS];
    uint8_t inverse[VT_ROWS][(VT_COLS+7u)/8u];
    uint8_t row,col,esc,params,private_csi,reverse,cursor_hidden,clipped;
    uint16_t arg[3];
    uint32_t revision;
} pty_screen;
static struct {unsigned fd_pos[8];} tasks[1];
static int current;
''' + func + r'''
int main(void) {
    memset(pty_screen.cells,' ',sizeof pty_screen.cells);
    memcpy(pty_screen.cells[0],"ABC",3);
    pty_screen.inverse[0][0] = 2; /* inverse B */
    pty_screen.row=0;pty_screen.col=1; /* cursor on reverse B */
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
    pty_screen.col=2;tasks[0].fd_pos[3]=0;
    char moved[2400];size_t count=0;
    while((n=pty_screen_read(3,moved+count,sizeof moved-count))>0) count+=(size_t)n;
    moved[count]=0;assert(strstr(moved,"\033[4mC\033[24m")!=0);
    assert(memcmp(moved,all,used)!=0);
    /* Cursor at a blank cell still has a marker. */
    pty_screen.row=1;pty_screen.col=0;tasks[0].fd_pos[3]=0;
    count=0;
    while((n=pty_screen_read(3,moved+count,sizeof moved-count))>0) count+=(size_t)n;
    moved[count]=0;assert(strstr(moved,"\n\033[4m \033[24m")!=0);
    pty_screen.cursor_hidden=1;tasks[0].fd_pos[3]=0;
    count=0;
    while((n=pty_screen_read(3,moved+count,sizeof moved-count))>0) count+=(size_t)n;
    moved[count]=0;assert(strstr(moved,"\033[4m")==0);
    pty_screen.cursor_hidden=0;tasks[0].fd_pos[3]=0;
    count=0;
    while((n=pty_screen_read(3,moved+count,sizeof moved-count))>0) count+=(size_t)n;
    moved[count]=0;assert(strstr(moved,"\033[4m \033[24m")!=0);
    puts("PASS: reverse + underline cursor, hidden cursor, blank cursor and chunked reads");
}
'''
with tempfile.TemporaryDirectory() as td:
    src = Path(td) / 'cursor.c'
    binary = Path(td) / 'cursor'
    src.write_text(source)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-o', str(binary), str(src)], check=True)
    subprocess.run([str(binary)], check=True)
