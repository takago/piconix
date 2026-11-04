#!/usr/bin/env python3
"""ソース中の関数定義と直前の説明コメントから索引を作る．

使い方:
    python3 tools/gen_functions.py
    python3 tools/gen_functions.py --check

Cはコメント・文字列・プリプロセッサを隠して最上位の関数定義を探す．
外部ライブラリは不要だが，C言語全体を解析するコンパイラではない．
"""

import argparse
import ast
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "docs/FUNCTIONS.md"
# PicoNIXの実装関数を用途別に分類する．
# tools/ はビルド・テスト用であり，userland/examples/ と userland/fs/ は
# 実行例・初期ファイルなので，関数索引には含めない．
GROUPS = (
    ("カーネル", ("kernel",)),
    ("アプリケーション", ("userland/apps",)),
    ("ライブラリ", ("userland/lib",)),
    ("ホスト用ユーティリティ", ("host",)),
)


def mask_source(source):
    """コメントと文字列を空白に置換し，行番号と括弧を保つ．"""
    out = list(source)
    comments = []
    n = len(source)
    i = 0
    while i < n:
        start = i
        if source.startswith("//", i):
            end = source.find("\n", i)
            i = n if end < 0 else end
            comments.append((start, i, source[start:i]))
        elif source.startswith("/*", i):
            end = source.find("*/", i + 2)
            i = n if end < 0 else end + 2
            comments.append((start, i, source[start:i]))
        elif source[i] in ('"', "'"):
            quote = source[i]
            i += 1
            while i < n:
                if source[i] == "\\":
                    i += 2
                elif source[i] == quote:
                    i += 1
                    break
                else:
                    i += 1
        else:
            i += 1
            continue
        for j in range(start, min(i, n)):
            if source[j] != "\n":
                out[j] = " "

    # マクロに含まれる { } や関数呼び出しを関数定義と誤認しない．
    offset = 0
    directive = False
    for line in "".join(out).splitlines(keepends=True):
        if line.lstrip().startswith("#"):
            directive = True
        if directive:
            for j in range(offset, offset + len(line)):
                if out[j] != "\n":
                    out[j] = " "
            directive = line.rstrip("\n").rstrip().endswith("\\")
        offset += len(line)
    return "".join(out), comments

def comment_summary(comment):
    """直前のコメントから最初の説明文を取り出す．"""
    lines = []
    for line in comment.splitlines():
        line = re.sub(r"^\s*(?:/\*+|\*|//)\s?", "", line)
        line = re.sub(r"\*/\s*$", "", line).strip()
        if line and "SPDX-License-Identifier:" not in line:
            lines.append(line)
    if not lines:
        return ""
    # 複数行に分かれた説明を繋ぎ，最初の一文だけを採用する．
    full = "".join(lines) if re.search(r"[ぁ-んァ-ヶ一-龯]", lines[0]) else " ".join(lines)
    for sep in ("．", "。", ". "):
        pos = full.find(sep)
        if pos >= 0:
            full = full[:pos + (1 if sep != ". " else 1)]
            break
    return full.strip()

def preceding_comment(source, comments, start):
    """関数宣言と空白だけを挟んで接するコメントを取得する．"""
    for a, b, raw in reversed(comments):
        if b <= start:
            if not source[b:start].strip():
                return comment_summary(raw)
            break
    return ""

def c_functions(source):
    """波括弧の深さ0にあるC関数定義を列挙する．宣言は含めない．"""
    code, comments = mask_source(source)
    found = []
    depth = 0
    boundary = 0
    for i, ch in enumerate(code):
        if ch == "{":
            if depth == 0:
                header = code[boundary:i]
                match = re.search(r"\b([a-zA-Z_]\w*)\s*\((.*)\)\s*$", header, re.S)
                if match:
                    name = match.group(1)
                    prefix = header[:match.start(1)].strip()
                    if (prefix and "=" not in prefix and "typedef" not in prefix.split()
                            and name not in {"if", "while", "switch", "for"}):
                        # 開始行は宣言の先頭行（コメント・空白は数えない）．
                        start = boundary + len(header) - len(header.lstrip())
                        line = source.count("\n", 0, start) + 1
                        found.append((name, line, "static" if "static" in prefix.split() else "外部",
                                      preceding_comment(source, comments, start)))
            depth += 1
        elif ch == "}":
            if depth:
                depth -= 1
                if depth == 0:
                    boundary = i + 1
        elif ch == ";" and depth == 0:
            boundary = i + 1
    return found

def py_functions(source):
    """Pythonのトップレベル関数を抽出する（テスト・関数内の補助関数は対象外）．"""
    tree = ast.parse(source)
    found = []
    lines = source.splitlines()
    for node in tree.body:
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            doc = ast.get_docstring(node) or ""
            summary = doc.splitlines()[0].strip() if doc else ""
            if not summary:
                # docstringがなければdefのすぐ上の#コメントを利用する．
                i = node.lineno - 2
                while i >= 0 and not lines[i].strip():
                    i -= 1
                if i >= 0 and lines[i].lstrip().startswith("#"):
                    summary = lines[i].strip().lstrip("#").strip()
            found.append((node.name, node.lineno, "モジュール" if not node.name.startswith("_") else "内部名", summary))
    return found

def asm_functions(source):
    """.typeで関数と指定されたアセンブリの入口ラベルを抽出する．"""
    declarations = set(re.findall(r"(?m)^\s*\.type\s+([a-zA-Z_]\w*)\s*,\s*%function\b", source))
    found = []
    for line, text in enumerate(source.splitlines(), 1):
        match = re.match(r"^([a-zA-Z_]\w*):\s*$", text)
        if match and match.group(1) in declarations:
            name = match.group(1)
            # .globalの直前にあるコメントを使用する（.type等の指示子は読み飛ばす）．
            header = re.search(r"(/\*[^*]*?(?:\*(?!/)[^*]*?)*\*/)\s*\.global\s+" + name + r"\b", source, re.S)
            summary = comment_summary(header.group(1)) if header else ""
            found.append((name, line, "大域", summary))
    return found

def sources():
    """OS本体・ホスト操作プログラムの実装ファイルのみ取得する．"""
    for _, directories in GROUPS:
        for dirname in directories:
            for path in sorted((ROOT / dirname).rglob("*")):
                if path.is_file() and path.suffix in {".c", ".h", ".py", ".S"}:
                    yield path

# 対象ファイルの関数定義と説明を集計し，Markdown形式の関数一覧を構築する．
def render():
    grouped = {heading: [] for heading, _ in GROUPS}
    count = 0
    documented = 0
    for path in sources():
        source = path.read_text(encoding="utf-8")
        if path.suffix in {".c", ".h"}:
            entries = c_functions(source)
        elif path.suffix == ".S":
            entries = asm_functions(source)
        else:
            entries = py_functions(source)
        if not entries:
            continue
        relative = path.relative_to(ROOT).as_posix()
        heading = next(title for title, dirs in GROUPS
                       if any(relative.startswith(dirname + "/") for dirname in dirs))
        grouped[heading].append((relative, entries))
        count += len(entries)
        documented += sum(bool(entry[3]) for entry in entries)

    out = [
        "# 関数一覧（自動生成）", "",
        "この文書は `python3 tools/gen_functions.py` でソースから自動生成します．",
        "**手で編集せず，関数直前の日本語コメントを修正して再生成してください．**", "",
        "- **カーネル**：`kernel/` にあるOS本体の関数．",
        "- **アプリケーション（App）**：`userland/apps/` にあるコマンドやアプリの関数．",
        "- **ライブラリ（Lib）**：`userland/lib/` にある共通Cライブラリとシステムコール呼出しの関数．",
        "- **ホスト用ユーティリティ**：`host/` にあるPC側のPicoNIX操作プログラムの関数（OS本体とは別）．",
        "- **対象外**：`tools/`（ビルド・検証・索引生成），`userland/examples/`（実行例），`userland/fs/`（初期ファイル）．",
        "- C関数定義，Pythonトップレベル関数，アセンブリの関数ラベルを対象とし，ヘッダは**関数定義のみ**を掲載します（宣言・マクロ・型定義は対象外）．",
        "- 関数定義がないファイルは省略します．",
        "- 行番号はこの文書を生成した時点の行番号です．変更後は再生成してください．",
        "- 概要の `—` は関数直前に説明コメントがないことを表します．関数名からの推測は行いません．",
        "- 範囲欄の `static` はCの内部結合，`外部` は外部結合，`モジュール` と `内部名` はPythonの名前付け，`大域` はアセンブリラベルを表します．外部結合は公開APIを意味しません．",
        "- C言語の解析は簡易的な構文走査で，プリプロセッサの条件分岐を評価しません．環境依存の定義は両方載る場合があります．", "",
        f"**掲載関数：{count}件（概要コメントあり：{documented}件）**", "",
        "## 分類別目次", "",
    ]
    for title, _ in GROUPS:
        if grouped[title]:
            out.append(f"- [{title}](#{anchor(title)})：{len(grouped[title])}ファイル")
    out += [""]
    for title, _ in GROUPS:
        if not grouped[title]:
            continue
        out.extend((f"## {title}", ""))
        for relative, entries in grouped[title]:
            out.extend((f"### `{relative}`（{len(entries)}件）", "", "| 関数 | 行 | 範囲 | 概要 |", "|---|---:|---|---|"))
            for name, line, scope, comment in entries:
                # 相対パスリンクはGitHub上でもソースの該当行に移動できる．
                link = f"../{relative}#L{line}"
                synopsis = (comment or "—").replace("|", "\\|").replace("`", "\\`")
                out.append(f"| [`{name}()`]({link}) | {line} | {scope} | {synopsis} |")
            out.append("")
    return "\n".join(out).rstrip() + "\n"

# GitHub Markdownにおける日本語見出しのアンカー文字列を返す．
def anchor(heading):
    # 章タイトルのGitHub Markdown見出しアンカーは日本語をそのまま使える．
    return heading

# --check指定時は再生成漏れを検出し，通常時は関数一覧ファイルを書き出す．
def main():
    parser = argparse.ArgumentParser(description="PicoNIXの関数一覧をソースから自動生成")
    parser.add_argument("--check", action="store_true", help="生成結果と既存文書を比較し，不一致なら終了コード1")
    args = parser.parse_args()
    content = render()
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != content:
            parser.exit(1, "FAIL: docs/FUNCTIONS.md は古くなっています．python3 tools/gen_functions.py で更新してください．\n")
        print("PASS: docs/FUNCTIONS.md は現行ソースと一致")
    else:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != content:
            OUTPUT.write_text(content, encoding="utf-8")
        print(f"Wrote {OUTPUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
