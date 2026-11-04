#!/usr/bin/env python3
"""自動生成する関数一覧の抽出精度と更新状態を確認する．"""

import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("gen_functions", ROOT / "tools/gen_functions.py")
generator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(generator)

# コメントや文字列，マクロの波括弧と，プロトタイプを関数定義と誤認しない．
c = '''#define FAKE(x) { fake(x); }
typedef struct { int a; } record_t;
static const char *sample = "{ invalid() }";
int declared(int a);
/* 日本語の説明．詳細は次の文に記す． */
static int implemented(int x,
                       int (*callback)(int)) {
    if (x) { return callback(x); }
    return 0;
}
// もう一つの関数．
void next(void) { /* 文字列: "}" */ }
'''
entries = generator.c_functions(c)
assert [(name, line, scope) for name, line, scope, _ in entries] == [
    ("implemented", 6, "static"), ("next", 12, "外部")
], entries
assert entries[0][3] == "日本語の説明．", entries[0]
assert entries[1][3] == "もう一つの関数．", entries[1]

py = '''# このコメントを使う．
def top():
    return 1

def _private():
    """docstringによる説明．"""
    def local():
        pass
    return local
'''
assert [(name, line, scope) for name, line, scope, _ in generator.py_functions(py)] == [
    ("top", 2, "モジュール"), ("_private", 5, "内部名")
]
assert generator.py_functions(py)[0][3] == "このコメントを使う．"

asm = '''/* 例外ハンドラの役割を説明． */
.global handler
.type handler,%function
handler:
    bx lr
.size handler,.-handler
other:
    bx lr
'''
assert generator.asm_functions(asm) == [("handler", 4, "大域", "例外ハンドラの役割を説明．")]

# すべての掲載関数にソース由来の概要があることをチェックする．
# 新たな関数を追加した際は，ソース側の説明コメントを付けてから再生成する．
for path in generator.sources():
    source = path.read_text(encoding="utf-8")
    if path.suffix in (".c", ".h"):
        indexed = generator.c_functions(source)
    elif path.suffix == ".S":
        indexed = generator.asm_functions(source)
    else:
        indexed = generator.py_functions(source)
    assert all(summary for _, _, _, summary in indexed), f"概要がない関数: {path}"

# 機械生成結果が常に一致することをチェックする．
assert generator.OUTPUT.read_text(encoding="utf-8") == generator.render(), "関数一覧を再生成してください"
print("PASS: function index parsing and generated documentation")
