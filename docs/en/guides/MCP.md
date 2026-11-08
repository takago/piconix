# Using the MCP server (Pico W)

`mcpd` is an **experimental MCP server** running as a PicoNIX user program on Pico W. It listens for `POST /mcp` on TCP port 8080, independently of `httpd` on port 80.

[日本語](../../ja/guides/MCP.md)

> **Security:** There is no authentication or TLS. `--shell` exposes remote command execution and file writes. Use an isolated test network only; never expose this endpoint to the Internet or a shared LAN. Requests with an `Origin` header are rejected, but this is not authentication.

## 1. Start the server and discover tools

After connecting Pico W to the network, run one of the following:

```sh
mcpd &
# For isolated experiments requiring command execution:
mcpd --shell &
```

| Startup | Exposed MCP tools | Purpose |
|---|---|---|
| `mcpd &` | `get_temperature` | Read onboard temperature |
| `mcpd --shell &` | Above plus `get_manual`, `execute_command`, `write_file` | Manual lookup and remote actions |

`get_manual()` returns the `/man/README` index; supplying `topic` returns one page. `execute_command` is for short **noninteractive** commands. `write_file` overwrites ASCII text at `/tmp/NAME` (up to 2,048 bytes, also subject to the HTTP request limit). The normal mode does not expose command execution or writes.

`mcpd` is not started automatically, and `httpd` can continue running independently.

## 2. Test from Linux (no LLM needed)

Run these commands from the repository root. Replace the IP address with your Pico W address; this example uses AP mode.

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install "mcp>=2,<3"

# List tools, without calling any
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool none

# Read-only tool available in both modes
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool get_temperature
```

With `mcpd --shell &`, use `--tool get_manual` to read the manual index. `--tool all` deliberately calls only the available **`get_temperature` and `get_manual`** tools, never `execute_command` or `write_file` automatically. The `--mode 2026-07-28` option selects the protocol mode explicitly.

## 3. Manual HTTP check

To test the endpoint without the SDK:

```sh
PICO_IP=192.168.4.1
curl -i -X POST "http://$PICO_IP:8080/mcp" \
  -H 'Content-Type: application/json' \
  -H 'Accept: application/json, text/event-stream' \
  -H 'MCP-Protocol-Version: 2026-07-28' \
  -H 'Mcp-Method: server/discover' \
  --data '{"jsonrpc":"2.0","id":1,"method":"server/discover","params":{"_meta":{"io.modelcontextprotocol/protocolVersion":"2026-07-28","io.modelcontextprotocol/clientInfo":{"name":"curl-test","version":"1.0"},"io.modelcontextprotocol/clientCapabilities":{}}}}'
```

`Mcp-Method` must match the JSON `method`. For `tools/call`, `Mcp-Name` must also match `params.name`. See the implementation and extension lab below.

## 4. Current limitations

- Implements part of MCP `2026-07-28`: `server/discover`, `tools/list` and `tools/call`. Interoperability with all MCP clients is not guaranteed.
- HTTP request buffer: 3,600 bytes; maximum JSON body: 3,200 bytes; one client processed at a time.
- `execute_command` accepts up to 119 ASCII bytes of command text and up to 4 KiB of output, sent using HTTP/1.1 chunked transfer.
- No SSE, legacy `initialize`, notifications, or concurrent client processing.
- Do not run interactive or full-screen applications such as `vi`, `edit`, `fd`, `mem`, `watch`, or `pong` with `execute_command`.



## 5. Local LLM and agent integration

### Architecture

```text
User → host/mcp_llm_client.py ←→ OpenAI-compatible LLM server
                 ↓ MCP (TCP 8080)
              Pico W: mcpd
                 ↓
        /dev, /proc, /man, shell
```

The LLM selects tools using Function Calling; the host client forwards calls via MCP and discovers tool schemas through `tools/list`.

### Start the server on Pico W

On a trusted, isolated test network:

```sh
mcpd --shell &
```

This mode exposes `get_temperature`, `get_manual`, `execute_command`, and `write_file`. **There is no authentication or TLS. LLM-selected commands run on Pico W, and the host client does not ask for confirmation before executing tools.** Do not expose the server to shared networks or the Internet. Implement an approval gate in the host client before using it for consequential operations.

### Connect from Linux

Run from the repository root. `192.168.4.1` is an AP-mode example.

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install "mcp>=2,<3" openai

# Verify the MCP connection before involving the LLM
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool none
```

Update these constants at the beginning of `host/mcp_llm_client.py` (`######` denotes a placeholder):

| Setting | Purpose |
|---|---|
| `LLM_URL` | Base URL for the OpenAI-compatible API (usually ending in `/v1`) |
| `LLM_MODEL` | Model name; Function Calling support required |
| `LLM_API_KEY` | LLM server API key |
| `MCP_URL` | Pico W endpoint: `http://IP:8080/mcp` |
| `LLM_REASONING_EFFORT` | Reasoning setting; accepted values depend on the provider/model |
| `SHOW_THINKING` | Toggle visible reasoning when the server returns it |

```sh
python host/mcp_llm_client.py
```

Type `exit` or `quit` to leave. Responses are streamed. With `SHOW_THINKING=False`, the client can still display a brief progress indicator if reasoning data is received. LLM output streaming is separate from chunked MCP HTTP responses.

### Example requests

These are illustrative user prompts; **the tool calls actually chosen by the model are not guaranteed**.

| User prompt | Expected MCP tools |
|---|---|
| “Read the current temperature” | `get_temperature` |
| “Find the PicoNIX IP address” | `execute_command` running `ip` |
| “List current processes” | `execute_command` running `ps` |
| “Show a heart on the LED matrix” | Read instructions via `get_manual`, then run `echo heart > /dev/fbchar` through `execute_command` |

`get_manual()` returns the `/man/README` index; `get_manual({"topic":"languages/c"})` retrieves one selected page. The old `get_wifi_info` and `display_message` tools are no longer exposed.

Use `execute_command` only for short noninteractive commands. Do not run full-screen tools such as `vi`, `edit`, `fd`, `mem`, `watch`, or `pong`. `write_file` overwrites ASCII text at `/tmp/NAME`; take care not to overwrite files unexpectedly.

### Troubleshooting

- **Connection fails:** Verify `mcpd --shell &` on Pico W, the IP address, and port. First test with `host/mcp_client.py ... --tool none`.
- **Only one tool listed:** Normal `mcpd &` mode exposes temperature reading only. Restart in `--shell` mode if appropriate.
- **LLM does not call tools:** Check Chat Completions and Function Calling support; model behavior varies.
- **`reasoning_effort` fails:** Verify the setting is supported by the chosen server/model.

Check the direct MCP client and LLM-integrated client separately to isolate connectivity and model issues.


### Operating guidelines for LLM agents

PicoNIX is not Linux. Do not assume that `bash`, `sudo`, `apt`, `gcc`, `ssh`, or a full POSIX shell exists. Use `get_manual()` to discover available topics, retrieve only the relevant `topic`, or inspect `help` through `execute_command`.

- Prefer short observational commands such as `ps`, `ip`, `cat /proc/net/wifi`, or `cat /dev/adc/temp`. Avoid programs that require interactive input or full-screen terminals.
- Each `execute_command` invocation starts a fresh noninteractive shell. State such as `cd` is not preserved. Commands are limited to 119 printable ASCII bytes, output to 4,096 bytes, and a nominal 3-second timeout. A tool completion result is not necessarily the command's actual exit status.
- `write_file` creates or overwrites a `/tmp/NAME` ASCII file, with a 2,048-byte content limit and a 3,200-byte JSON request-body limit. Append/resume is not supported.
- `execute_command` is **not a sandbox**. Obtain user approval before overwriting files, terminating processes, or changing GPIO/network state. The stock `host/mcp_llm_client.py` **does not implement a pre-execution approval gate**. A system prompt is not a substitute for enforcing approval on the host side.
- Log commands, outputs, errors, and truncation on the host. Do not automatically retry potentially consequential actions after a connection failure or timeout.

Suggested model guidance:

> PicoNIX is a resource-constrained educational UNIX-like OS, not Linux. Check `get_manual` or `help` before guessing commands or options. Request user approval before making changes. Avoid interactive or indefinitely running commands. Distinguish actual tool output from conjecture.

### Using another MCP host, such as Open WebUI

MCP-capable hosts may connect directly to `http://PICO_IP:8080/mcp`, but PicoNIX implements only a limited subset of MCP. **Direct compatibility with any particular Open WebUI version is not guaranteed.** Verify the server first with `host/mcp_client.py` as described above. Configure the endpoint as an MCP tool server rather than an OpenAPI server, using the instructions for your host version; administrator UI labels and transport support can change.

Check the model's tool permissions, start with read-only `get_temperature`, and restrict access to the network and users. In particular, unauthenticated `mcpd --shell` must not be exposed to untrusted networks. Knowledge integration may not inject the operating guidelines into every model invocation, so consequential-action approval must be enforced by the host where needed.

## 6. `mcpd` implementation and extension lab

This lab walks through `userland/apps/mcpd.c`, linking TCP, HTTP, JSON-RPC, MCP and file operations using the existing OS APIs.

### Goals and architecture

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

### Preparation and source map

Use a Pico W, Linux PC, USB console, Pico SDK, and ARM toolchain. Start with [setup](../setup/SETUP.md) and [networking](../reference/network/NETWORK.md). A standard Pico has no Wi-Fi support for this exercise.

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

### Stage A: accept a TCP connection

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

### Stage B: receive an HTTP request

Follow `handle()` into `read_request()`. It accumulates bytes in a static `request[]` array, searches for the `\r\n\r\n` header terminator, parses `Content-Length`, and waits for the specified number of body bytes.

- `REQUEST_SIZE = 3600`: entire request buffer.
- `BODY_LIMIT = 3200`: maximum JSON request body.
- `RESPONSE_SIZE = 1400`: JSON response buffer.
- Large buffers are static instead of local stack variables to preserve the 4 KiB stack.

The accepted descriptor is nonblocking. `read_request()` uses `select()` with `{0, 5000}` (up to 5 ms per wait) and retries when `recv()` returns `OS_EAGAIN`. A retry limit is also present.

**Key lesson:** One `recv()` does **not** necessarily equal one complete HTTP request. TCP may split the headers and body across multiple reads. The prototype handles some such fragmentation but is not a complete HTTP implementation.

**Exercise:** Find the handling of missing `Content-Length`, excessive request size, and methods other than `POST /mcp`.

### Stage C: parse JSON-RPC

Read `process_json()`, `member()`, `json_end()`, `rpc_result()`, and `rpc_error()`. A basic JSON-RPC request looks like:

```json
{"jsonrpc":"2.0","id":1,"method":"tools/list","params":{}}
```

An `id` associates a response with its request. A successful result uses `result`; failures use `error`. **This simplified JSON is not enough for the current prototype:** the server also expects MCP HTTP headers (`MCP-Protocol-Version`, `Mcp-Method`, etc.) and client metadata in `params._meta`. See the 「基本的な使い方」章 for a complete example.

The parser is deliberately small: bounded buffers, limited string handling, and a maximum JSON nesting depth of eight. It is **not a general-purpose, fully compliant JSON parser**.

**Exercise:** Trace responses for a missing `id`, an unknown `method`, and disagreement between the HTTP `Mcp-Method` header and the JSON `method`.

### Stage D: publish MCP tools

The prototype has three methods:

| Method | Function |
|---|---|
| `server/discover` | Report protocol information |
| `tools/list` | Return tool names, descriptions, and input schemas |
| `tools/call` | Invoke a named tool and return its result |

Normal mode exposes only `get_temperature`. `mcpd --shell` also exposes `get_manual`, `execute_command`, and `write_file`:

| Tool | PicoNIX file |
|---|---|
| `get_temperature` | `/dev/adc/temp` |
| `get_manual` | `/man/README` index or a ROM page selected with `topic` (`--shell` only) |

`tools/call` branches on `params.name`. Temperature reads a device; `get_manual` streams a ROM file selected by optional `topic` (default `README`), and `execute_command` streams short command output. Its key operations are ordinary UNIX-like I/O:

```c
int fd = open(path, O_RDONLY);
int n = read(fd, out, cap - 1);
close(fd);
```

*This excerpt omits the checks present in the real function; the full implementation checks errors and NUL-terminates its output.*

The result is JSON-escaped with `append_escaped()` and returned as a `content` array containing `type: "text"`. Device-read failures produce `isError: true`. ROM manuals and command output are returned using HTTP chunked transfer; consult the source for current limits.

**Exercise:** Compare ordinary JSON replies with chunked streaming of a ROM manual page.

### Stage E: build and launch on Pico W

After checking the current `mcpd` application registration, build from the repository root:

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

### Stage F: test from Linux

Start with the full `curl` request in the 「基本的な使い方」章, then test the official Python SDK v2 client:

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install 'mcp>=2,<3'
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool none
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool get_temperature
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool get_manual
```

Replace the IP address with the actual Pico W address. `--tool none` lists tools without calling them. **The client does not use an LLM.** The SDK and device have not been tested together yet, so if a connection fails, record the exact failure and return to the manual HTTP test to isolate protocol issues.

### Stage G: add a tool (design exercise)

Design one more **read-only** tool. A short textual `/proc` file may be suitable, but verify that the path actually exists and check FD usage, output size, and error cases before writing the code.

1. Choose the name, description, and `inputSchema`.
2. Add the tool to the JSON returned by `tools/list`.
3. Add a matching `tools/call` branch.
4. Test failed reads, long output, and JSON escaping.
5. Repeat host-side listing and invocation tests.
6. Rebuild with `-Oz` and check PIC and RAM usage.

Do **not** begin with GPIO-write tools. Authentication, input validation, and restrictions on operations must come first.

### Observation and further work

| What to inspect | Tool | Question |
|---|---|---|
| Running task | `ps` | Does `mcpd` remain running? |
| TCP connections / FDs | `lsof` | Which descriptors exist during a connection? |
| System calls | `trace` | Where are `accept`, `select`, `recv`, `open`, `read` used? |
| RAM layout | `mem`, build layout checks | How are PIC, BSS, and stack fitted into 24 KiB? |
| Error paths | Malformed HTTP / unknown tool | How is failure reported and the connection closed? |

Consult the [trace guide](TRACE.md) and [mem guide](MEM.md) for exact command options.

**Not implemented or not verified:** SSE, legacy `initialize`, complete HTTP/JSON validation, authentication, TLS, concurrent clients, and complete MCP conformance or long-term stability. Basic SDK interoperability was verified on hardware on 2026-11-06. Do not describe this experiment as fully MCP-compliant.

### Takeaway

The central idea is that **a network service can be built on existing UNIX-like APIs without adding a special MCP system call**. Read the source, validate its HTTP exchanges, and measure its RAM usage before extending it.

## Hardware verification update (2026-11-06)

The Pico W `mcpd` server successfully handled `tools/list` and `tools/call` from the official MCP Python SDK v2; an OpenAI-compatible local LLM selected and invoked both tools. Any earlier "not tested on hardware" statements in this prototype tutorial describe its initial stage and are superseded by this result for basic interoperability only. Complete MCP conformance and long-running reliability are not verified. Continue with 「LLMとの連携」章 and `host/mcp_llm_client.py`; LLM connection settings are placeholders.


### Register PicoNIX documentation in Open WebUI

If using Open WebUI Knowledge, upload `docs/en/guides/MCP.md` and separately configure the MCP endpoint `http://PICO_IP:8080/mcp`. Uploading documentation does not enable tool calls, and retrieval is not guaranteed on every turn. Enforce approval for consequential actions in the host application.
