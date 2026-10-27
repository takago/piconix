"""Static integration checks for the compact watch/ps/cat update."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
k=(r/'src/kernel.c').read_text()
m=(r/'src/main.c').read_text()
sh=(r/'ram_apps/shell.c').read_text()
w=(r/'ram_apps/watch.c').read_text()
for i,name in enumerate(('ps','cat','watch'),8):
    assert f'register_image({i},' in m and f'"{name}"' in m
    assert f'ram_apps/{name}.c' in (r/'CMakeLists.txt').read_text() or name in (r/'CMakeLists.txt').read_text()
assert 'else if(eq(s,"ps"))' not in sh
assert 'else if(eq(s,"cat")' not in sh
assert '"ps\\r\\n' not in sh
assert '"cat\\r\\n' not in sh
assert 'uart1_irq_count++' in k and 'console_irq_count++' in k
assert '(unsigned long)t' in k and 'ticks += 1u' in k
assert 'spawn(id,count,args)' in w and 'waitpid(child,0,0)' in w
print('PASS: external apps, watch and IRQ counters integrated')
