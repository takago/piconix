from pathlib import Path
s = (Path(__file__).resolve().parents[2] / 'kernel/kernel.c').read_text()
mode = s[s.index('if(cmd==OS_TTY_SET_CANON &&'):s.index('if(cmd==OS_TTY_SET_ECHO)')]
assert 'memset(&tty_buf' not in mode
restore = s[s.index('} else if(tty_restore_pid[dev]==t->pid)'):s.index('} else if(tty_mode_owner[dev]==t->pid)')]
assert 'memset(&tty_buf' not in restore
assert 'memset(&tty_buf[dev],0,sizeof tty_buf[dev]);' in s[s.index('static void tty_mode_reset(int dev) {'):s.index('static void tty_raw_enqueue')]
print('PASS: TTY mode transitions preserve queued input; explicit reset still flushes')
