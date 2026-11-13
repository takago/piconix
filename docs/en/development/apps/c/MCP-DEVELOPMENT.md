# `mcpd` implementation and extension

This reference describes the user-space MCP server implementation, its UNIX-like APIs, and extension procedures. See [MCP usage](../../../reference/commands/MCP.md) for operations.

## `mcpd` architecture and extension

`userland/apps/mcpd.c` combines TCP, HTTP, JSON-RPC, MCP, and file operations using existing OS APIs.

### Architecture

```text
Linux PC                       Pico W / PicoNIX
host/mcp_client.py             userland/apps/mcpd.c
      |                                |
      +---- TCP 8080 / POST /mcp ----->+
                                       | Parse HTTP and JSON
                                       | Dispatch MCP methods
                                       +-- /dev/adc/temp
                                       +-- /proc/net/wifi
      <------------ JSON response -----+
```

`mcpd` is a standalone **user-space application**, not a kernel-resident service. It does not replace the separate `httpd` server on TCP port 80.

### Preparation and source map

Use a Pico W, Linux PC, USB console, Pico SDK, and ARM toolchain. Start with [setup](../../../setup/SETUP.md) and [networking](../../../reference/network/NETWORK.md). A standard Pico has no Wi-Fi support for network operations.

| File | Purpose |
|---|---|
| `userland/apps/mcpd.c` | TCP, HTTP, JSON-RPC, MCP dispatch, device-file access |
| `userland/lib/unistd.h` | Socket, `select()`, and I/O API declarations |
| `userland/lib/fcntl.h` | `fcntl()` and `O_NONBLOCK` |
| `CMakeLists.txt` | Application build registration |
| `kernel/main.c` | Embedded image and command registration |
| `host/mcp_client.py` | Linux SDK-based client |
| `docs/en/reference/commands/MCP.md` | Usage and manual HTTP requests |

The current `kernel/main.c` registers the bundled `mcpd` at **program ID 49**. It is already part of the standard Pico W firmware build; no experimental patch or separate application registration is required. Choose another unused ID when adding your own program.

### TCP connection acceptance

The connection path in `main()` uses the following calls:

```c
int server = socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in address = {0};
address.sin_family = AF_INET;
address.sin_port = htons(8080);

bind(server, (const struct sockaddr *)&address, sizeof address);
listen(server, 1);
/* Wait until the listening socket is readable. */
fd_set rd = 0;
FD_SET(server, &rd);
select(server + 1, &rd, 0, 0, 0);
int client = accept(server, 0, 0);
```

*This is an outline only. The full source checks errors, loops, and closes descriptors; keep those checks in real programs.*

`mcpd` waits for readability on the listening socket with `select()` before calling `accept()`. After accepting, the server sets the accepted descriptor to nonblocking using `fcntl(client, F_SETFL, O_NONBLOCK)`, processes one client, and closes it. **Connections are handled sequentially, not concurrently.**


### HTTP request reception

`handle()` invokes `read_request()`, which accumulates bytes in a static `request[]` array, searches for the `\r\n\r\n` header terminator, parses `Content-Length`, and waits for the specified number of body bytes.

- `REQUEST_SIZE = 2800`: entire request buffer.
- `BODY_LIMIT = 2400`: maximum JSON request body.
- `RESPONSE_SIZE = 900`: JSON response buffer.
- Large buffers are static instead of local stack variables to preserve the 4 KiB stack.

The accepted descriptor is nonblocking. `read_request()` uses `select()` for up to 5 ms per wait and retries when `recv()` returns `OS_EAGAIN`. The write-readiness wait uses 2 ms; the `execute_command` output pipe uses 10 ms. These are per-wait bounds, with separate retry and command-duration limits. A retry limit is also present.

**TCP framing:** One `recv()` does **not** necessarily equal one complete HTTP request. TCP may split the headers and body across multiple reads. The server accumulates fragmented headers and bodies within fixed buffer and retry limits; it is not a general HTTP implementation.


### JSON-RPC parsing

`process_json()`, `member()`, `json_end()`, `rpc_result()`, and `rpc_error()` implement parsing and responses. A basic JSON-RPC request looks like:

```json
{"jsonrpc":"2.0","id":1,"method":"tools/list","params":{}}
```

An `id` associates a response with its request. A successful result uses `result`; failures use `error`. **This JSON body alone is not a complete MCP request:** the server also expects MCP HTTP headers (`MCP-Protocol-Version`, `Mcp-Method`, etc.) and client metadata in `params._meta`. See the [MCP reference](../../../reference/commands/MCP.md) for the required HTTP request format.

The parser is deliberately small: bounded buffers, limited string handling, and a maximum JSON nesting depth of eight. It is **not a general-purpose, fully compliant JSON parser**.


### MCP tool publication

The MCP endpoint supports these methods:

| Method | Function |
|---|---|
| `server/discover` | Report protocol information |
| `tools/list` | Return tool names, descriptions, and input schemas |
| `tools/call` | Invoke a named tool and return its result |

`mcpd` always exposes `get_temperature`, `get_manual`, `execute_command`, `write_file`, and the PTY tools. There is no authentication or TLS:

| Tool | PicoNIX file |
|---|---|
| `get_temperature` | `/dev/adc/temp` |
| `get_manual` | `/man/README` index or a ROM page selected with `topic` |

`tools/call` branches on `params.name`. Temperature reads a device; `get_manual` streams a ROM file selected by optional `topic` (default `README`), and `execute_command` streams short command output. Its key operations are ordinary UNIX-like I/O:

```c
int fd = open(path, O_RDONLY);
int n = read(fd, out, cap - 1);
close(fd);
```

*This excerpt omits the checks present in the real function; the full implementation checks errors and NUL-terminates its output.*

The result is JSON-escaped with `append_escaped()` and returned as a `content` array containing `type: "text"`. Device-read failures produce `isError: true`. ROM manuals and command output are returned using HTTP chunked transfer; consult the source for current limits.


### Pico W build and launch

After checking the current `mcpd` application registration, build from the repository root:

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.3.1" \
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

Check that the `-Oz` PIC application fits a 24 KiB user slot, including its reserved 4 KiB stack. The actual build output is the authority for PIC, flash image, and RAM-slot capacity; check it for each build.

Flash the updated UF2, connect to Wi-Fi, and try:

```sh
ls /bin
mcpd &
ps
```

If `mcpd` is absent, inspect the app list in `CMakeLists.txt`, image declaration and registration in `kernel/main.c`, and the actual UF2 version. Check that TCP port 8080 is not already occupied.

### Linux host communication

Start with the full `curl` request in the [MCP reference](../../../reference/commands/MCP.md), then test the official Python SDK v2 client:

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install 'mcp>=2,<3'
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool none
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool get_temperature
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool get_manual
```

Replace the IP address with the actual Pico W address. `--tool none` lists tools without calling them. **The client does not use an LLM.** The SDK and device have been checked for basic interoperability. A failed connection should be diagnosed with the manual HTTP request and the exact error output; complete protocol conformance is not established.

### Adding a read-only tool

When adding a **read-only** tool, a short textual `/proc` file may be suitable, but verify that the path actually exists and check FD usage, output size, and error cases before writing the code.

1. Choose the name, description, and `inputSchema`.
2. Add the tool to the JSON returned by `tools/list`.
3. Add a matching `tools/call` branch.
4. Test failed reads, long output, and JSON escaping.
5. Repeat host-side listing and invocation tests.
6. Rebuild with `-Oz` and check PIC and RAM usage.

Do **not** begin with GPIO-write tools. Authentication, input validation, and restrictions on operations must come first.

### Diagnostics

| Component | Tool | Diagnostic detail |
|---|---|---|
| Running task | `ps` | Confirm that `mcpd` is running in the background |
| TCP connections / FDs | `lsof` | Compare open FDs during and after the connection |
| System calls | `trace` | Observe `accept`, `select`, `recv`, `open`, `read` |
| RAM layout | `mem`, build layout checks | Check PIC, BSS, stack and the 24 KiB slot limit |
| Error paths | Malformed HTTP / unknown tool | Check errors and connection cleanup |

Consult the [trace guide](../../../reference/commands/TRACE.md) and [mem guide](../../../reference/commands/MEM.md) for exact command options.

**Not implemented or not verified:** SSE, `initialize`, complete HTTP/JSON validation, authentication, TLS, concurrent clients, and complete MCP conformance or long-term stability. Basic SDK interoperability has been verified on hardware. Do not describe this experiment as fully MCP-compliant.

### Implementation summary

The implementation demonstrates that **a network service can be built on existing UNIX-like APIs without adding a special MCP system call**. Read the source, validate its HTTP exchanges, and measure its RAM usage before extending it.


### HTTP writes and TCP close

`send_chunk()` packs chunks up to 768 bytes (hex length, payload, CRLF) into a single `send_all()` call to reduce the number of tiny `tcp_write()` operations consuming the fixed send queue. `send_all()` handles partial writes and `OS_EAGAIN`/zero-length writes, and records a failure after roughly three seconds without progress. It also checks the final `0\r\n\r\n` terminator. The app-level successful write is not proof that the peer received the response.

The socket is released by `net_socket_tcp_close_client()` in `kernel/net_socket.c`; if `tcp_close()` must be deferred, the TCP poll callback retries. This is distinct from application-level HTTP completion. Correlate `/proc/net/memory` counters, `trace` syscalls, the `mcpd` console, and `host/mcp_stress.py`. Long-running behavior on actual hardware remains unverified.
