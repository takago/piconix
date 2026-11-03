#!/usr/bin/env python3
"""Guard the current bounded HTTP parser and lwIP deferred-close behavior."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
http = (root/'userland/apps/httpd.c').read_text()
net = (root/'kernel/net_socket.c').read_text()
assert 'static char req[2048]' in http
assert "req[i-3]=='\\r'&&req[i-2]=='\\n'&&req[i-1]=='\\r'&&req[i]=='\\n'" in http
assert 'if(!header&&used==sizeof req-1)' in http
assert 'if(post&&!got_length)' in http
assert 'if(n>32)return -2' in http
assert 'int length_status=content_length(' in http
assert 'if(n==0){text_reply(fd,400,"Incomplete headers\\n");return;}' in http
assert 'if(count!=length)' in http
for path in ('/api/fb','/api/tone','/api/pwm/freq','/api/pwm/a','/api/pwm/b',
             '/index.html','/fbcheck.html','/pwm.html'):
    assert path in http,path
close=net.split('void net_socket_tcp_close_client(int id) {',1)[1].split('int net_socket_tcp_connected(',1)[0]
assert 'tcp_recv(pcb,socket_tcp_closing_recv)' in close
assert 'tcp_poll(pcb,socket_tcp_closing_poll,2)' in close
assert 'tcp_sent(pcb,NULL)' in close
assert 'tcp_abort(pcb)' not in close
assert 'tcp_close(pcb)' in close
print('PASS: bounded HTTP parser, current Web routes and deferred TCP close')
