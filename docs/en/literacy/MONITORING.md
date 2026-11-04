# System monitoring lab

[日本語](../../ja/literacy/MONITORING.md) | [Learning path](LEARNING-PATH.md) | [Trace reference](TRACE.md) | [Memory viewer](MEM.md)

Learn to **observe a running OS** through processes, file descriptors (FDs), system calls, and memory. The first four steps run on both Pico and Pico W with only USB. The `piconix#` prompt identifies commands entered on PicoNIX, rather than on the Linux host PC.

## 1. Inspect processes

```text
piconix# ps
piconix# cat /proc/uptime
piconix# cat /proc/tasks
```

**Check:** Compare PIDs and command names in `ps` with the information under `/proc`. Unlike a stored file, `/proc` provides a view of live system state, which may change between reads.

## 2. Link FDs to standard I/O

```text
piconix# lsof
piconix# echo hello | cat
piconix# lsof
```

**Check:** Recall the roles of FDs 0, 1, and 2. `lsof` lists FDs open by running tasks. Short-lived `echo` and `cat` processes may be gone before the second `lsof`: this is expected. Find an active PID with `ps` and use `lsof PID` to inspect it. See [Pipes](PIPE.md).

## 3. Observe requests to the kernel

```text
piconix# trace run hello
piconix# trace run ls
```

**Check:** `ENTER` and `EXIT` mark system-call entry and exit, and `SWITCH` marks a task switch. The number of events is not fixed. Why do `hello` and `ls` produce different traces? `trace run` displays events after the named program exits. The trace ring buffer can overwrite old records. Read the [trace reference](TRACE.md) for details.

## 4. Compare RAM and Flash

```text
piconix# mem -s
piconix# mem
```

In the full-screen viewer, use `1`–`6` to jump to user slots, `k` for kernel RAM, `K` for XIP Flash, and `q` to quit. **Check:** Compare address ranges and changing bytes. This is not an atomic snapshot; `mem` is read-only. See [Memory viewer](MEM.md) and [Memory layout](../reference/MEMORY-LAYOUT.md).

## 5. Observe network resources on Pico W (optional)

```text
piconix# net
piconix# net -r
piconix# cat /proc/net/memory
```

The standard Pico has no Wi-Fi. Pico W must be configured for networking to interpret these results. An empty ARP table does not necessarily mean a network failure. `/proc/net/memory` reports network-stack diagnostics. Continue with the [Networking lab](NETWORKING.md).

## Review

- What does each of `ps`, `lsof`, `trace`, and `mem` observe?
- How do `read()` and `write()` events relate to FDs?
- Why can observations of the live system change between commands?

Next, try the [Networking lab](NETWORKING.md) or write your own observable program using the [C programming guide](../development/apps/c/PROGRAMMING.md).

---

## Learning navigation

| ← Previous | ↑ Contents | Next → |
| :--- | :---: | ---: |
| [Memory and execution](MEMORY-EXECUTION.md) | [Intermediate course](INTERMEDIATE.md) | [Device I/O](DEVICE-IO.md) |
