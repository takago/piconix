#!/usr/bin/env python3
"""Check bilingual coverage, newest-first diary dates, and editorial duplicates."""
from pathlib import Path
from datetime import date
import re

root = Path(__file__).resolve().parents[2]
paths = {}
for lang in ("ja", "en"):
    base = root / "docs" / lang
    paths[lang] = {p.relative_to(base) for p in base.rglob("*.md")}
assert paths["ja"] == paths["en"], (
    f"Bilingual Markdown path difference: {paths['ja'] ^ paths['en']}"
)
for lang in ("ja", "en"):
    diary = (root / "docs" / lang / "DEVELOPMENT_DIARY.md").read_text()
    dates = [date.fromisoformat(value) for value in re.findall(
        r"^#{2,3}\s+(\d{4}-\d{2}-\d{2})(?=[\s：:—-])", diary, re.M
    )]
    assert dates, f"No dated entries in {lang} diary"
    assert dates == sorted(dates, reverse=True), f"Diary dates out of order: {lang}"

ja_top = (root / "docs/ja/reference/commands/TOP.md").read_text()
assert ja_top.count("**表示出力の最適化：**") == 1
ja_diary = (root / "docs/ja/DEVELOPMENT_DIARY.md").read_text()
assert ja_diary.count("### 2026-11-12 — 全画面アプリの標準出力フラッシュを整理") == 1
print("PASS: bilingual document parity, diary order, and editorial duplication")
