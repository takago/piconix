#!/usr/bin/env python3
"""Verify the stdlib MCP stress client detects a missing final HTTP chunk."""
import importlib.util
import json
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
import threading
import http.client

root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('mcp_stress', root / 'host/mcp_stress.py')
client = importlib.util.module_from_spec(spec)
spec.loader.exec_module(client)


class Handler(BaseHTTPRequestHandler):
    """Minimal HTTP transport fixture with valid and deliberately broken replies."""
    protocol_version = 'HTTP/1.1'
    broken = False

    def do_POST(self):
        """Consume one JSON request and send a complete or truncated response."""
        assert self.path == '/mcp'
        body = self.rfile.read(int(self.headers['Content-Length']))
        req = json.loads(body)
        assert req['method'] == 'tools/call'
        assert req['params']['name'] == 'get_temperature'
        answer = json.dumps({'jsonrpc': '2.0', 'id': req['id'], 'result': {'content': []}}).encode()
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Transfer-Encoding', 'chunked')
        self.send_header('Connection', 'close')
        self.end_headers()
        self.wfile.write(f'{len(answer):x}\r\n'.encode() + answer + b'\r\n')
        if not self.broken:
            self.wfile.write(b'0\r\n\r\n')
        self.wfile.flush()
        self.close_connection = True

    def log_message(self, *args):
        """Keep fixture output deterministic."""
        pass


server = HTTPServer(('127.0.0.1', 0), Handler)
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()
try:
    url = f'http://127.0.0.1:{server.server_port}/mcp'
    length, mode = client.send_request(url, 1, 'get_temperature', '', 2)
    assert length and mode == 'chunked'
    Handler.broken = True
    try:
        client.send_request(url, 2, 'get_temperature', '', 2)
    except (http.client.IncompleteRead, http.client.HTTPException, ConnectionError):
        pass
    else:
        raise AssertionError('Truncated chunked HTTP body was silently accepted')
finally:
    server.shutdown()
    server.server_close()
    thread.join(timeout=2)
print('PASS: MCP host stress probe detects incomplete final HTTP chunk')
