#!/usr/bin/env python3
"""Static regression contract for VFS dot entries and atomic foreground spawn."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
k=(root/'kernel/kernel.c').read_text()
a=(root/'userland/apps').resolve()
shell=(a/'shell.c').read_text()
fd=(a/'fd.c').read_text()
walk=(a/'walk.h').read_text()
ls=(a/'ls.c').read_text()
lib=(root/'userland/lib/syscalls.c').read_text()
assert 'EMIT(".",1); EMIT("..",1);' in k
assert k.index('EMIT(".",1); EMIT("..",1);') < k.index('switch(dir) {',k.index('static int vdir_nth('))
assert 'if(walk_eq(entry.name,".")||walk_eq(entry.name,".."))continue;' in walk
assert 'if(!strcmp(e.name,".")) continue;' in fd
assert 'entries[count++].dir=1' not in fd
assert 'if(!show_all && e.name[0]==\'.\')continue;' in ls
assert 'if(result>0 && foreground) foreground_child(result);' in k
assert 'if(result>0 && b==1) foreground_child(result);' in k
assert 'int spawn_foreground(' in lib and 'int spawn_tmp_foreground(' in lib
assert 'spawn_foreground(id,argc' in shell
assert 'spawn_tmp_foreground(path)' in shell
assert 'spawn_foreground(editor,2,args)' in fd
assert 'shell_list_matches(line,len,cwd)' in shell
assert 'tab_pending=matches>1;' in shell
print('PASS: VFS dot entries, traversal filters, foreground handoff and double Tab')
