# 4. Processes and tasks

[Learning path](LEARNING-PATH.md) | [日本語](../../ja/literacy/PROCESSES.md) | [Monitoring](MONITORING.md)

## Goal and prerequisites

**Goal:** Distinguish a program file from a running task and understand PID, foreground/background execution, and termination. USB is sufficient. PicoNIX has a fixed number of task slots and differs from Linux's general-purpose process model.

## Lab

```text
piconix# ps
piconix# blink &
piconix# ps
piconix# cat /proc/tasks
```

`blink` now runs in the background. Find its PID in `ps`. To stop it, use `kill PID`, **replacing `PID` with the numeric ID you actually observed**. The visible LED behavior varies by board. Avoid launching many background tasks and exhausting the fixed slots.

## Observe and explain

- How does a program in `/bin` differ from a task shown by `ps`?
- Why can the shell accept input while a task runs in the background?
- What happens when all task slots are occupied?

## Next step

Inspect FDs and system calls in [Monitoring](MONITORING.md), and learn about task switching in the [Architecture reference](../reference/ARCHITECTURE.md).
