# Call PicoNIX MCP tools from a local LLM

This tutorial connects an OpenAI-compatible LLM on a Linux PC to the PicoNIX `mcpd` server on Pico W. `host/mcp_llm_client.py` is a small **non-streaming** Python example: the LLM chats normally and selects MCP tools only when needed.

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

The LLM does not connect to MCP directly: the Python application translates OpenAI-compatible function calls into MCP `tools/call` requests. LLM `stream=True` and MCP *Streamable HTTP* are unrelated; this example collects each LLM response before displaying it.

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

## 6. Exercise

Compare an ordinary question with a PicoNIX status query and observe when the LLM chooses `tools/call`. Compare `host/mcp_client.py` (no LLM) with `host/mcp_llm_client.py` (LLM-assisted) to understand the separation between MCP transport and tool selection.
