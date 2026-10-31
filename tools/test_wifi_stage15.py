from pathlib import Path
s=(Path(__file__).resolve().parents[1]/"kernel/kernel.c").read_text()
assert "#define OS_TTY_COUNT 3" in s
assert "#define OS_TTY_COUNT 2" in s
for key in ("foreground_pid", "tty_shell_pid", "tty_buf", "tty_claim_pid", "tty_mode_owner"):
    assert key+"[OS_TTY_COUNT]" in s, key
assert 't->tty==2?"wifi"' in s
print("PASS: third TTY state groundwork")
