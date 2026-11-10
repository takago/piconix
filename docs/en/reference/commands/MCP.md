# Using the MCP server (Pico W)

`mcpd` is an **experimental MCP server** running as a PicoNIX user program on Pico W. It listens for `POST /mcp` on TCP port 8080, independently of `httpd` on port 80.

[日本語](../../../ja/reference/commands/MCP.md)

> **Security:** There is no authentication or TLS. Starting `mcpd` exposes remote command execution and file writes. Use an isolated test network only; never expose this endpoint to the Internet or a shared LAN. Requests with an `Origin` header are rejected, but this is not authentication.

## 1. Start the server and discover tools

After connecting Pico W to the network, start the server:

```sh
mcpd &
```

`mcpd` always exposes eight tools: `get_temperature`, `get_manual`, `write_file`, `execute_command`, `terminal_open`, `terminal_send`, `terminal_read`, and `terminal_close`. The `--shell` option has been removed. **There is no authentication or TLS: use only an isolated trusted test network.**

`get_manual({})` or `get_manual({"topic":"index"})` returns the `/man/README` topic index; use an exact topic listed there to retrieve a page. `execute_command` is for short **noninteractive** commands. `write_file` overwrites ASCII text at `/tmp/NAME` (up to 2,048 bytes, also subject to the HTTP request limit). Command execution and writes are always available.

`mcpd` is not started automatically, and `httpd` can continue running independently.

## 2. Test from Linux (no LLM needed)

Run these commands from the repository root. Replace the IP address with your Pico W address; this example uses AP mode.

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install "mcp>=2,<3"

# List tools, without calling any
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool none

# Read-only temperature tool
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool get_temperature
```

With `mcpd &`, use `--tool get_manual` to read the manual index. `--tool all` deliberately calls only the available **`get_temperature` and `get_manual`** tools, never `execute_command` or `write_file` automatically. The `--mode 2026-07-28` option selects the protocol mode explicitly.

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
- HTTP request buffer: 2,800 bytes; maximum JSON body: 2,400 bytes; one client processed at a time.
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
mcpd &
```

This mode exposes `get_temperature`, `get_manual`, `execute_command`, `write_file`, and four `terminal_*` tools. **There is no authentication or TLS. LLM-selected commands run on Pico W, and the host client does not ask for confirmation before executing tools.** Do not expose the server to shared networks or the Internet. Implement an approval gate in the host client before using it for consequential operations.

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

- **Connection fails:** Verify `mcpd &` on Pico W, the IP address, and port. First test with `host/mcp_client.py ... --tool none`.
- **Only one tool listed:** Rebuild and restart the current `mcpd` and inspect `tools/list`.
- **LLM does not call tools:** Check Chat Completions and Function Calling support; model behavior varies.
- **`reasoning_effort` fails:** Verify the setting is supported by the chosen server/model.

Check the direct MCP client and LLM-integrated client separately to isolate connectivity and model issues.


### Operating guidelines for LLM agents

PicoNIX is not Linux. Do not assume that `bash`, `sudo`, `apt`, `gcc`, `ssh`, or a full POSIX shell exists. Use `get_manual()` to discover available topics, retrieve only the relevant `topic`, or inspect `help` through `execute_command`.

- Prefer short observational commands such as `ps`, `ip`, `cat /proc/net/wifi`, or `cat /dev/adc/temp`. Avoid programs that require interactive input or full-screen terminals.
- Each `execute_command` invocation starts a fresh noninteractive shell. State such as `cd` is not preserved. Commands are limited to 119 printable ASCII bytes, output to 4,096 bytes, and a nominal 3-second timeout. `sh -c` returns the last command's exit code (or the final pipeline stage); a nonzero value yields MCP `isError:true` and an exit-code message. HTTP completion alone does not imply command success.
- `write_file` creates or overwrites a `/tmp/NAME` ASCII file, with a 2,048-byte content limit and a 3,200-byte JSON request-body limit. Append/resume is not supported.
- `execute_command` is **not a sandbox**. Obtain user approval before overwriting files, terminating processes, or changing GPIO/network state. The stock `host/mcp_llm_client.py` **does not implement a pre-execution approval gate**. A system prompt is not a substitute for enforcing approval on the host side.
- Log commands, outputs, errors, and truncation on the host. Do not automatically retry potentially consequential actions after a connection failure or timeout.

Suggested model guidance:

> PicoNIX is a resource-constrained educational UNIX-like OS, not Linux. Check `get_manual` or `help` before guessing commands or options. Request user approval before making changes. Avoid interactive or indefinitely running commands. Distinguish actual tool output from conjecture.

### Using another MCP host, such as Open WebUI

MCP-capable hosts may connect directly to `http://PICO_IP:8080/mcp`, but PicoNIX implements only a limited subset of MCP. **Direct compatibility with any particular Open WebUI version is not guaranteed.** Verify the server first with `host/mcp_client.py` as described above. Configure the endpoint as an MCP tool server rather than an OpenAPI server, using the instructions for your host version; administrator UI labels and transport support can change.

Check the model's tool permissions, start with read-only `get_temperature`, and restrict access to the network and users. In particular, unauthenticated `mcpd` must not be exposed to untrusted networks. Knowledge integration may not inject the operating guidelines into every model invocation, so consequential-action approval must be enforced by the host where needed.


## 6. Read and extend the implementation

The TCP, HTTP, JSON-RPC and development lab now live in the [MCP server development tutorial](../../development/apps/c/MCP-DEVELOPMENT.md). This guide describes operation; the development tutorial traces the source.

## Hardware verification update (2026-11-06)

The Pico W `mcpd` server successfully handled `tools/list` and `tools/call` from the official MCP Python SDK v2; an OpenAI-compatible local LLM selected and invoked both tools. Any earlier "not tested on hardware" statements in this prototype tutorial describe its initial stage and are superseded by this result for basic interoperability only. Complete MCP conformance and long-running reliability are not verified. Continue with 「LLMとの連携」章 and `host/mcp_llm_client.py`; LLM connection settings are placeholders.


### Register PicoNIX documentation in Open WebUI

If using Open WebUI Knowledge, upload `docs/en/reference/commands/MCP.md` and separately configure the MCP endpoint `http://PICO_IP:8080/mcp`. Uploading documentation does not enable tool calls, and retrieval is not guaranteed on every turn. Enforce approval for consequential actions in the host application.

## Interactive applications through PTY

Four terminal tools are always available. `terminal_open` starts a program with at most one optional argument. `terminal_send` takes either `text` or `key` and drains PTY output during an approximately 200 ms wait before returning the screen. This prevents a full output ring from stalling redraw, but does not guarantee that drawing has finished. `ESCAPE` aliases `ESC`, `BS` aliases `BACKSPACE`, and `SP` aliases `SPACE`. `terminal_read` returns the reconstructed **80x24 screen with ANSI inverse and underlined-cursor markers**, not the raw PTY byte stream. `terminal_close` terminates only this session's child and closes its PTY.

```text
terminal_open({"command":"fd"})
terminal_read({})
terminal_send({"key":"DOWN"})
terminal_read({})
terminal_send({"key":"ENTER"})
terminal_close({})
```

Send ordinary text with `terminal_send({"text":"hello"})`. Send a space with `terminal_send({"text":" "})` or `terminal_send({"key":"SPACE"})` (alias `SP`). Supported names include `UP`, `DOWN`, `LEFT`, `RIGHT`, `ENTER`, `ESC`, `ESCAPE`, `TAB`, `SPACE`, `SP`, `BACKSPACE`, `BS`, `DELETE`, and `CTRL_A` through `CTRL_Z`. `CTRL_C` uses PicoNIX's existing foreground-task termination, not POSIX signals. There is no scrollback.

Prefer `execute_command` for ordinary noninteractive commands. Only one PTY session is available and it shares the six task slots. Starting `mcpd` as a PTY application is forbidden. Kernel-side kill protection prevents MCP descendants from killing `mcpd` or the shell that launched it; it does not provide sandboxing or replace authentication/TLS (both absent).

Both `terminal_send` and `terminal_read` tool descriptions explain that SGR 4/24 marks the cursor and SGR 7/0 marks reverse video.

The `.screen` interface is a PicoNIX extension with intentionally limited ANSI support. See [PTY device](../devices/PTY.md).

### On-device PTY verification

```sh
python3 host/mcp_pty_test.py http://192.168.4.1:8080/mcp --command fd --keys DOWN,UP
```

The first read returns the 80x24 ANSI-marked screen; `ESC[4m`/`ESC[24m` surround the visible terminal cursor cell (none while hidden by CSI ?25l, restored by CSI ?25h). Later identical reads return `unchanged`. The two-pass comparison is not an atomic screen snapshot during concurrent updates.


### Repeated MCP transport test (2026-11-09)

The experimental `mcpd` processes **one HTTP connection at a time** and does not guarantee concurrent acceptance. `Connection: close` causes a new TCP handshake and teardown on each request. The host stress test reads the **entire response**, including the final HTTP chunk, rather than treating HTTP 200 alone as success.

```sh
# 50 read-only temperature calls, no SDK or LLM required
python3 host/mcp_stress.py http://192.168.4.1:8080/mcp --count 50

# On an isolated trusted test network; repeat a short observational command
python3 host/mcp_stress.py http://192.168.4.1:8080/mcp \
  --tool execute_command --command 'trace run hello' --count 50
```

The test prints the request number, elapsed time, and exception on failure; it stops at the first error by default. **Failed operations are never automatically retried.** `--continue-on-error` proceeds to the *next* numbered request for diagnostic runs and should not be used for commands with side effects.

Compare `cat /proc/net/memory` before and after testing on the device. `TCP_PCB_POOL` and `TCP_SEG_POOL` show lwIP pool usage (current/peak/allocation errors); `TCP_ACCEPT_BUSY` counts rejected connections while the single slot is occupied; `TCP_WRITE_BACKPRESSURE` indicates a full send queue; `TCP_CLOSE_DEFERRED` and `TCP_CLOSE_RETRY` record deferred close; and `TCP_ASYNC_ERROR` counts asynchronous lwIP errors. The `mcpd` console separately reports an HTTP send timeout (bytes still unsent) or a negative socket error. Correlate these with the existing `trace` output for `write`, `close`, and `select` syscalls. All counters are cumulative since boot: examine deltas.

Combining small HTTP chunks and adding diagnostics **does not prove that intermittent failures are fixed on hardware**. Keep the host failure number and the corresponding device-side observations.
