> **2026-11-08 update:** `host/mcp_llm_client.py` now always calls the LLM with `stream=True` and uses `reasoning_effort="low"` by default for Ollama's OpenAI-compatible endpoint. If the model/server exposes `reasoning_content` or `reasoning` stream deltas, the client displays them live as gray `[THINK]` text. Normal replies stream as `LLM >`; fragmented tool-call data is reconstructed before invoking MCP. If the API omits reasoning text, nothing is shown under `[THINK]`. No PicoNIX firmware changes are required.

**Configuration:** Change `LLM_REASONING_EFFORT = "low"` to another level supported by your model/server if needed. Actual thinking depth depends on the model and Ollama version. Displayed reasoning is only what the API exposes, not necessarily the model's entire internal reasoning. LLM streaming is distinct from MCP Streamable HTTP.

# Call PicoNIX MCP tools from a local LLM

> **Change on 2026-11-08:** `get_wifi_info` and `display_message` have been removed from current `mcpd`. Examples using these tools below document the earlier implementation. With `mcpd --shell`, call `get_manual()` without arguments, then run commands such as `ip` or `echo HELLO | scroll` using `execute_command`. See the [agent guide](AGENT-GUIDE.md).

## Current MCP agent workflow (2026-11-08)

With `mcpd --shell &`, the current tools are `get_temperature`, `execute_command`, `write_file`, and `get_manual`. `get_wifi_info` and `display_message` have been removed. Starting `mcpd` without `--shell` does not expose shell-operation tools.

`get_manual` now takes **no arguments** and returns the complete embedded guide. `host/mcp_llm_client.py` discovers tools dynamically via `tools/list`. Its system prompt instructs the model to request `get_manual()` before operating PicoNIX, and to consult `execute_command({"command":"help trace"})` when details are needed. Model compliance with this instruction is not guaranteed.

```sh
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool get_manual
python host/mcp_llm_client.py
```

The direct test client sends an empty argument object `{}` to `get_manual`. Its `--tool all` option calls only available read-only tools (`get_temperature` and `get_manual`), not `execute_command` or `write_file`.

For LED text use a standard-input pipeline such as `echo HELLO | scroll` via `execute_command`, not UDP or `nc` unless specifically needed. Pipelines consume extra process slots. Do not run full-screen or interactive programs such as `mem`, `vi`, or `watch` through MCP. Use `mcpd --shell` only on an isolated trusted network, because it exposes unauthenticated command execution.

The older tool examples below are retained as development history and do not describe current operation.



This guide connects an OpenAI-compatible LLM on a Linux PC to the PicoNIX `mcpd` server on Pico W. `host/mcp_llm_client.py` is a small **always-streaming** Python example: the LLM chats normally and selects MCP tools only when needed.

**Hardware-verified (2026-11-06):** A Linux client connected to PicoNIX in AP mode at `192.168.4.1`, listed and invoked tools using the official MCP Python SDK v2, and used a local LLM to select tools and compose responses. This verifies a basic workflow, not complete MCP compliance or long-term reliability.

[日本語](../../ja/guides/MCP-LLM-CLIENT.md) | [MCP server guide](MCP.md) | [Server development tutorial](../development/apps/c/MCP-SERVER-DEVELOPMENT.md)

## 1. Architecture

```text
Linux PC                         Pico W / PicoNIX
User -> local LLM
        | tool request              mcpd (TCP 8080)
        v                           | /dev/adc/temp
host/mcp_llm_client.py -- HTTP --> | /proc/net/wifi
        ^             <-- result -- |
        |
local LLM -> natural-language answer
```

The LLM does not connect to MCP directly: the Python application translates OpenAI-compatible function calls into MCP `tools/call` requests. LLM `stream=True` and MCP *Streamable HTTP* are unrelated; this example displays LLM output as it arrives.

## 2. Setup

Start `mcpd &` on Pico W and verify the MCP connection **without** an LLM first:

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install "mcp>=2,<3" openai
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool none
```

Configure these constants near the beginning of `host/mcp_llm_client.py`. Replace `######` with your own LLM settings:

```python
LLM_URL = "http://######:11434/v1"
LLM_MODEL = "######"
LLM_API_KEY = "######"
LLM_REASONING_EFFORT = "low"

MCP_URL = "http://192.168.4.1:8080/mcp"
```

The LLM server must implement the OpenAI-compatible Chat Completions API and function/tool calling. Port `11434` is only an example. The MCP endpoint is kept at the tested PicoNIX AP address; update it only if your network differs. Never commit a real API key.

## 3. Run

```sh
python host/mcp_llm_client.py
```

Excerpt from the successful hardware test (measurements vary):

```text
User > こんにちは
LLM > こんにちは！何かお手伝いできることはありますか？

User > PicoNIXの状態を教えて
Tool: get_wifi_info
Result: MODE: AP
...
IP: 192.168.4.1
...
Tool: get_temperature
Result: 31.8

LLM > PicoNIXの現在の状態は以下の通りです．
... 31.8°C, AP mode, IP address 192.168.4.1 ...
```

Enter `exit` or `quit` to leave. Normal questions require no tool. Device-specific questions may trigger `get_temperature` and/or `get_wifi_info`; the selection, ordering and wording depend on the model.

## 4. Reading the source

1. `Client(MCP_URL, mode="auto")` opens an MCP connection.
2. `mcp.list_tools()` provides tool names, descriptions and `input_schema`, converted to the OpenAI-compatible tool format.
3. `llm.chat.completions.create()` returns a chat message, possibly with `tool_calls`.
4. If tools are requested, parse JSON arguments and call `mcp.call_tool()`.
5. Append tool results to the chat history with `role="tool"` and ask the LLM again.
6. When there is no tool call, print the final answer and wait for another question.

MCP Python SDK v2 uses the attributes `input_schema` and `is_error`, not `inputSchema` or `isError`. Text output comes from items in `result.content`.

## 5. Limitations and safety

- The prototype MCP server has neither authentication nor TLS. Use a trusted, isolated test network only.
- Both current tools are read-only. Hardware write operations require server-enforced validation, authorization and confirmation policies.
- The example deliberately has no tool-call count limit. A model may repeat calls indefinitely; add limits for production use.
- An LLM may add unsupported interpretations of sensor readings. Distinguish the measured data from model inference.
- Tool calling depends on the OpenAI-compatible server and selected model.

## 6. Related exercises

See [exercises](../exercises/README.md#compare-mcp-and-llm-roles) for questions comparing MCP transport and LLM tool selection.


## LED ticker and multi-tool orchestration

Start `scroll -u 2001` on PicoNIX, then try:

```sh
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool display_message --text "HELLO PICONIX"
python host/mcp_llm_client.py
```

Ask the LLM: `Find the PicoNIX IP address and scroll it on the LED matrix.` The expected workflow is `get_wifi_info` followed by `display_message` with the retrieved IP as `text`. This is an illustrative workflow, not a recorded run. No IP-specific workflow is hardcoded into the Python client.

A successful UDP send does not confirm delivery or display. If `scroll` is stopped, UDP sending may still succeed. Use UDP only on trusted networks. `--tool all` now also sends an LED message; use `--tool none` or a read-only tool to avoid modifying the display.
