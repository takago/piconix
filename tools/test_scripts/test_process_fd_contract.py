#!/usr/bin/env python3
"""Ensure process/pipe resource and error contracts remain in sync with docs."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
k = (root / "kernel/kernel.c").read_text()
for fragment in (
    "if(!running) { result=OS_ESRCH; break; }",
    "if(c & 1) { result=0; break; }",
    "if(!exited[i].pid) { pos=i; break; }",
    "if(pos<0) pos=0;",
    "for(int j=0;j<3;j++) fd_inherit(t,j,&tasks[current],j);",
    "result=slot<0 ? OS_EAGAIN : spawn_in_slot(",
    "if(found!=2) { result=OS_EMFILE; break; }",
    "if(index<0) { result=OS_EAGAIN; break; }",
    "if(target<0) { result=OS_EMFILE; break; }",
    "result=OS_EIO; break;",
    "for(int fd=0;fd<OS_FD_MAX;fd++) fd_release(t,fd);",
    "pipe_ref(src->fd[from],1);",
    "pipe_ref(dev,-1);",
):
    assert fragment in k, f"kernel contract changed: {fragment}"
for locale in ("ja", "en"):
    syscalls = (root / f"docs/{locale}/reference/SYSCALLS.md").read_text()
    pipe = (root / f"docs/{locale}/guides/PIPE.md").read_text()
    for symbol in ("OS_EINVAL", "OS_ENOENT", "OS_EAGAIN", "OS_ESRCH", "OS_EMFILE", "OS_EIO", "WNOHANG", "dup2", "waitpid"):
        assert symbol in syscalls, f"{locale} syscall document lacks {symbol}"
    for symbol in ("OS_EMFILE", "OS_EAGAIN", "OS_EINVAL", "dup2"):
        assert symbol in pipe, f"{locale} pipe document lacks {symbol}"
print("PASS: process lifecycle, FD resources, and bilingual contract coverage")
