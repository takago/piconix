#!/usr/bin/env python3
"""Exercise actual Sudoku and SameGame C rules with a host compiler."""
from pathlib import Path
import subprocess
import tempfile
import re

root=Path(__file__).resolve().parents[2]
sudoku=(root/'userland/apps/sudoku.c').read_text()
samegame=(root/'userland/apps/samegame.c').read_text()
sud_core=sudoku[sudoku.index('static const char puzzle4'):sudoku.index('static void render')]
sud_core=sud_core[:sud_core.index('/* Read the')] + sud_core[sud_core.index('/* Pseudorandom'):]
sud_core=sud_core[:sud_core.index('/* Keep the header')]
same_core=samegame[samegame.index('static unsigned random32'):samegame.index('static void render')]

def run(src):
    with tempfile.TemporaryDirectory() as temp:
        c=Path(temp)/'test.c'
        exe=Path(temp)/'test'
        c.write_text(src)
        subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True,timeout=30)

run(r'''#include <stdio.h>
#include <string.h>
'''+sud_core+r'''
/* Count solutions independently of the generator, stopping at two. */
static int solve(unsigned char b[81],int n){
  int chosen=-1,nopts=10,mask=0;
  for(int p=0;p<n*n;p++)if(!b[p]){
    int local=0,count=0;
    for(int v=1;v<=n;v++)if(allowed(b,n,p,v)){local|=1<<(v-1);count++;}
    if(!count)return 0;
    if(count<nopts){chosen=p;nopts=count;mask=local;}
  }
  if(chosen<0)return 1;
  int total=0;
  for(int v=1;v<=n;v++)if(mask&(1<<(v-1))){
    b[chosen]=v;
    total+=solve(b,n);
    b[chosen]=0;
    if(total>=2)return total;
  }
  return total;
}
int main(void){
 unsigned char b[81],given[81],again[81],copy[81],work[81],reference[81];
 int unique4=0,unique9=0;
 for(int n=4;n<=9;n+=5){
  int varied=0;
  for(unsigned seed=0;seed<200;seed++){
   generate(n,seed,b,given);
   generate(n,seed,again,copy);
   if(memcmp(b,again,n*n)||memcmp(given,copy,n*n))return 1;
   if(seed==0)memcpy(reference,b,n*n);
   if(seed>0&&memcmp(b,reference,n*n))varied++;
   for(int p=0;p<n*n;p++)if(b[p]&&!allowed(b,n,p,b[p]))return 2;
   memcpy(work,b,n*n);
   if(solve(work,n)!=1)return 3;
   if(n==4)unique4++;else unique9++;
  }
  if(varied<175)return 4;
 }
 generate(4,1234,b,given);
 int free=-1,occupied=-1;
 for(int p=0;p<16;p++){if(given[p]&&occupied<0)occupied=p;if(!given[p]&&free<0)free=p;}
 if(free<0||occupied<0)return 5;
 int rejected=0,done=0;
 place(b,given,4,occupied,0,&rejected,&done);
 if(rejected!=1||b[occupied]!=given[occupied])return 6;
 int y=free/4;
 int conflict=0;
 for(int x=0;x<4;x++)if(b[y*4+x])conflict=b[y*4+x];
 if(!conflict)return 7;
 /* A duplicate MUST be kept on screen, rather than rejected. */
 place(b,given,4,free,conflict,&rejected,&done);
 if(rejected!=1||b[free]!=conflict||count_conflicts(b,4)<2||done)return 8;
 /* Clearing it removes the conflicts; bad ranges are rejected. */
 place(b,given,4,free,0,&rejected,&done);
 if(b[free]||count_conflicts(b,4))return 9;
 place(b,given,4,free,5,&rejected,&done);
 if(rejected!=2||b[free])return 10;
 /* Filling a solved grid must end the game without answer-key checks. */
 memcpy(work,b,16);
 if(solve(work,4)!=1)return 11;
 /* Independently solve a known seeded puzzle, then assert full-grid success. */
 generate(4,4321,b,given);
 memcpy(work,b,16);
 if(solve(work,4)!=1)return 12;
 /* Use a canonical 4x4 solution for completed() without hidden-key data. */
 unsigned char full[81]={1,2,3,4,3,4,1,2,2,1,4,3,4,3,2,1};
 if(!completed(full,4))return 13;
 full[1]=1;
 if(completed(full,4)||count_conflicts(full,4)<2)return 14;
 printf("PASS: Sudoku unique solutions for %d 4x4 and %d 9x9 seeded puzzles; editable duplicates and conflicts\n",unique4,unique9);
 return 0;
}
''')

run(r'''#include <stdio.h>
#include <string.h>
#define W 20
#define H 12
#define CELLS (W*H)
'''+same_core+r'''
/* After gravity: no blocks float; after packing: no gaps between columns. */
static int packed(const unsigned char b[CELLS]){
  int emptycol=0;
  for(int x=0;x<W;x++){
    int empty=0;
    for(int y=H-1;y>=0;y--){
      unsigned char v=b[y*W+x];
      if(v!='A'&&v!='B'&&v!='C'&&v!=0)return 0;
      if(!v)empty=1;
      else if(empty)return 0;
    }
    if(!b[(H-1)*W+x])emptycol=1;
    else if(emptycol)return 0;
  }
  return 1;
}
int main(void){
 unsigned char b[CELLS],other[CELLS],before[CELLS],mark[CELLS];
 int varied=0;
 for(unsigned seed=0;seed<300;seed++){
  generate(b,seed);generate(other,seed);
  if(memcmp(b,other,CELLS)||!has_moves(b))return 1;
  if(seed>0&&memcmp(b,before,CELLS))varied++;
  memcpy(before,b,CELLS);
  int remaining=CELLS,score=0,turns=0;
  while(has_moves(b)){
   int p=-1,n=0;
   for(int i=0;i<CELLS;i++)if((n=group(b,i,mark))>=2){p=i;break;}
   if(p<0)return 2;
   int old_score=score;
   int removed=remove_group(b,p,&score,&remaining);
   if(removed!=n||removed<2||remaining<0||!packed(b))return 3;
   int expected=(n-2)*(n-2)+(remaining==0?1000:0);
   if(score-old_score!=expected)return 4;
   int count=0;
   for(int i=0;i<CELLS;i++)if(b[i])count++;
   if(count!=remaining)return 5;
   if(++turns>CELLS/2)return 6;
  }
  if(has_moves(b))return 7;
 }
 if(varied<295)return 8;
 if(direct_position('a','0','1')!=0||direct_position('t','1','2')!=CELLS-1)return 13;
 if(direct_position('a','1','0')!=9*W||direct_position('b','0','3')!=2*W+1)return 14;
 if(direct_position('u','0','1')!=-1||direct_position('a','1','3')!=-1)return 15;
 if(direct_position('a','0','0')!=-1||direct_position('a','1','9')!=-1)return 16;
 /* High score belongs to the process, not to one generated board. */
 int best=0;
 unsigned best_seed=0;
 update_best(42,1234,&best,&best_seed);
 if(best!=42||best_seed!=1234)return 10;
 update_best(12,5678,&best,&best_seed);
 if(best!=42||best_seed!=1234)return 11;
 update_best(101,5678,&best,&best_seed);
 if(best!=101||best_seed!=5678)return 12;
 /* Exact collapse: the surviving right-hand block must move to the left. */
 for(int i=0;i<CELLS;i++)b[i]=0;
 b[(H-2)*W+2]='B';b[(H-1)*W+2]='B';b[(H-1)*W+9]='C';
 compact(b);
 if(b[(H-2)*W]!='B'||b[(H-1)*W]!='B'||b[(H-1)*W+1]!='C'||!packed(b))return 9;
 puts("PASS: SameGame seeded boards, gravity, scoring, and per-process high-score retention (300 seeds)");
 return 0;
}
''')

# Exercise the actual C text renderers, not just their underlying rules.
def capture(src):
    with tempfile.TemporaryDirectory() as temp:
        c=Path(temp)/'screen.c'
        exe=Path(temp)/'screen'
        c.write_text(src)
        subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],check=True)
        raw=subprocess.check_output([str(exe)],timeout=10).decode()
        return re.sub(r"\x1b\[[0-9;]*[A-Za-z]",'',raw)

sudoku_render=sudoku[sudoku.index('/* Keep the header'):sudoku.index('/* Parse -4')]
output=capture('#include <stdio.h>\n#include <string.h>\n'+sud_core+sudoku_render+r'''
int main(void){
  unsigned char b[81],g[81];int rejected=0,finished=0;
  generate(9,12345,b,g);
  /* Find an editable cell, then enter a duplicate of an existing given. */
  int p=-1;
  for(int i=0;i<81;i++)if(!g[i]){p=i;break;}
  int val=0;
  for(int j=0;j<9;j++)if(b[(p/9)*9+j]){val=b[(p/9)*9+j];break;}
  place(b,g,9,p,val,&rejected,&finished);
  render(b,g,9,12345,p,rejected,finished,"Test rendering");
  puts("\n#4x4#");
  generate(4,12345,b,g);
  render(b,g,4,12345,0,0,0,"Test rendering");
  return 0;
}
''')
full,mini=output.split('#4x4#')
for n,segment in [(9,full),(4,mini)]:
    lines=segment.splitlines()
    assert max(map(len,lines))<=80,(n,max(map(len,lines)))
    assert len(lines)<=24,(n,len(lines))
    rows=[line for line in lines if re.match(r'^ [1-9] \|',line)]
    assert len(rows)==n,(n,rows)
    assert all(row.count('|')==n//(2 if n==4 else 3)+1 for row in rows)
    assert '[ ]' in segment and 'Given:' in segment and 'Cursor:' in segment
    assert 'Conflicts:' in segment and 'Rejected:' in segment
assert '[1]' in full and 'Conflicts: 0' not in full
same_render=samegame[samegame.index('/* Draw an 80-column-safe'):samegame.index('/* Start a seeded SameGame')]
same_output=capture('#include <stdio.h>\n#include <string.h>\n#define W 20\n#define H 12\n#define CELLS (W*H)\n'+same_core+same_render+r'''
int main(void){
  unsigned char b[CELLS];generate(b,1234);
  if(direct_position('t','1','2')!=CELLS-1)return 1;
  int score=0,remaining=CELLS,best=0;
  unsigned best_seed=1234;
  remove_group(b,0,&score,&remaining);
  update_best(120,4321,&best,&best_seed);
  render(b,1234,0,50,best,best_seed,remaining,1,0,"Test high score");
  return 0;
}
''')
assert 'Score: 50  Best: 120  Best seed: 4321' in same_output
same_lines=same_output.splitlines()
assert max(map(len,same_lines))<=80, max(map(len,same_lines))
assert len(same_lines)<=24, len(same_lines)
assert len([line for line in same_lines if re.match(r'^\s+\d{1,2} ',line)])==12
assert ' t ' in same_output and 'Remaining:' in same_output
assert ':a01 or :t12' in same_output
assert f'/{20*12}' in same_output
print('PASS: Sudoku ASCII bracket rendering, 4x4/9x9 dimensions, and SameGame high-score display')

for name,ident in [('sudoku',52),('samegame',53)]:
    assert (root/f'userland/apps/{name}.c').exists()
    assert name in (root/'CMakeLists.txt').read_text()
    assert f'register_image({ident},"{name}"' in (root/'kernel/main.c').read_text()
    assert f'else if(!strcmp(argv[1],"{name}"))' in (root/'userland/apps/help.c').read_text()
    assert (root/'manual'/name).exists()
    for lang in ('ja','en'):
        assert name+'.c' in (root/f'docs/{lang}/reference/commands/README.md').read_text()
        assert (root/f'docs/{lang}/reference/commands/{name.upper()}.md').exists()
print('PASS: both games are registered, documented and have ROM manuals')
