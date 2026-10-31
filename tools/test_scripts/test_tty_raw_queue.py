from pathlib import Path
s = (Path(__file__).resolve().parents[2] / 'kernel/kernel.c').read_text()
a = s.index('static void tty_raw_enqueue(')
b = s.index('static void tty_enqueue(', a)
body = s[a:b]
assert 't->lines[last].len++' in body
assert 't->used==TTY_LINES' in body
assert 't->offset==0' in body
assert 'sizeof(t->lines[last].text)' in body
print('PASS: raw TTY bytes are packed into line slots')
