# Development Guides
**Required background:** Familiarity with C, UNIX and OS concepts such as processes, FDs and syscalls is assumed. These guides focus on reading and changing the actual implementation. Before editing, consult [Differences from Linux](../reference/DIFFERENCES.md) and [Memory layout](../reference/MEMORY-LAYOUT.md).


These documents describe current APIs, implementation details, extension procedures and complete code examples. Choose a reference by topic.

**Find by goal:** [Task index](../TASKS.md).

## User-space C and applications

- [Developing C applications for PicoNIX](apps/c/PROGRAMMING.md)
- [PicoNIX C library and application APIs](apps/c/LIBC.md)
- [Socket API and network applications](apps/c/NETWORK-PROGRAMMING.md)
- [Developing the `mcpd` MCP server](apps/c/MCP-DEVELOPMENT.md)

## Python and assembly

- [PicoNIX Tiny Python — User Guide](apps/python/PYTHON.md)
- [Device Programming with Tiny Python](apps/python/DEVICE-PROGRAMMING.md)
- [TCP/UDP Socket Programming with Tiny Python (Pico W)](apps/python/SOCKET-PROGRAMMING.md)
- [PicoNIX Tiny Thumb Assembler](apps/asm/ASM.md)

## Kernel source and extension references

- [Kernel source and call-path reference](kernel/KERNEL-READING-GUIDE.md)
- [Kernel Development Guide](kernel/KERNEL-DEVELOPMENT.md)
- [Screen Management Module (ANSI, CDC and PTY)](kernel/SCREEN-MANAGEMENT.md)

## System calls

- [System Call Development](syscalls/SYSCALL-DEVELOPMENT.md)

## Device drivers

- [Device Driver Development](drivers/DRIVER-DEVELOPMENT.md)

## Kernel networking (Pico W)

- [Network development guide index](network/README.md)
- [Wi-Fi, lwIP, sockets and kernel service implementation](network/NETWORK-DEVELOPMENT.md)

**Source index:** [Function index](FUNCTIONS.md). Line locations change with code revisions.

**Implementation examples and extension procedures:** [Build UDP/TCP clients](apps/c/NETWORK-PROGRAMMING.md) / [Add a system call](syscalls/SYSCALL-DEVELOPMENT.md) / [Add a device node and driver](drivers/DRIVER-DEVELOPMENT.md). Each includes preparation, source, build, execution and checks.

---

**Related links:** [日本語](../../ja/development/README.md)
