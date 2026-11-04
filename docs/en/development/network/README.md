# Network Development Guides

**Document type: DEVELOPMENT GUIDE INDEX (Pico W kernel-side networking)** | [Development guides](../README.md) | [Technical reference](../../reference/network/README.md) | [日本語](../../../ja/development/network/README.md)

Start here to **read or modify the kernel-side networking implementation**. For sequential learning, use [Textbook C5](../../learn/C/C5.md); for current connection behavior and command specifications, use the [network reference](../../reference/network/README.md).

## Find the right guide

| I want to… | Read this |
|---|---|
| Understand or extend kernel Wi-Fi, lwIP, and sockets | [Kernel networking implementation and extension](NETWORK-DEVELOPMENT.md) |
| Write a **user-space C TCP/UDP application** | [Socket API and network applications](../apps/c/NETWORK-PROGRAMMING.md) |
| Trace the kernel from boot | [Kernel Reading Guide](../kernel/KERNEL-READING-GUIDE.md) |
| Look up Telnet, Wi-Fi, and network behavior | [Network technical reference](../../reference/network/README.md) |
| Learn networking in order | [Textbook B6](../../learn/B/B6.md) → [Textbook C5](../../learn/C/C5.md) |

**Ownership rule:** `development/network/` describes **kernel implementation and modification**. `development/apps/c/NETWORK-PROGRAMMING.md` describes **the API exposed to user-space programs and application examples**. Current behavior and limits are specified in `reference/network/` and the [syscall reference](../../reference/SYSCALLS.md).

Ordinary Pico boards have no Wi-Fi. Kernel networking work and hardware labs are optional Pico W topics.
