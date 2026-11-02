"""Audit hazards that must be resolved before enabling pipe pipelines."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]
k=(root/'kernel/kernel.c').read_text()
s=(root/'userland/shell.c').read_text()
checks={
 'sixteen descriptors per task': 'int fd[OS_FD_MAX]' in k and '#define OS_FD_MAX 16' in k,
 'spawn inherits standard streams': 'for(int j=0;j<3;j++) fd_inherit(t,j,&tasks[current],j);' in k,
 'dup2 dispatch exists': 'case SYS_DUP2:' in k,
 'close dispatch exists': 'case SYS_CLOSE:' in k,
 'task termination exists': 'static void finish_task(' in k,
 'shell uses spawn': 'spawn(id,argc' in s,
 'shell uses waitpid': 'waitpid(pid,&status,0)' in s,
}
for name,ok in checks.items(): print(('PASS' if ok else 'FAIL')+': '+name)
assert all(checks.values()), 'Baseline architecture changed; revisit pipe design'
print('NOTE: This is a baseline audit, not a functional pipe test.')
