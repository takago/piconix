# Building the `mcpd` MCP server — A C user-application lab

*Advanced Pico W lab: reading and extending an experimental implementation*

This tutorial uses the PicoNIX user-space `mcpd` server to connect **TCP → HTTP → JSON-RPC → MCP tools → device files**. A Linux host client (`host/mcp_client.py`) tests the server without an LLM.

**Scope and validation:** The tutorial describes the experimental `userland/apps/mcpd.c`. It implements only part of the MCP 2026-07-28-style exchange. **Tool listing and calls with the official MCP Python SDK v2, including LLM tool selection, were verified on Pico W hardware on 2026-11-06.** A fresh production-image RAM measurement and long-term reliability checks remain unverified. Commands below are exercises and intended checks, not a record of successful hardware tests.

**Security:** The experiment has no TLS or authentication. It rejects requests containing an `Origin` header, but this is not authentication. Use it **only on an isolated test network**, never on the public Internet. Do not add write-capable tools before designing access controls and validation.

[日本語](../../../../ja/development/apps/c/MCP-SERVER-DEVELOPMENT.md) | [MCP usage guide](../../../guides/MCP.md) | [C programming](PROGRAMMING.md) | [Socket programming](NETWORK-PROGRAMMING.md)

## 1. Goals and architecture

By the end of this lab, you should understand:

1. Why `socket()`, `bind()`, `listen()`, and `accept()` are used by a TCP server.
2. How to extract HTTP headers and a message body from a TCP byte stream.
3. How JSON-RPC uses the `method` and `id` fields.
4. How `tools/list` and `tools/call` expose ordinary device-file reads.
5. What PIC, a 24 KiB task slot, and a reserved 4 KiB stack mean in PicoNIX.

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

## 2. Preparation and source map

Use a Pico W, Linux PC, USB console, Pico SDK, and ARM toolchain. Start with [setup](../../../setup/SETUP.md) and [networking](../../../reference/network/NETWORK.md). A standard Pico has no Wi-Fi support for this exercise.

| File | Purpose |
|---|---|
| `userland/apps/mcpd.c` | TCP, HTTP, JSON-RPC, MCP dispatch, device-file access |
| `userland/lib/unistd.h` | Socket, `select()`, and I/O API declarations |
| `userland/lib/fcntl.h` | `fcntl()` and `O_NONBLOCK` |
| `CMakeLists.txt` | Application build registration |
| `kernel/main.c` | Embedded image and command registration |
| `host/mcp_client.py` | Linux SDK-based client |
| `docs/en/guides/MCP.md` | Usage and manual HTTP requests |

The experimental patch registers `mcpd` using **program ID 49**. Always check the current `register_image()` list before adding more applications, to avoid duplicate IDs.

## 3. Stage A: accept a TCP connection

Read `main()` at the bottom of `mcpd.c`. The essential calls are:

```c
int server = socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in address = {0};
address.sin_family = AF_INET;
address.sin_port = htons(8080);

bind(server, (const struct sockaddr *)&address, sizeof address);
listen(server, 1);
int client = accept(server, 0, 0);
```

*This is an outline only. The full source checks errors, loops, and closes descriptors; keep those checks in real programs.*

By default, `accept()` waits for a connection. After accepting, the prototype sets the accepted descriptor to nonblocking using `fcntl(client, F_SETFL, O_NONBLOCK)`, processes one client, and closes it. **Connections are handled sequentially, not concurrently.**

**Exercise:** Find the failure paths for `socket()` and `bind()`, and locate every corresponding `close()`.

## 4. Stage B: receive an HTTP request

Follow `handle()` into `read_request()`. It accumulates bytes in a static `request[]` array, searches for the `\r\n\r\n` header terminator, parses `Content-Length`, and waits for the specified number of body bytes.

- `REQUEST_SIZE = 3072`: entire request buffer.
- `BODY_LIMIT = 1536`: maximum JSON request body.
- `RESPONSE_SIZE = 1400`: JSON response buffer.
- Large buffers are static instead of local stack variables to preserve the 4 KiB stack.

The accepted descriptor is nonblocking. `read_request()` uses `select()` with `{0, 5000}` (up to 5 ms per wait) and retries when `recv()` returns `OS_EAGAIN`. A retry limit is also present.

**Key lesson:** One `recv()` does **not** necessarily equal one complete HTTP request. TCP may split the headers and body across multiple reads. The prototype handles some such fragmentation but is not a complete HTTP implementation.

**Exercise:** Find the handling of missing `Content-Length`, excessive request size, and methods other than `POST /mcp`.

## 5. Stage C: parse JSON-RPC

Read `process_json()`, `member()`, `json_end()`, `rpc_result()`, and `rpc_error()`. A basic JSON-RPC request looks like:

```json
{"jsonrpc":"2.0","id":1,"method":"tools/list","params":{}}
```

An `id` associates a response with its request. A successful result uses `result`; failures use `error`. **This simplified JSON is not enough for the current prototype:** the server also expects MCP HTTP headers (`MCP-Protocol-Version`, `Mcp-Method`, etc.) and client metadata in `params._meta`. See the [MCP usage guide](../../../guides/MCP.md) for a complete example.

The parser is deliberately small: bounded buffers, limited string handling, and a maximum JSON nesting depth of eight. It is **not a general-purpose, fully compliant JSON parser**.

**Exercise:** Trace responses for a missing `id`, an unknown `method`, and disagreement between the HTTP `Mcp-Method` header and the JSON `method`.

## 6. Stage D: publish MCP tools

The prototype has three methods:

| Method | Function |
|---|---|
| `server/discover` | Report protocol information |
| `tools/list` | Return tool names, descriptions, and input schemas |
| `tools/call` | Invoke a named tool and return its result |

The initial tool set is read-only:

| Tool | PicoNIX file |
|---|---|
| `get_temperature` | `/dev/adc/temp` |
| `get_wifi_info` | `/proc/net/wifi` |

`tools/call` selects a file path from `params.name` and invokes `read_device()`. Its key operations are ordinary UNIX-like I/O:

```c
int fd = open(path, O_RDONLY);
int n = read(fd, out, cap - 1);
close(fd);
```

*This excerpt omits the checks present in the real function; the full implementation checks errors and NUL-terminates its output.*

The result is JSON-escaped with `append_escaped()` and returned as a `content` array containing `type: "text"`. Device-read failures produce `isError: true`. **Wi-Fi information may be truncated**, since the prototype reads at most 255 bytes.

**Exercise:** Identify the logic shared by both tools and the small piece that changes per tool.

## 7. Stage E: build and launch on Pico W

After applying the `mcpd` experimental patch and checking application registration, build from the repository root:

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

Check that the `-Oz` PIC application fits a 24 KiB user slot, including its reserved 4 KiB stack. A preliminary link with replacement EABI helpers suggested it might fit, **but does not establish that a normal build will pass**.

Flash the updated UF2, connect to Wi-Fi, and try:

```sh
ls /bin
mcpd &
ps
```

If `mcpd` is absent, inspect the app list in `CMakeLists.txt`, image declaration and registration in `kernel/main.c`, and the actual UF2 version. Check that TCP port 8080 is not already occupied.

## 8. Stage F: test from Linux

Start with the full `curl` request in the [MCP usage guide](../../../guides/MCP.md), then test the official Python SDK v2 client:

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install 'mcp>=2,<3'
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool none
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool get_temperature
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool get_wifi_info
```

Replace the IP address with the actual Pico W address. `--tool none` lists tools without calling them. **The client does not use an LLM.** The SDK and device have not been tested together yet, so if a connection fails, record the exact failure and return to the manual HTTP test to isolate protocol issues.

## 9. Stage G: add a tool (design exercise)

Design one more **read-only** tool. A short textual `/proc` file may be suitable, but verify that the path actually exists and check FD usage, output size, and error cases before writing the code.

1. Choose the name, description, and `inputSchema`.
2. Add the tool to the JSON returned by `tools/list`.
3. Add a matching `tools/call` branch.
4. Test failed reads, long output, and JSON escaping.
5. Repeat host-side listing and invocation tests.
6. Rebuild with `-Oz` and check PIC and RAM usage.

Do **not** begin with GPIO-write tools. Authentication, input validation, and restrictions on operations must come first.

## 10. Observation and further work

| What to inspect | Tool | Question |
|---|---|---|
| Running task | `ps` | Does `mcpd` remain running? |
| TCP connections / FDs | `lsof` | Which descriptors exist during a connection? |
| System calls | `trace` | Where are `accept`, `select`, `recv`, `open`, `read` used? |
| RAM layout | `mem`, build layout checks | How are PIC, BSS, and stack fitted into 24 KiB? |
| Error paths | Malformed HTTP / unknown tool | How is failure reported and the connection closed? |

Consult the [trace guide](../../../guides/TRACE.md) and [mem guide](../../../guides/MEM.md) for exact command options.

**Not implemented or not verified:** SSE, legacy `initialize`, complete HTTP/JSON validation, authentication, TLS, concurrent clients, and complete MCP conformance or long-term stability. Basic SDK interoperability was verified on hardware on 2026-11-06. Do not describe this experiment as fully MCP-compliant.

### Takeaway

The central idea is that **a network service can be built on existing UNIX-like APIs without adding a special MCP system call**. Read the source, validate its HTTP exchanges, and measure its RAM usage before extending it.

## Hardware verification update (2026-11-06)

The Pico W `mcpd` server successfully handled `tools/list` and `tools/call` from the official MCP Python SDK v2; an OpenAI-compatible local LLM selected and invoked both tools. Any earlier "not tested on hardware" statements in this prototype tutorial describe its initial stage and are superseded by this result for basic interoperability only. Complete MCP conformance and long-running reliability are not verified. Continue with [the LLM client tutorial](../../../guides/MCP-LLM-CLIENT.md) and `host/mcp_llm_client.py`; LLM connection settings are placeholders.
