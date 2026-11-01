from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
http = (root/'userland/httpd.c').read_text()
net = (root/'kernel/net_socket.c').read_text()
assert 'read_request(fd, request, sizeof(request))' in http
assert 'terminator[] = "\\r\\n\\r\\n"' in http
assert re.search(r'tcp_poll\s*\(\s*pcb\s*,\s*socket_tcp_closing_poll\s*,\s*2\s*\)', net)
assert re.search(r'tcp_sent\s*\(\s*pcb\s*,\s*socket_tcp_closing_sent\s*\)', net)
assert 'if (tcp_close(pcb) != ERR_OK) tcp_abort(pcb);' not in net[net.index('void net_socket_tcp_close_client(int id) {'):net.index('int net_socket_tcp_connected(int id)')]
print('PASS: HTTP header drain and graceful TCP close retry')
