#!/usr/bin/env python3
"""Compile and execute actual game rule code on the host; validate integration."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[2]
tic=(root/'userland/apps/tictactoe.c').read_text()
maze=(root/'userland/apps/maze.c').read_text()
tic_core=tic[tic.index('static const unsigned char triples'):tic.index('static void render')]
maze_core=maze[maze.index('static unsigned random32'):maze.index('static void render')]
assert 'static const char *const map' not in maze and 'char map[H][W+1];' in maze, 'maze map must be task-local for PIC'

def run(name,src):
    with tempfile.TemporaryDirectory() as tmp:
        c=Path(tmp)/'test.c'
        exe=Path(tmp)/'test'
        c.write_text(src)
        subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True,timeout=10)

# Exhaustively traverse either PLAYER side against the actual COM logic.
run('tic', '''#include <stdio.h>
'''+tic_core+'''
static int verify(unsigned char b[9],int player,int com){
  int r=outcome(b);
  if(r==player)return 0;
  if(r)return 1;
  for(int k=0;k<9;k++)if(!b[k]){
    b[k]=(unsigned char)player;
    int after=outcome(b);
    if(after==player){b[k]=0;return 0;}
    if(after==0){
      int m=computer_move(b,com);
      if(m<0||b[m]!=com||!verify(b,player,com)){b[m]=0;b[k]=0;return 0;}
      b[m]=0;
    }
    b[k]=0;
  }
  return 1;
}
int main(void){
 unsigned char b[9]={0};
 if(outcome(b)||score(b,1)!=0)return 1;
 if(!verify(b,1,2))return 2;
 int m=computer_move(b,1);
 if(m!=4||b[m]!=1||!verify(b,2,1))return 3;
 b[0]=b[1]=b[2]=1;
 if(outcome(b)!=1)return 4;
 puts("PASS: tic-tac-toe COM cannot lose for either starting side");return 0;
}
''')

run('maze','''#include <stdio.h>
#include <string.h>
#define W 21
#define H 9
#define GOAL_X 17
#define GOAL_Y 7
'''+maze_core+'''
int main(void){
 char m[H][W+1],again[H][W+1],other[H][W+1];
 int differences=0;
 for(unsigned seed=0;seed<300;seed++){
  generate(m,seed);generate(again,seed);
  if(memcmp(m,again,sizeof(m)))return 1;
  if(seed==1)memcpy(other,m,sizeof(m));
  if(seed>1&&memcmp(m,other,sizeof(m)))differences++;
  for(int y=0;y<H;y++)for(int x=0;x<W;x++){
   if((x==0||y==0||x==W-1||y==H-1)&&traversable(m,x,y))return 2;
  }
  if(m[1][1]!='S'||m[GOAL_Y][GOAL_X]!='G')return 3;
  int dist=shortest(m);
  if(dist<1||dist>=W*H)return 4;
  int x=1,y=1,moves=0,hits=0;
  if(step(m,&x,&y,0,-1,&moves,&hits)!=0||x!=1||y!=1||moves||hits!=1)return 5;
  /* Walk a shortest route found by an independent BFS. */
  int d[W*H],prev[W*H],q[W*H],head=0,tail=0,start=W+1,goal=GOAL_Y*W+GOAL_X;
  for(int i=0;i<W*H;i++){d[i]=-1;prev[i]=-1;}
  d[start]=0;q[tail++]=start;
  while(head<tail){
   int p=q[head++],xx=p%W,yy=p/W;
   const int dx[]={1,-1,0,0},dy[]={0,0,1,-1};
   for(int i=0;i<4;i++){
    int nx=xx+dx[i],ny=yy+dy[i],v=ny*W+nx;
    if(traversable(m,nx,ny)&&d[v]<0){d[v]=d[p]+1;prev[v]=p;q[tail++]=v;}
   }
  }
  if(d[goal]!=dist)return 6;
  int path[W*H],n=0;
  for(int p=goal;p!=start;p=prev[p]){if(p<0||n>=W*H)return 7;path[n++]=p;}
  x=y=1;moves=hits=0;
  for(int i=n-1;i>=0;i--){
   int nx=path[i]%W,ny=path[i]/W;
   int result=step(m,&x,&y,nx-x,ny-y,&moves,&hits);
   if(result!=(i==0?2:1))return 8;
  }
  if(x!=GOAL_X||y!=GOAL_Y||moves!=dist||hits)return 9;
 }
 if(differences<200)return 10;
 puts("PASS: seeded mazes repeat, vary, stay connected; shortest path and step counters verified");
 return 0;
}
''')

for name,number in [('tictactoe',59),('maze',60)]:
    assert (root/f'userland/apps/{name}.c').exists()
    assert name in (root/'CMakeLists.txt').read_text()
    assert f'register_image({number},"{name}"' in (root/'kernel/main.c').read_text()
    assert f'else if(!strcmp(argv[1],"{name}"))' in (root/'userland/apps/help.c').read_text()
    assert f'manual/{name}' and (root/'manual'/name).exists()
    for lang in ('ja','en'):
        assert f'{name}.c' in (root/f'docs/{lang}/reference/commands/README.md').read_text()
print('PASS: commands, registration, documentation, and manuals')

assert 'strcmp(argv[1],"-c")' in tic
assert 'if(com==1)' in tic
