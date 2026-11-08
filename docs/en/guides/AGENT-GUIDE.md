# PicoNIX Agent Guide (experimental)

This guide is intended for a host-side LLM agent, such as Open WebUI, controlling **PicoNIX** through MCP. PicoNIX is a small educational UNIX-like OS running on RP2040. **It is not Linux.** Do not assume `bash`, `sudo`, `apt`, `gcc`, `ssh`, a POSIX shell, or a complete C standard library is available.

## Enabling the command tool

On a trusted, isolated laboratory network only:

```sh
mcpd --shell &
```

This explicitly enables the MCP tools `execute_command` and `write_file` in addition to `get_temperature`, `get_wifi_info`, and `display_message`. Normal `mcpd &` does **not** expose command execution. The HTTP endpoint is `http://<pico-w-ip>:8080/mcp`. This implementation uses a deliberately limited MCP protocol (`2026-07-28`, JSON responses without SSE). Check compatibility with the particular Open WebUI MCP client or use the included host client/compatible bridge. Authentication, TLS, and per-command authorization are **not implemented**. Do not expose port 8080 to an untrusted network or the public Internet.

## `execute_command`

Input schema: `{"command": "ps"}`. The tool launches a **new**, non-interactive `sh -c` process for each request, captures stdout/stderr through a pipe, and returns an MCP text result. Its working directory starts at `/tmp` for each invocation; `cd` changes are **not** preserved across calls. Shell commands are limited to 119 printable ASCII bytes, output to 4,096 bytes (truncation indicated), and a nominal 3-second timeout. The tool streams JSON with HTTP/1.1 chunked transfer rather than allocating a 4 KiB output array inside `mcpd`; the MCP client must support chunked HTTP responses. A disconnected response can be incomplete. A timed-out shell may leave a descendant process running; review with `ps` if this happens. The completion indicator is **not** a reliable exit status of the invoked command. Avoid commands that require interactive input or a terminal.

The tool is **not a sandbox**. The shell is allowed to access devices, RAM files, and network resources. Consult the user before overwriting files, killing processes, resetting the device, or changing hardware state. The default expectation is to inspect, explain, and request permission before modifying. Use only within an isolated and resettable lab environment.

## `write_file` — creating small source files

When started with `mcpd --shell`, PicoNIX also exposes `write_file` for overwriting a RAM text file under `/tmp`. Example MCP tool arguments:

```json
{"path":"/tmp/test.c","content":"int main(void) { write(1, \"OK\\n\", 3); return 0; }\n"}
```

The `content` parameter is a JSON string (normal `\n`, `\t`, and escaped quotes are accepted). Only ASCII text is supported. Files are limited to **2,048 bytes**, and the entire encoded HTTP JSON request must fit the **3,200-byte request-body limit**; many escaped characters may reduce the practical maximum. No append/resume support is provided. The operation truncates an existing file; inspect and obtain user approval before overwriting. Only simple `/tmp/NAME` paths are accepted (letters, digits, dot, hyphen, underscore; no subdirectories). PicoNIX has only 11 RAM-file slots shared by `/tmp` and `/etc`. A failed write may leave a partial file. The tool reports the number of decoded bytes written, not a verified compiler result.

A possible agent workflow is: `write_file` → `execute_command("cc /tmp/test.c")` → `execute_command("as /tmp/test.s")` → `execute_command("/tmp/a.out")`. This is experimental and not yet verified on hardware. Do not assume that every C program fits the RAM file limit; assembly output can be larger than source code.

## Command examples

| Goal | Example | Notes |
| --- | --- | --- |
| Available commands | `help` | Check actual command list first |
| Files | `ls`, `pwd`, `cat /etc/rc` | Filesystem is largely RAM-backed |
| Processes | `ps`, `lsof` | Process slots are limited |
| Network | `ip`, `cat /proc/net/wifi` | Wi-Fi requires Pico W |
| Network memory | `cat /proc/net/memory` | lwIP heap and pbuf diagnostic |
| Hardware | `cat /dev/adc/temp` | Read temperature sensor |
| Compiler | `cc test.c`, `as test.s` | C subset, not ISO C |
| Pipeline | `ps | grep sh` | At most three commands; no redirects in pipeline |
| Traces | `trace` | Inspect `help` for current options |

Example workflow for compiling an existing C file:

```text
execute_command({"command":"ls"})
execute_command({"command":"cc test.c"})
execute_command({"command":"as test.s"})
execute_command({"command":"/tmp/a.out"})
```

The tool is designed for short observational commands. For longer output, request a smaller range or use a more specific command. Programs such as `vi`, `edit`, `python` in interactive mode, or `scroll` in the foreground are unsuitable. Do not assume piping, quoting, globbing, redirection, job control, and variable expansion work exactly as on a Linux shell. Do not run `sh -c` recursively without a reason.

## Configuring Open WebUI

These steps assume a **trusted, isolated lab network**. `mcpd --shell` provides unauthenticated command execution without TLS or per-command authorization. Restrict network access and access to the Open WebUI model/tools. UI labels may vary by Open WebUI version.

### 1. Load this guide into Knowledge

1. In **Workspace → Knowledge → Create**, create a collection such as `PicoNIX Agent Guide`.
2. Upload `docs/en/guides/AGENT-GUIDE.md` and wait for processing to finish. The Japanese version is at `docs/ja/guides/AGENT-GUIDE.md`.
3. Under **Workspace → Models**, create or edit a dedicated PicoNIX model and attach the Knowledge collection. For an individual chat, you can also reference Knowledge using `#`.
4. **Full Context** can work for a short guide, but with Native function calling, model-attached Knowledge is not always injected automatically. The model may need to invoke Knowledge tools. If it fails to consult the guide, check Knowledge tool settings and instruct it in the system prompt to retrieve the PicoNIX guide before acting.

### 2. Connect the MCP server

1. On Pico W, check the IP address and start `mcpd --shell &` on the isolated network.
2. As an Open WebUI administrator, navigate to **Settings → Admin → Integrations → External Tool Servers → Add Connection**.
3. Select **MCP (Streamable HTTP)** (not OpenAPI) and enter `http://<pico-w-ip>:8080/mcp` as Server URL. Save the connection. If Open WebUI runs in Docker, its container must be able to reach Pico W over the network.
4. Confirm that tools such as `get_wifi_info`, `display_message`, `execute_command`, and `write_file` are discovered. Enable the required tools for the model/chat and configure access appropriately.

> **Compatibility:** The PicoNIX MCP server implements a limited subset of MCP. Some Open WebUI versions may not connect to it directly. If connection fails, first verify communication using the supplied `host/mcp_llm_client.py` or another compatible client, then consider a bridge if required.

### 3. Configure the system prompt

Use the suggested **Agent operating policy** below as the dedicated model's System Prompt. Keep critical rules in the prompt rather than depending solely on Knowledge retrieval: PicoNIX is not Linux, changes require approval, and RAM files are limited to 2 KiB.

### 4. Verify the connection

In a fresh chat, ask: “Which processes are running on PicoNIX?” Verify that the agent invokes `execute_command` with `ps` and reports the actual output. Then try read-only commands such as `ip` or `help`. If this fails, inspect MCP connectivity, model tool permissions, and Knowledge access separately. Do not test `write_file` or state-changing hardware commands until read-only behavior works and the user has approved the change.

Official Open WebUI documentation: [Knowledge](https://docs.openwebui.com/features/workspace/knowledge/) / [Models](https://docs.openwebui.com/features/workspace/models/) / [MCP integration](https://docs.openwebui.com/features/extensibility/mcp/).

## Agent operating policy (suggested system prompt)

> You are connected to PicoNIX, a resource-constrained educational UNIX-like OS, **not** a Linux computer. Use the `execute_command` MCP tool to inspect state using small, bounded commands. Start with `help` when unsure. Never invent command-line flags or assume POSIX utilities. Ask for the user's explicit approval before commands that modify files, processes, devices, or network configuration. Use no infinite loops or interactive commands. Respect the short timeout and output limit. Explain what each command does and interpret its output cautiously. An MCP tool result does not guarantee the OS is still running. If the device stops responding, do not blindly retry potentially destructive commands.

## Differences from Linux

- Six fixed user-task slots, each 24 KiB including a 4 KiB reserved stack on RP2040; no memory isolation or Unix user permissions.
- Minimal shell: one command line, up to three pipeline stages; no general Bash syntax or package manager.
- `/tmp` is RAM-backed. `/proc` and `/dev` expose OS and device interfaces; some writes can affect physical pins.
- `cc` compiles a **subset of C** to Thumb-1 assembly; `as` assembles it to a RAM-loadable image.
- `mcpd` is a user-space server and needs a free task slot to spawn `sh` and its child commands.
- `get_wifi_info` and `display_message` are separate MCP tools; the latter sends UDP text to `scroll -u 2001 &` (no delivery guarantee).

## Host-side logging

Record command, timestamp, tool result, error/truncation status and the agent's rationale **on the host**. `trace` within PicoNIX provides a separate view of system calls and scheduling. Do not assume clocks are synchronized between the host and PicoNIX.

## Limitations and verification

This feature was added as a small experimental implementation. ARM-target syntax checks can pass without demonstrating Pico W hardware execution. Validate on hardware with `execute_command({"command":"ps"})`, then try other read-only commands. Verify network isolation before enabling the shell tool.
