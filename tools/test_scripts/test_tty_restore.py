from pathlib import Path
s = (Path(__file__).resolve().parents[2] / 'kernel/kernel.c').read_text()
assert 'tty_restore_pid[2]' in s
assert 'tty_saved_canon[tty]=tty_canonical[tty]' in s
assert 'tty_saved_echo[tty]=tty_echo[tty]' in s
assert 'tty_saved_onlcr[tty]=tty_onlcr[tty]' in s
assert 'tty_canonical[dev]=tty_saved_canon[dev]' in s
assert 'tty_echo[dev]=tty_saved_echo[dev]' in s
assert 'tty_onlcr[dev]=tty_saved_onlcr[dev]' in s
assert 'if(tty_restore_pid[tty]==tasks[current].pid) tty_restore_pid[tty]=0;' in s
assert 'if(!tty_canonical[dev])tty_mode_reset(dev);' not in s
print('PASS: foreground TTY save/restore and persistent override wiring')
