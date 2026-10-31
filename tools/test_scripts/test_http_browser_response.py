from pathlib import Path
root = Path(__file__).resolve().parents[2]
http = (root/'userland/httpd.c').read_text()
net = (root/'kernel/net_socket.c').read_text()
assert 'read_request(fd, request, sizeof(request))' in http
assert 'terminator[] = "\\r\\n\\r\\n"' in http
assert 'tcp_poll(pcb, socket_tcp_closing_poll, 2)' in net
assert 'tcp_sent(pcb, socket_tcp_closing_sent)' in net
assert 'if (tcp_close(pcb) != ERR_OK) tcp_abort(pcb);' not in net[net.index('void net_socket_tcp_close_client(void) {'):net.index('int net_socket_tcp_connected(void)')]
print('PASS: HTTP header drain and graceful TCP close retry')
