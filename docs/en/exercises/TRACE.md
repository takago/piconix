# Kernel trace exercises

Use these exercises after reading the [trace usage guide](../guides/TRACE.md). See the guide for command syntax and record formats.

## SVC and asynchronous execution

These exercises assume a build with `PICONIX_KERNEL_FUNCTION_TRACE=ON`. An ordinary build still records system calls, SysTick events and task switches, but does not show `KERNEL` function records. Actual timestamps, PIDs and call sequences vary.

### Exercise 1: system calls and selected kernel functions

```text
piconix# trace run hello
piconix# trace run ls
piconix# trace run python test.py
```

Compare `SYSCALL` and `KERNEL` records to follow calls from a system call into selected VFS or driver functions. An entry is shown as the function name and an exit with a trailing `/`. Adjacent entry/exit records may be shown as one `(name)` row. Without `-a`, function records are limited to SVC execution. `trace run` displays only records associated with the launched PID.

### Exercise 2: ICMP device activity during ping

```text
piconix# trace run ping 127.0.0.1
```

Look for `open`, `write` and `read` calls involving `/dev/net/icmp`. In instrumented builds, `net_echo_write`, `net_echo_read` and `net_echo_close` may appear if actually called in SVC context. This does not trace every asynchronous stage of ICMP reception.

### Exercise 3: observe other contexts using two terminals

Use **two terminals**, such as USB CDC plus UART1 or Telnet.

Terminal A:

```text
piconix# trace on -a
piconix# trace follow
```

Terminal B:

```text
piconix# ping 127.0.0.1
piconix# ls
```

Look for `SYSCALL`, `SWITCH`, `TIMER` and `KERNEL` records in terminal A. The `-a` option widens capture of **selected instrumented kernel functions** beyond SVC. Asynchronous and interrupt-context function events have no PID and appear as `-`. Functions not selected or instrumented, such as `net_capture_poll` and `echo_received`, are not recorded. The periodic SysTick `TICKx1K` record is an ordinary event and does not depend on `-a`.

From another terminal, stop capture:

```text
piconix# trace off
```

`trace follow` drains the remaining records and exits. Note that `trace show` also stops capture before displaying history.

### Exercise 4: compare with `trace run -a`

```text
piconix# trace run -a ping 127.0.0.1
```

The `-a` option **records** selected functions in all contexts, but the resulting output **displays** records for the launched ping PID only. To inspect PID-less asynchronous functions, use `trace on -a` with `trace follow` as in exercise 3. Keep captures short: the 512-event ring buffer overwrites old records when full.

## files, devices, networking, and scheduling

Use `trace run` for short-lived programs and `trace on` plus `trace follow` from another terminal for long-running servers. `trace run` prints its report only after the target terminates. Stop a live capture using `trace off` from another terminal.

### Exercise 5: RAM files and VFS

Prepare a small RAM file first:

```text
piconix# echo hello > /tmp/test.txt
piconix# trace run cat /tmp/test.txt
```

Look for `open`, `read`, `write`, and `close`, and compare them to selected `KERNEL` calls such as `vfs_rw`. A compact `(in_task_range)` row represents consecutive entry and exit records at the same recorded time. The two underlying events remain in the buffer.

### Exercise 6: Python file operations and ADC

```text
piconix# trace run python test.py
```

The bundled `test.py` reads the on-chip temperature sensor five times. Python assignments and integer arithmetic run in user space and do not themselves generate `SYSCALL` events. Observe device-file `open`, `read`, `write`, and `close`, together with any waiting syscalls. Check the contents of `/tmp/test.py` when interpreting the result.

### Exercise 7: BH1750 and I2C ioctl

With a BH1750 connected, limit the program to one measurement:

```text
piconix# trace run bh1750 1
```

Observe `open` for `/dev/i2c/1`, `ioctl` for the transfer, and `close` during cleanup. In an instrumented build, look for `i2c_driver_transfer` when the corresponding SVC path runs. A sensor-free board cannot be expected to return a valid measurement. Without a count argument, `bh1750` measures continuously, so supply a count when using `trace run`.

### Exercise 8: HTTP server, select, accept, and read

Start the server on terminal A:

```text
piconix# httpd
```

On terminal B:

```text
piconix# trace on
piconix# trace follow
```

Access the Pico W HTTP server from a PC browser. Look for `select`, `accept`, `read`, and `write`. Relate the count of ready descriptors returned by `select` to subsequent `accept` and `read` operations. Because `httpd` normally continues running, do not use `trace run httpd` in this exercise.

### Exercise 9: blocking accept versus I/O multiplexing

On terminal A:

```text
piconix# tcp_echod 7000
```

Capture events using `trace on` and `trace follow` on another terminal, and connect with an external TCP client on port 7000 to exchange data. Compare events during idle waiting and after connections or data arrive. Next, compare with communication through `nc` to study blocking on one socket versus using `select` for multiple descriptors. Confirm the precise wait states in the user-space and kernel sources; the trace alone may not identify every internal state transition.

### Exercise 10: PID filtering and SysTick

```text
piconix# ps
piconix# trace on
piconix# trace follow
```

Find periodic `TIMER TICKx1K` events and `SWITCH` records. `TICKx1K` reports every 1,000 ticks of the 1 ms SysTick; it does not report every interrupt.

To focus on one program, obtain its PID using `ps`, then use `trace follow PID` or `trace show PID`. A PID argument is a **display filter**, not a capture filter, and does not reduce ring-buffer usage. `SWITCH` records are filtered by destination PID. To inspect PID-less asynchronous function events, use `trace on -a` with unfiltered `trace follow`.

### Lab caveats

- A build with `PICONIX_KERNEL_FUNCTION_TRACE=OFF` produces no `KERNEL` function rows.
- `-a` does **not** enable every kernel function. It admits non-SVC contexts only for selected and instrumented functions.
- The ring buffer holds 512 events and overwrites old entries. Start with short captures.
- `Ret` for function events is not the internal function's return value; distinguish it from syscall return values.
- Tracing adds runtime overhead, so displayed timestamps are not a precise performance benchmark.

---

[Exercise index](README.md) | [Trace guide](../guides/TRACE.md) | [日本語](../../ja/exercises/TRACE.md)
