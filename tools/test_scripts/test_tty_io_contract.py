#!/usr/bin/env python3
"""Guard the documented TTY I/O semantics against implementation drift."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "kernel/kernel.c").read_text()
start = source.index("static int device_rw_tty(")
end = source.index("static int device_rw_pwm(", start)
body = source[start:end]
assert "if(n==0) return 0;" in body
assert "if(tasks[current].fd_flags[fd] & OS_O_NONBLOCK)" in body
assert "restore_interrupts(irq_state); return 0;" in body
assert "t->state=BLOCKED;" in body
assert "return count; /* 0バイトの入力はCtrl+DによるEOFの可能性がある． */" in body
assert "if(!tty_tx_char(uartdev,c,insert_cr)) break;" in body
assert "net_terminal_connected() ? OS_EAGAIN : OS_EIO" in body
assert "return (int)i;" in body
assert "if(CONSOLE_TX_CAP-1u-used < (unsigned)(1+insert_cr)) return 0;" in source
assert "if(dev==2) {" in source and "net_terminal_write(bytes+(!insert_cr),1+insert_cr)" in source
assert "VMIN" not in (root / "include/piconix.h").read_text()
for language in ("ja", "en"):
    tty = (root / f"docs/{language}/guides/TTY.md").read_text()
    syscalls = (root / f"docs/{language}/reference/SYSCALLS.md").read_text()
    for term in ("O_NONBLOCK", "VMIN", "VTIME", "OS_EAGAIN", "OS_EIO", "Wi-Fi TTY", "ONLCR"):
        assert term in tty, (language, term)
    assert "../guides/TTY.md" in syscalls
print("PASS: TTY read/write, nonblocking, and output-queue documentation")
