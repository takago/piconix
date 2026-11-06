# PicoNIX exercises

Usage guides explain commands and examples, references describe current behavior, and development guides explain source code and implementation. This section collects tasks for learners to investigate and compare independently.

- [Kernel trace exercises](TRACE.md): syscalls, selected functions, and asynchronous events.
- [Kernel reading exercises](KERNEL-READING.md): review questions and the final source-reading task.

## Compare RAM and Flash (`mem`)

Use the [memory viewer guide](../guides/MEM.md) to carry out these observations.

1. Run `mem` and note addresses and highlighted changes. What could explain the changing values?
2. Press `1`–`6` to visit task slots and compare the displayed task names.
3. Press `k` and `K` to compare kernel RAM and XIP Flash. Explain their different purposes.
4. Run `mem -s kernel`. How do symbol names help you interpret addresses?

**Note:** Live RAM is not a consistent, atomic snapshot.

## Observe differences from Linux

From a PicoNIX shell, try:

```sh
ps
cat /proc/memory
ls /bin
ls /dev
ls /proc
cat /proc/ramfiles
echo hello | cat -n
```

Compare process capacity in `ps` with the six slots, inspect which paths are virtual, then follow one `read()` or `write()` from a user program through the system call and VFS. On Pico W, additionally try `ping` and `tcpdump`; network results depend on the connection.

**Learning point:** PicoNIX models UNIX concepts with a deliberately small implementation. Do not assume that a command name or API spelling means full Linux or POSIX compatibility.

## Network observation

After using the Pico W networking examples in the references, investigate the following questions.

1. What is the difference between the address shown for `lo0` and the Wi-Fi address? ([IP.md](../reference/network/IP.md))

## Compare MCP and LLM roles

Compare an ordinary question with a PicoNIX status query and observe when the LLM chooses `tools/call`. Compare `host/mcp_client.py` (no LLM) with `host/mcp_llm_client.py` (LLM-assisted) to understand the separation between MCP transport and tool selection.

[Usage guides](../guides/README.md) | [Learning roadmap](../ROADMAP.md) | [日本語](../../ja/exercises/README.md)
