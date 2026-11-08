# Find documentation by goal — PicoNIX

Find documentation for **what you want to do**. Each task links to starting explanations, practical instructions, and detailed specifications. Open only the pages you need.

**How to search:** Browse the categories below or use your browser's Find command (Ctrl+F or equivalent) for words such as “pipes”, “GPIO”, or “system calls”.

**Note:** Pico W networking is unavailable on a standard Pico. Check the wiring instructions and 3.3 V precautions before connecting peripherals.


## Task index

### Getting started

| I want to… | Related pages |
|---|---|
| Run PicoNIX for the first time | [Setup](setup/SETUP.md) · [Basic commands](reference/commands/HELP.md) · [Reference: wiring](reference/hardware/WIRING.md) |
| Build and flash a UF2 | [Setup](setup/SETUP.md) · [C programming](development/apps/c/PROGRAMMING.md) |
| Connect through the USB console | [Setup](setup/SETUP.md) · [Basic commands](reference/commands/HELP.md) · [TTY guide](reference/commands/TTY.md) |
| Find available commands and their usage | [How-to: help](reference/commands/HELP.md) |
| Compare PicoNIX with Linux | [Basic commands](reference/commands/HELP.md) · [Reference: differences from Linux](reference/DIFFERENCES.md) · [Reference: specification](reference/SPECIFICATION.md) |

### Shell, files, and pipes

| I want to… | Related pages |
|---|---|
| List and navigate directories | [Basic commands](reference/commands/HELP.md) · [Reference: filesystem](reference/FILESYSTEM.md) |
| Create, copy, or remove files | [Pipes and FDs](reference/commands/PIPE.md) · [Reference: filesystem](reference/FILESYSTEM.md) |
| Search text in files (grep) | [Pipes and FDs](reference/commands/PIPE.md) |
| Use standard I/O and redirection | [Pipes and FDs](reference/commands/PIPE.md) · [Reference: filesystem](reference/FILESYSTEM.md) |
| Connect commands with pipes | [Pipes and FDs](reference/commands/PIPE.md) · [File system](reference/FILESYSTEM.md) |
| Edit text with edit or vi | [How-to: edit](reference/commands/EDIT.md) · [How-to: vi](reference/commands/VI.md) |
| Browse directories with fd | [How-to: fd](reference/commands/FD.md) · [Basic commands](reference/commands/HELP.md) |
| Copy binary data with dd | [How-to: dd](reference/commands/DD.md) · [Reference: filesystem](reference/FILESYSTEM.md) |
| Inspect /etc/rc and initial files | [Reference: initial files and rc](reference/INITIAL-FILES.md) · [Basic commands](reference/commands/HELP.md) · [Reference: filesystem](reference/FILESYSTEM.md) |

### Processes and terminals

| I want to… | Related pages |
|---|---|
| Inspect processes with ps and stop them with kill | [System specifications](reference/SPECIFICATION.md) · [mem guide](reference/commands/MEM.md) |
| Use background execution or Ctrl+C | [System specifications](reference/SPECIFICATION.md) · [Lab: TTY settings](reference/commands/TTY.md) |
| Inspect open descriptors with lsof | [File system](reference/FILESYSTEM.md) · [trace guide](reference/commands/TRACE.md) |
| Use a separate shell on UART1 | [TTY guide](reference/commands/TTY.md) · [Reference: UART1](reference/devices/UART.md) |
| Understand stty and raw/cooked input | [TTY guide](reference/commands/TTY.md) |
| Understand screen control and full-screen apps | [How-to: terminal screen](reference/commands/TERMINAL.md) · [Lab: TTY settings](reference/commands/TTY.md) |

### Inspect OS internals

| I want to… | Related pages |
|---|---|
| Trace system calls | [trace guide](reference/commands/TRACE.md) · [Reference: syscalls](reference/SYSCALLS.md) |
| Inspect RAM and tasks with mem | [mem guide](reference/commands/MEM.md) · [Reference: SRAM layout](reference/MEMORY-LAYOUT.md) |
| Understand Flash, RAM, and PIC placement | [mem guide](reference/commands/MEM.md) · [Reference: SRAM layout](reference/MEMORY-LAYOUT.md) |
| Inspect kernel symbols | [Reference: kernel symbols](reference/KERNEL-SYMBOLS.md) · [How-to: mem](reference/commands/MEM.md) · [Index: source functions](../ja/development/FUNCTIONS.md) |
| Understand how VFS and FDs relate | [File system](reference/FILESYSTEM.md) · [System call development](development/syscalls/SYSCALL-DEVELOPMENT.md) |

### Devices

Use `/dev/fbchar` to display ASCII characters or icons ([device reference](reference/devices/DEVICES.md)); use [scroll](reference/commands/SCROLL.md) for longer messages and [LLM/MCP](reference/commands/MCP.md) for remote tool calling.
 and electronics

| I want to… | Related pages |
|---|---|
| Check GPIO pins and wiring | [Reference: pin assignments](reference/hardware/PINS.md) · [Reference: wiring](reference/hardware/WIRING.md) · [Device reference](reference/devices/DEVICES.md) |
| Control GPIO input/output | [Device reference](reference/devices/DEVICES.md) |
| Control an 8×8 LED matrix | [Device reference](reference/devices/DEVICES.md) · [Reference: LED wiring](reference/hardware/WIRING.md) |
| Try PWM, buzzer, or servo control | [Device reference](reference/devices/DEVICES.md) · [Reference: wiring](reference/hardware/WIRING.md) |
| Read ADC inputs or sensors | [Device reference](reference/devices/DEVICES.md) · [Reference: wiring](reference/hardware/WIRING.md) |
| Connect an I²C1 sensor | [How-to: I²C](reference/devices/I2C.md) · [Reference: wiring](reference/hardware/WIRING.md) · [Device reference](reference/devices/DEVICES.md) |

### Pico W networking (optional)

| I want to… | Related pages |
|---|---|
| Configure Wi-Fi AP or STA mode | [Network reference](reference/network/NETWORK.md) · [Reference: Wi-Fi modes](reference/network/WIFI-MODES.md) |
| Inspect IP addresses and link state | [Network reference](reference/network/NETWORK.md) · [How-to: net](reference/network/NET.md) · [Reference: Wi-Fi modes](reference/network/WIFI-MODES.md) |
| Use a remote shell over Telnet | [Network reference](reference/network/NETWORK.md) · [How-to: net](reference/network/NET.md) |
| Try TCP/UDP with nc | [Network reference](reference/network/NETWORK.md) · [How-to: nc](reference/network/NC.md) |
| Serve a web page with httpd | [How-to: httpd](reference/network/HTTPD.md) · [Reference: initial files and rc](reference/INITIAL-FILES.md) · [Network reference](reference/network/NETWORK.md) |
| Build and test an MCP server (Pico W) | [Unified MCP guide (including development lab)](reference/commands/MCP.md) |
| Inspect network traffic with ping or tcpdump | [`ip`](reference/network/IP.md) · [`ping`](reference/network/PING.md) · [`tcpdump`](reference/network/TCPDUMP.md) |
| Control devices remotely over UDP | [Reference: remote device control](reference/network/REMOTE-DEVICE.md) · [Lab: remote GPIO](reference/network/REMOTE_GPIO.md) · [Network reference](reference/network/NETWORK.md) |

### Programming and kernel development

| I want to… | Related pages |
|---|---|
| Use the lightweight Python interpreter | [Develop: Python](development/apps/python/PYTHON.md) · [C programming](development/apps/c/PROGRAMMING.md) |
| Experiment with the Thumb assembler | [Develop: assembly](development/apps/asm/ASM.md) · [C programming](development/apps/c/PROGRAMMING.md) |
| Write a new C command | [C programming](development/apps/c/PROGRAMMING.md) · [Develop: C library](development/apps/c/LIBC.md) |
| Look up the user C library | [Develop: C library](development/apps/c/LIBC.md) · [Reference: syscalls](reference/SYSCALLS.md) |
| Write TCP/UDP socket applications (Pico W) | [Develop: network apps](development/apps/c/NETWORK-PROGRAMMING.md) · [Network development](development/network/NETWORK-DEVELOPMENT.md) · [Reference: syscalls](reference/SYSCALLS.md) |
| Read the kernel in startup order | [Kernel reading guide](development/kernel/KERNEL-READING-GUIDE.md) · [Reference: architecture](reference/ARCHITECTURE.md) |
| Read task switching and scheduling | [Kernel reading guide](development/kernel/KERNEL-READING-GUIDE.md) · [Reference: internals](reference/INTERNAL.md) |
| Add a system call | [System call development](development/syscalls/SYSCALL-DEVELOPMENT.md) · [Reference: syscalls](reference/SYSCALLS.md) |
| Add a device driver | [Driver development](development/drivers/DRIVER-DEVELOPMENT.md) · [Reference: devices](reference/devices/DEVICES.md) |
| Modify Wi-Fi, lwIP, or kernel networking (Pico W) | [Develop: kernel networking](development/network/NETWORK-DEVELOPMENT.md) · [Reference: networking](reference/network/README.md) |
| Make and validate a small kernel change | [Kernel development](development/kernel/KERNEL-DEVELOPMENT.md) · [Read: kernel](development/kernel/KERNEL-READING-GUIDE.md) |
| Find source code by function name | [Index: source functions](../ja/development/FUNCTIONS.md) · [Read: kernel](development/kernel/KERNEL-READING-GUIDE.md) |

## Other ways to navigate

- **Follow a course:** [Learning roadmap](ROADMAP.md)
- **Browse command examples:** [Command reference](reference/commands/README.md)
- **Look up exact specifications:** [Technical reference](reference/README.md)
- **Find implementation guides:** [Development guides](development/README.md)
- **Troubleshoot build or flashing:** [Setup and troubleshooting](setup/SETUP.md)

## Hands-on networking and kernel extension

| I want to... | Start with | Details |
|---|---|---|
| Inspect my IP | [`ip` lab](reference/network/IP.md) | [Network connection](reference/network/NETWORK.md) |
| Check IP reachability | [`ping` lab](reference/network/PING.md) | [`net` diagnostics](reference/network/NET.md) |
| Interpret TCP/UDP/ICMP traffic | [`tcpdump` lab](reference/network/TCPDUMP.md) | [Network reference](reference/network/NETWORK.md) |
| Write a TCP/UDP C program | [Complete client examples](development/apps/c/NETWORK-PROGRAMMING.md) | [Register an application](development/apps/c/PROGRAMMING.md) |
| Add a syscall | [getuptime lab](development/syscalls/SYSCALL-DEVELOPMENT.md) | [SVC reference](reference/SYSCALLS.md) |
| Add a node and driver | [/dev/uptime lab](development/drivers/DRIVER-DEVELOPMENT.md) | [Devices](reference/devices/DEVICES.md) |

[日本語](../ja/TASKS.md) | [Documentation directory](README.md)
