# Experimental MCP Streamable HTTP server (`mcpd`)

`mcpd` is an experimental user-space MCP server for PicoNIX on Pico W. It listens on TCP port 8080 and accepts `POST /mcp`, independently of the existing `httpd` on port 80. No kernel behavior changes are needed.

**Experimental and unauthenticated. Do not expose this server to the Internet.** TLS and access control are not provided. Every request containing an `Origin` header is rejected. Use a trusted, isolated LAN for command-line tests only.

## Scope

- Partial support for MCP `2026-07-28` Streamable HTTP using JSON-RPC over HTTP POST with JSON responses.
- Implements `server/discover`, `tools/list`, and `tools/call`.
- Tools: `get_temperature` reads `/dev/adc/temp`; `get_wifi_info` reads `/proc/net/wifi` (up to the first 255 bytes).
- No SSE, legacy `initialize`, authentication, TLS, notifications, concurrent client handling, or complete general-purpose HTTP/JSON validation. Basic tool listing and calls were verified with the official MCP Python SDK v2 on Pico W hardware.
- Fixed HTTP request buffer of 3072 bytes, JSON body limit of 1536 bytes, and one client at a time.

## Start on Pico W

```sh
mcpd &
```

The server is not added to the default startup script. The existing `httpd` remains unchanged.

## Test from a PC

Replace `PICO_IP` with the device address.

```sh
PICO_IP=192.168.1.50
curl -i -X POST "http://$PICO_IP:8080/mcp" \
  -H 'Content-Type: application/json' \
  -H 'Accept: application/json, text/event-stream' \
  -H 'MCP-Protocol-Version: 2026-07-28' \
  -H 'Mcp-Method: server/discover' \
  --data '{"jsonrpc":"2.0","id":1,"method":"server/discover","params":{"_meta":{"io.modelcontextprotocol/protocolVersion":"2026-07-28","io.modelcontextprotocol/clientInfo":{"name":"curl-test","version":"1.0"},"io.modelcontextprotocol/clientCapabilities":{}}}}'
```

The `Mcp-Method` header must match the JSON `method`. A `tools/call` request also needs a matching `Mcp-Name` header and `params.name` (for example `get_temperature`). Passing this simple HTTP test does not imply general MCP-client interoperability.

## Python client on Linux

`host/mcp_client.py` is a direct test client based on the official MCP Python SDK v2. It uses no LLM, and lists and invokes the two exposed tools.

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install "mcp>=2,<3"
python host/mcp_client.py http://192.168.1.50:8080/mcp
```

Replace the address with the Pico W IP. Without a URL argument, the default is `http://192.168.4.1:8080/mcp`.

```sh
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool none
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool get_temperature
python host/mcp_client.py http://192.168.1.50:8080/mcp --mode 2026-07-28
```

`--tool none` only lists tools; `--mode 2026-07-28` skips the SDK's default `server/discover` probe. Interoperability for tool listing and calls was **verified on Pico W hardware on 2026-11-06**. Use this unauthenticated HTTP endpoint only on an isolated LAN.

## Integration and verification

- Integrated in `CMakeLists.txt`, `kernel/main.c`, and `userland/apps/help.c`, with program ID 49.
- Uses the existing `-Oz` user application settings and 24 KiB task slot with 4 KiB reserved stack.
- ARM Cortex-M0+ syntax check and host JSON-RPC response tests passed.
- A provisional ARM link with test-only replacement EABI helper functions produced a 7704-byte PIC image, with 4776 bytes of BSS, fitting the 24 KiB slot. **These are provisional size measurements from the prototype. MCP communication was subsequently verified on Pico W hardware, but a fresh production-image size measurement and long-running reliability test are not established here.**

## Local LLM integration

See [Calling MCP tools from an LLM](MCP-LLM-CLIENT.md) and `host/mcp_llm_client.py`. A local LLM successfully selected tools and retrieved Pico W temperature and Wi-Fi information on 2026-11-06. The LLM server address, model and API key remain placeholders.
