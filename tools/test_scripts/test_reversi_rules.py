#!/usr/bin/env python3
"""Compile the actual Reversi rule functions on the host and smoke-test them."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'userland/apps/reversi.c').read_text()
core = source[source.index('static int inside'):source.index('static void render')]
harness = '''#include <stdio.h>
#define N 8
#define EMPTY 0
#define BLACK 1
#define WHITE 2
''' + core + '''
int main(void){
 unsigned char b[64]={0};
 b[3*8+3]=WHITE;b[3*8+4]=BLACK;b[4*8+3]=BLACK;b[4*8+4]=WHITE;
 if(move(b,3,2,BLACK,0)!=1 || b[2*8+3])return 1;
 if(move(b,3,2,BLACK,1)!=1 || b[3*8+3]!=BLACK || b[2*8+3]!=BLACK)return 2;
 if(move(b,3,2,BLACK,1)!=0 || !can_move(b,WHITE))return 3;
 int x,y,invalid_com=0;computer_move(b,&x,&y,&invalid_com);
 if(invalid_com)return 6;
 if(!inside(x,y)||b[y*8+x]!=WHITE)return 4;
 unsigned char empty[64]={0};
 if(can_move(empty,BLACK)||can_move(empty,WHITE))return 5;
 int invalid_player=0;
 if(player_move(b,3,2,&invalid_player)!=0 || invalid_player!=1)return 7;
 if(player_move(b,0,0,&invalid_player)!=0 || invalid_player!=2)return 8;
 unsigned char fresh[64]={0};
 fresh[3*8+3]=WHITE;fresh[3*8+4]=BLACK;
 fresh[4*8+3]=BLACK;fresh[4*8+4]=WHITE;
 if(player_move(fresh,3,2,&invalid_player)!=1 || invalid_player!=2)return 9;
 if(fresh[3*8+3]!=BLACK)return 10;
 puts("PASS: reversi rules and COM reply");return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    src = Path(tmp) / 'test.c'
    exe = Path(tmp) / 'test'
    src.write_text(harness)
    subprocess.run(['cc', '-Wall', '-Wextra', '-Werror', str(src), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
assert 'reversi' in (root / 'CMakeLists.txt').read_text()
assert 'register_image(57,"reversi"' in (root / 'kernel/main.c').read_text()

# Rendering and turn-transition regression checks.
assert 'if(x==lastx&&y==lasty)printf("[%c]",c);' in source
assert 'if(selected)printf("\\033[7m");' in source
assert 'msleep(1000);' in source
assert 'lastx=mx;lasty=my;' in source
assert 'lastx=x;lasty=y;' in source
assert 'render(board,x,y,turn,0,moves,lastx,lasty,invalid_black,invalid_white,"COM is thinking...");' in source
assert 'Invalid moves: PLAYER %d  COM %d' in source
assert 'if(!player_move(board,x,y,&invalid_black))' in source
assert 'computer_move(board,&mx,&my,&invalid_white)' in source
