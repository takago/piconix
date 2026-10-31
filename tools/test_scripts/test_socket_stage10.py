from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
net = (ROOT / "kernel/net_socket.c").read_text()
kernel = (ROOT / "kernel/kernel.c").read_text()
assert "udp_bind(pcb, IP_ANY_TYPE, 0)" in net, "UDP client must get an ephemeral local port"
assert "udp_recv(pcb, udp_received, NULL)" in net, "UDP client must receive replies"
assert "in_task_range(pkt,sizeof(*pkt))" in kernel, "Packet descriptors must be validated"
assert "in_task_range(pkt->data,pkt->length)" in kernel, "User payload pointers must be validated"
assert "writing ? OS_EIO : 0" in kernel, "TCP write after EOF must not report success"
print("PASS: stage10 socket invariants")
