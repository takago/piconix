#!/usr/bin/env python3
"""Ensure /etc RAM files remain editable from edit, vi and fd."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
common = (root / "userland/apps/editor_common.h").read_text()
fd = (root / "userland/apps/fd.c").read_text()
help_text = (root / "userland/apps/help.c").read_text()
page = (root / "userland/fs/etc_picow/index.html").read_text()
assert 'strncmp(path,"/tmp/",5)&&strncmp(path,"/etc/",5)' in common
assert 'strncmp(edit_path,"/tmp/",5)&&strncmp(edit_path,"/etc/",5)' in fd
assert 'strchr(path+5' in common and 'strchr(edit_path+5' in fd
assert 'only files in /tmp or /etc' in common
assert 'Edit a /tmp or /etc file' in help_text
assert 'edit /etc/index.html' in page and 'dd if=' not in page
assert len(page.encode()) <= 2048
print("PASS: /tmp and /etc editor paths and Web page directions")
