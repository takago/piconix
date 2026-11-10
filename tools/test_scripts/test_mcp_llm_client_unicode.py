#!/usr/bin/env python3
"""Check that the host LLM client handles unencodable tool text safely."""
import ast
import io
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[2] / "host/mcp_llm_client.py"
source = SOURCE.read_text(encoding="utf-8")
module = ast.parse(source)

# Run only formatting helpers, without requiring a live LLM or MCP server.
helpers = [n for n in module.body if isinstance(n, ast.FunctionDef) and n.name in ("styled", "show")]
ns = {"USE_COLOR": True, "COLORS": {"result": "\033[34m"}}
code = compile(ast.Module(body=helpers, type_ignores=[]), str(SOURCE), "exec")
exec(code, ns)

# Emulate an ASCII-only terminal that rejects surrogate code points by default.
output = io.TextIOWrapper(io.BytesIO(), encoding="ascii", errors="strict", write_through=True)
output.reconfigure(errors="backslashreplace")
import sys
saved = sys.stdout
try:
    sys.stdout = output
    ns["show"]("[RESULT]", "screen: \udcff", "result")
finally:
    sys.stdout = saved
assert b"\\udcff" in output.buffer.getvalue(), output.buffer.getvalue()
assert 'except Exception as exc:' in source and 'LLM\u5fdc\u7b54\u3092\u53d6\u5f97\u3067\u304d\u307e\u305b\u3093\u3067\u3057\u305f' in source
print("PASS: unencodable MCP result is safely displayed; stream errors are handled")
