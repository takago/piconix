"""Static regression checks for nc/dd streaming integration."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
read = lambda f: (root / f).read_text()
dd=read("userland/dd.c")
nc=read("userland/nc.c")
assert '(in && strcmp(in,"-"))' in dd
assert '(out && strcmp(out,"-"))' in dd
assert 'shutdown(peer,SHUT_WR)' in nc
assert 'stdin_finished=1' in nc
assert 'tcp_shutdown(s->client,0,1)' in read("kernel/net_socket.c")
assert 'SYS_SHUTDOWN_TCP' in read("kernel/kernel.c")
assert 'SYS_SHUTDOWN_TCP' in read("include/piconix_syscall.h")
print("PASS: dd standard streams, nc TCP send half-close wiring")
