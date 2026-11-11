#!/usr/bin/env python3
"""Offline tests for JSONL logging, usage aggregation, and streamed tool calls."""
import asyncio
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[2]
# No network or installed MCP package required for this test.
sys.modules.setdefault("mcp", SimpleNamespace(Client=object))
sys.modules["openai"] = SimpleNamespace(AsyncOpenAI=lambda **kw: None, BadRequestError=type("BadRequestError", (Exception,), {}))
spec = importlib.util.spec_from_file_location("mcp_llm_client", ROOT / "host/mcp_llm_client.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def delta(**fields):
    return SimpleNamespace(**fields)


def chunk(delta_obj=None, usage=None):
    return SimpleNamespace(choices=[] if delta_obj is None else [SimpleNamespace(delta=delta_obj)], usage=usage)


class FakeLLM:
    def __init__(self, supports_usage=True):
        self.calls = []
        self.chat = SimpleNamespace(completions=SimpleNamespace(create=self.create))
        self.supports_usage = supports_usage

    async def create(self, **kwargs):
        self.calls.append(kwargs)
        async def events():
            yield chunk(delta(content="Ready ", reasoning_content="private reasoning ", tool_calls=None))
            yield chunk(delta(content="now", reasoning_content="", tool_calls=[
                SimpleNamespace(index=0, id="call_1", function=SimpleNamespace(name="terminal_", arguments='{"key":'))]))
            yield chunk(delta(content="", reasoning_content="", tool_calls=[
                SimpleNamespace(index=0, id=None, function=SimpleNamespace(name="send", arguments='"UP"}'))]))
            if self.supports_usage:
                yield chunk(usage=SimpleNamespace(prompt_tokens=120, completion_tokens=45, total_tokens=165))
        return events()


async def test_stream():
    fake = FakeLLM()
    mod.llm = fake
    message, calls, usage = await mod.stream_reply([{"role":"user","content":"x"}], [])
    assert message["content"] == "Ready now"
    assert calls == [{"id": "call_1", "type": "function", "function": {
        "name": "terminal_send", "arguments": '{"key":"UP"}'}}]
    assert usage == {"prompt_tokens":120,"completion_tokens":45,"total_tokens":165}
    assert fake.calls[0]["stream_options"] == {"include_usage": True}
    assert "reasoning_content" not in str(message)
    fake = FakeLLM(False)
    mod.llm = fake
    assert (await mod.stream_reply([], []))[2] is None


asyncio.run(test_stream())
with tempfile.TemporaryDirectory() as temp:
    path = Path(temp) / "session.jsonl"
    writer = mod.SessionLog(path)
    writer.write("user", turn=1, content="日本語\\nline")
    writer.write("tool_result", content="TTY\\x1b[2J", is_error=False)
    writer.close()
    rows = [json.loads(line) for line in path.read_text().splitlines()]
    assert len(rows) == 2 and rows[0]["content"] == "日本語\\nline"
    assert rows[1]["content"] == "TTY\\x1b[2J"
    assert all("time" in row for row in rows)
    assert path.stat().st_mode & 0o777 == 0o600
    try:
        mod.SessionLog(path)
    except FileExistsError:
        pass
    else:
        raise AssertionError("existing log must not be overwritten")

counter = mod.UsageCounter()
counter.add({"prompt_tokens":100,"completion_tokens":25,"total_tokens":125})
counter.add(None)
counter.add({"prompt_tokens":300,"completion_tokens":40,"total_tokens":340})
assert counter.summary() == {"api_calls":3,"measured_calls":2,"missing_calls":1,
                             "prompt_tokens":400,"completion_tokens":65,"total_tokens":465}
assert mod.usage_fields(None) is None
assert mod.usage_fields({"prompt_tokens":2,"completion_tokens":3})["total_tokens"] == 5
assert mod.usage_fields({"prompt_tokens":2}) is None

# Drive one complete user turn with a tool round trip and then exit.
class FakeClient:
    protocol_version = "2026-07-28"
    def __init__(self, *args, **kwargs):
        pass
    async def __aenter__(self):
        return self
    async def __aexit__(self, *args):
        pass
    async def list_tools(self):
        return SimpleNamespace(tools=[SimpleNamespace(name="terminal_send", description="", input_schema={})])
    async def call_tool(self, name, arguments):
        assert name == "terminal_send" and arguments == {"key":"UP"}
        return SimpleNamespace(content=[SimpleNamespace(type="text", text="Player: (1,2)")], is_error=False)

class TwoRoundLLM(FakeLLM):
    async def create(self, **kwargs):
        self.calls.append(kwargs)
        index = len(self.calls)
        async def events():
            if index == 1:
                yield chunk(delta(content="", reasoning_content="keep this private", tool_calls=[
                    SimpleNamespace(index=0, id="call1", function=SimpleNamespace(name="terminal_send", arguments='{"key":"UP"}'))]))
            else:
                yield chunk(delta(content="Done", tool_calls=None))
            yield chunk(usage=SimpleNamespace(prompt_tokens=index*10, completion_tokens=5, total_tokens=index*10+5))
        return events()

async def test_conversation(log_path):
    mod.llm = TwoRoundLLM()
    mod.Client = FakeClient
    original_input = mod.input if hasattr(mod, "input") else None
    import builtins
    old = builtins.input
    iterator = iter(["mazeを解いて", "quit"])
    builtins.input = lambda *args: next(iterator)
    try:
        result = await mod.main(log_path)
        assert result is True
    finally:
        builtins.input = old

with tempfile.TemporaryDirectory() as temp:
    path = Path(temp) / "full.jsonl"
    asyncio.run(test_conversation(path))
    events = [json.loads(line) for line in path.read_text().splitlines()]
    assert [x["event"] for x in events] == ["session_start", "user", "assistant", "usage",
        "tool_call", "tool_result", "assistant", "usage", "turn_summary", "session_summary"]
    assert events[-1]["prompt_tokens"] == 30 and events[-1]["completion_tokens"] == 10
    assert events[-2]["api_calls"] == 2
    assert events[5]["content"] == "Player: (1,2)"
    assert not any("keep this private" in json.dumps(e) for e in events)

print("PASS: streamed reasoning/tool calls, usage, JSONL and missing usage")
