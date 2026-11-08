# Call PicoNIX MCP tools from a local LLM

This guide follows the current `userland/apps/mcpd.c` and `host/mcp_llm_client.py` sources. It connects an OpenAI-compatible local LLM on Linux to Pico W. The former `get_wifi_info` and `display_message` tools are **removed** and must not be used.

[日本語](../../ja/guides/MCP-LLM-CLIENT.md) | [MCP usage](MCP.md) | [Agent operations](AGENT-GUIDE.md) | [Server tutorial](../development/apps/c/MCP-SERVER-DEVELOPMENT.md)

## Architecture and tools

```text
User → Local OpenAI-compatible LLM
           ↓ Function Calling
    host/mcp_llm_client.py
           ↓ MCP Streamable HTTP (TCP 8080)
       Pico W: mcpd → /dev, /proc, /man and shell
```

| Startup | Exposed tools |
|---|---|
| `mcpd &` | `get_temperature` only |
| `mcpd --shell &` | `get_temperature`, `get_manual`, `execute_command`, `write_file` |

`get_manual()` returns **the `/man/README` index**, not every manual page. `get_manual({"topic":"languages/c"})` returns a particular ROM page. `execute_command` runs short noninteractive commands. `write_file` overwrites ASCII data under `/tmp/NAME`. The host discovers tool schemas from `tools/list` instead of hardcoding the tool list.

## Setup and connection

On Pico W, start `mcpd --shell &` after connecting to a trusted, isolated test network. **There is no authentication or TLS; do not expose shell access to public or untrusted networks.** On Linux:

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install "mcp>=2,<3" openai
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool none
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool get_manual
```

Configure `LLM_URL`, `LLM_MODEL`, `LLM_API_KEY`, and `MCP_URL` in `host/mcp_llm_client.py` for your network and server. Your LLM must support Chat Completions and Function Calling. Do not commit credentials. The IP above is an AP-mode example.

```sh
python host/mcp_llm_client.py
```

Type `exit` or `quit` to stop. LLM output uses `stream=True` and appears incrementally. `[THINK]` is shown only when the model/server exposes a reasoning stream. `LLM_REASONING_EFFORT` support varies by server and model. LLM streaming is distinct from MCP HTTP chunked responses.

## Example tasks (illustrative, not measured)

```text
User: Inspect the PicoNIX IP address
→ get_manual() → execute_command({"command":"ip"})

User: Display a heart on the LED matrix
→ get_manual({"topic":"led-patterns"})
→ execute_command({"command":"echo heart > /dev/fbchar"})

User: List running tasks
→ execute_command({"command":"ps"})
```

To scroll text, try `echo HELLO | scroll`, but pipelines use multiple task slots. Do not launch interactive or full-screen commands such as `vi`, `edit`, `mem`, `watch`, or `fd` through MCP.

`host/mcp_client.py --tool all` only invokes available read-only tools, `get_temperature` and `get_manual`; it never automatically writes files or executes shell commands. Obtain approval before changing devices, files, or network state. This experimental MCP implementation does not guarantee model tool selection or correctness.
