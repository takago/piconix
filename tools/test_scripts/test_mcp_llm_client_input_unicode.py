#!/usr/bin/env python3
"""Input-side Unicode safety for the MCP/LLM console client."""
import ast
import io
from pathlib import Path

source = Path(__file__).resolve().parents[2] / "host/mcp_llm_client.py"
content = source.read_text(encoding="utf-8")
tree = ast.parse(content)
helpers = [n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == "safe_user_text"]
ns = {}
exec(compile(ast.Module(body=helpers, type_ignores=[]), str(source), "exec"), ns)
safe = ns["safe_user_text"]
assert safe("日本語 ABC 😀") == "日本語 ABC 😀"
assert safe("bad \udcff and \ud800") == r"bad \udcff and \ud800"
assert safe("bad \udcff").encode("utf-8")
# Confirm a malformed byte can be read without UnicodeDecodeError.
stdin = io.TextIOWrapper(io.BytesIO(b"hello \xff\n"), encoding="utf-8", errors="backslashreplace")
assert safe(stdin.readline().strip()) == r"hello \xff"
assert 'sys.stdin.reconfigure(errors="backslashreplace")' in content
assert 'except UnicodeError as exc:' in content
print("PASS: input decoding, surrogate escaping and exception handling")
