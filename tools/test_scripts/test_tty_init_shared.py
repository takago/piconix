#!/usr/bin/env python3
"""Keep init-time TTY defaults aligned with PTY teardown defaults."""
from pathlib import Path
import re
s = (Path(__file__).resolve().parents[2] / "kernel/kernel.c").read_text()
reset = re.search(r"NO_TRACE static void tty_mode_reset\(int dev\) \{(.*?)\n\}", s, re.S)
init = re.search(r"NO_TRACE void os_init\(void\) \{(.*?)\n    if \(!stdio_usb_init", s, re.S)
assert reset and init
for field in ("tty_canonical", "tty_echo", "tty_onlcr", "tty_interrupt", "tty_last_cr", "tty_mode_owner", "tty_buf"):
    assert field in reset.group(1), field
assert "for(int tty=0;tty<OS_TTY_COUNT;tty++) tty_mode_reset(tty);" in init.group(1)
assert "memset(tty_claim_pid,0,sizeof tty_claim_pid);" in init.group(1)
assert "memset(tty_restore_pid,0,sizeof tty_restore_pid);" in init.group(1)
assert "memset(tty_saved_owner,0,sizeof tty_saved_owner);" in init.group(1)
print("PASS: TTY init reuses the reset function without losing owner cleanup")
