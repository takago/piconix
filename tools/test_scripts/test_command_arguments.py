#!/usr/bin/env python3
"""All registered user commands own argv validation; the shell stays generic."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
apps = root / 'userland/apps'
shell = (apps / 'shell.c').read_text()
assert 'path_arg=' not in shell
assert 'args[path_arg]' not in shell
assert 'args[argc++]="31"' not in shell
for name in ('pwd','tty','reset','ps','blink','fbed','gpio','mb','kitesys','httpd'):
    s = (apps / (('shell' if name=='sh' else name) + '.c')).read_text()
    assert 'if(argc!=1)' in s, name
# The shell additionally accepts `sh -c COMMAND` for noninteractive execution.
assert 'if(argc!=1 && !(argc==3 && !strcmp(argv[1],"-c")))' in shell
for name in ('python', 'hello', 'omikuji'):
    assert 'if(argc>2)' in (apps / f'{name}.c').read_text(), name
assert 'value<1 || value>30' in (apps/'omikuji.c').read_text()
assert 'scanf("%d",&seconds)' in (apps/'omikuji.c').read_text()
for name in ('cat','ls','grep','dd','hexdump','tree','find','ping','tcpdump','nc',
             'watch','trace','edit','vi','fd','pwmctl','i2cctl','stty','kill','lsof',
             'asm','bh1750','fbctl','net','pong','mem','tcp_echod','udp_echod'):
    s=(apps / f'{name}.c').read_text()
    assert re.search(r'int main\s*\(\s*int argc',s), name
    assert 'argc' in s[s.index('int main'):], name
print('PASS: command-owned argument checks and generic shell')
