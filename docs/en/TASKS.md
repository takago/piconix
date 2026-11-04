# Find documentation by goal — PicoNIX

**Document type: TASK INDEX (not a textbook chapter)** | [Textbook contents](learn/README.md) | [Technical reference](reference/README.md) | [Documentation directory](../DOCS.md) | [日本語](../ja/TASKS.md)

Use this index to find documentation by **what you want to do**. Use your browser Find command (Ctrl+F or equivalent) to search for a keyword such as “pipes”, “GPIO”, or “syscall”. Multiple links in each row are alternatives for different levels of detail, **not a required reading sequence**.

- **Textbook A/B/C:** learn concepts in order. Only textbook sections carry numbers such as `A1.1`.
- **How-to and lab:** try a command or observe behavior.
- **Reference, develop, and read:** inspect implementation details or APIs.

When coming here from a textbook chapter, use your browser Back action to resume that chapter. This task index has no textbook Previous/Next navigation. **Pico W networking is optional**; ordinary Pico hardware has no Wi-Fi. Check wiring and 3.3 V electrical precautions before attaching peripherals.

## Task index

### Getting started

| I want to… | Related pages |
|---|---|
| Run PicoNIX for the first time | [Setup](setup/SETUP.md) · [Textbook A1](learn/A/A1.md) · [Reference: wiring](reference/hardware/WIRING.md) |
| Build and flash a UF2 | [Setup](setup/SETUP.md) · [Textbook C1](learn/C/C1.md) |
| Connect through the USB console | [Setup](setup/SETUP.md) · [Textbook A1](learn/A/A1.md) · [Textbook B2](learn/B/B2.md) |
| Find available commands and their usage | [How-to: help](guides/HELP.md) · [Textbook A1](learn/A/A1.md) |
| Compare PicoNIX with Linux | [Textbook A1](learn/A/A1.md) · [Reference: differences from Linux](reference/DIFFERENCES.md) · [Reference: specification](reference/SPECIFICATION.md) |

### Shell, files, and pipes

| I want to… | Related pages |
|---|---|
| List and navigate directories | [Textbook A1](learn/A/A1.md) · [Reference: filesystem](reference/FILESYSTEM.md) |
| Create, copy, or remove files | [Textbook A2](learn/A/A2.md) · [Reference: filesystem](reference/FILESYSTEM.md) |
| Search text in files (grep) | [Textbook A2](learn/A/A2.md) · [Guide: pipes](guides/PIPE.md) |
| Use standard I/O and redirection | [Textbook A2](learn/A/A2.md) · [Reference: filesystem](reference/FILESYSTEM.md) |
| Connect commands with pipes | [Textbook A2](learn/A/A2.md) · [Guide: pipes](guides/PIPE.md) · [Textbook B1](learn/B/B1.md) |
| Edit text with edit or vi | [How-to: edit](guides/EDIT.md) · [How-to: vi](guides/VI.md) |
| Browse directories with fd | [How-to: fd](guides/FD.md) · [Textbook A1](learn/A/A1.md) |
| Copy binary data with dd | [How-to: dd](guides/DD.md) · [Reference: filesystem](reference/FILESYSTEM.md) |
| Inspect /etc/rc and initial files | [Reference: initial files and rc](reference/INITIAL-FILES.md) · [Textbook A1](learn/A/A1.md) · [Reference: filesystem](reference/FILESYSTEM.md) |

### Processes and terminals

| I want to… | Related pages |
|---|---|
| Inspect processes with ps and stop them with kill | [Textbook A3](learn/A/A3.md) · [Textbook B3](learn/B/B3.md) |
| Use background execution or Ctrl+C | [Textbook A3](learn/A/A3.md) · [Lab: TTY settings](guides/TTY.md) |
| Inspect open descriptors with lsof | [Textbook B1](learn/B/B1.md) · [Textbook B4](learn/B/B4.md) · [Reference: filesystem](reference/FILESYSTEM.md) |
| Use a separate shell on UART1 | [Textbook B2](learn/B/B2.md) · [Reference: UART1](reference/devices/UART.md) |
| Understand stty and raw/cooked input | [Textbook B2](learn/B/B2.md) · [Lab: TTY settings](guides/TTY.md) |
| Understand screen control and full-screen apps | [How-to: terminal screen](guides/TERMINAL.md) · [Lab: TTY settings](guides/TTY.md) · [Textbook B2](learn/B/B2.md) |

### Inspect OS internals

| I want to… | Related pages |
|---|---|
| Trace system calls | [Textbook B4](learn/B/B4.md) · [Lab: trace](guides/TRACE.md) · [Reference: syscalls](reference/SYSCALLS.md) |
| Inspect RAM and tasks with mem | [Textbook B3](learn/B/B3.md) · [How-to: mem](guides/MEM.md) · [Reference: SRAM layout](reference/MEMORY-LAYOUT.md) |
| Understand Flash, RAM, and PIC placement | [Textbook B3](learn/B/B3.md) · [Reference: SRAM layout](reference/MEMORY-LAYOUT.md) |
| Inspect kernel symbols | [Reference: kernel symbols](reference/KERNEL-SYMBOLS.md) · [How-to: mem](guides/MEM.md) · [Index: source functions](../FUNCTIONS.md) |
| Understand how VFS and FDs relate | [Textbook B1](learn/B/B1.md) · [Textbook C3](learn/C/C3.md) · [Reference: filesystem](reference/FILESYSTEM.md) |

### Devices and electronics

| I want to… | Related pages |
|---|---|
| Check GPIO pins and wiring | [Reference: pin assignments](reference/hardware/PINS.md) · [Reference: wiring](reference/hardware/WIRING.md) · [Textbook B5](learn/B/B5.md) |
| Control GPIO input/output | [Textbook B5](learn/B/B5.md) · [Reference: devices](reference/devices/DEVICES.md) |
| Control an 8×8 LED matrix | [Textbook B5](learn/B/B5.md) · [Reference: devices](reference/devices/DEVICES.md) · [Reference: LED wiring](reference/hardware/WIRING.md) |
| Try PWM, buzzer, or servo control | [Textbook B5](learn/B/B5.md) · [Reference: devices](reference/devices/DEVICES.md) · [Reference: wiring](reference/hardware/WIRING.md) |
| Read ADC inputs or sensors | [Textbook B5](learn/B/B5.md) · [Reference: devices](reference/devices/DEVICES.md) · [Reference: wiring](reference/hardware/WIRING.md) |
| Connect an I²C1 sensor | [How-to: I²C](reference/devices/I2C.md) · [Reference: wiring](reference/hardware/WIRING.md) · [Textbook B5](learn/B/B5.md) |

### Pico W networking (optional)

| I want to… | Related pages |
|---|---|
| Configure Wi-Fi AP or STA mode | [Textbook B6](learn/B/B6.md) · [How-to: Pico W connection](reference/network/NETWORK.md) · [Reference: Wi-Fi modes](reference/network/WIFI-MODES.md) |
| Inspect IP addresses and link state | [Textbook B6](learn/B/B6.md) · [How-to: net](reference/network/NET.md) · [Reference: Wi-Fi modes](reference/network/WIFI-MODES.md) |
| Use a remote shell over Telnet | [Textbook B6](learn/B/B6.md) · [How-to: Pico W connection](reference/network/NETWORK.md) · [How-to: net](reference/network/NET.md) |
| Try TCP/UDP with nc | [Textbook B6](learn/B/B6.md) · [How-to: nc](reference/network/NC.md) |
| Serve a web page with httpd | [How-to: httpd](reference/network/HTTPD.md) · [Reference: initial files and rc](reference/INITIAL-FILES.md) · [Textbook B6](learn/B/B6.md) |
| Inspect network traffic with ping or tcpdump | [Textbook B6](learn/B/B6.md) · [Reference: Pico W network](reference/network/NETWORK.md) |
| Control devices remotely over UDP | [Reference: remote device control](reference/network/REMOTE-DEVICE.md) · [Lab: remote GPIO](reference/network/REMOTE_GPIO.md) · [Textbook B6](learn/B/B6.md) |

### Programming and kernel development

| I want to… | Related pages |
|---|---|
| Use the lightweight Python interpreter | [Develop: Python](development/apps/python/PYTHON.md) · [Textbook C1](learn/C/C1.md) |
| Experiment with the Thumb assembler | [Develop: assembly](development/apps/asm/ASM.md) · [Textbook C1](learn/C/C1.md) |
| Write a new C command | [Textbook C1](learn/C/C1.md) · [Develop: C apps](development/apps/c/PROGRAMMING.md) · [Develop: C library](development/apps/c/LIBC.md) |
| Look up the user C library | [Develop: C library](development/apps/c/LIBC.md) · [Reference: syscalls](reference/SYSCALLS.md) |
| Write TCP/UDP socket applications (Pico W) | [Develop: network apps](development/apps/c/NETWORK-PROGRAMMING.md) · [Textbook C5](learn/C/C5.md) · [Reference: syscalls](reference/SYSCALLS.md) |
| Read the kernel in startup order | [Textbook C2](learn/C/C2.md) · [Read: kernel](development/kernel/KERNEL-READING-GUIDE.md) · [Reference: architecture](reference/ARCHITECTURE.md) |
| Read task switching and scheduling | [Textbook C2](learn/C/C2.md) · [Read: kernel](development/kernel/KERNEL-READING-GUIDE.md) · [Reference: internals](reference/INTERNAL.md) |
| Add a system call | [Textbook C3](learn/C/C3.md) · [Develop: syscall](development/syscalls/SYSCALL-DEVELOPMENT.md) · [Reference: syscalls](reference/SYSCALLS.md) |
| Add a device driver | [Textbook C4](learn/C/C4.md) · [Develop: driver](development/drivers/DRIVER-DEVELOPMENT.md) · [Reference: devices](reference/devices/DEVICES.md) |
| Modify Wi-Fi, lwIP, or kernel networking (Pico W) | [Develop: kernel networking](development/network/NETWORK-DEVELOPMENT.md) · [Textbook C5](learn/C/C5.md) · [Reference: networking](reference/network/README.md) |
| Make and validate a small kernel change | [Textbook C6](learn/C/C6.md) · [Develop: kernel](development/kernel/KERNEL-DEVELOPMENT.md) · [Read: kernel](development/kernel/KERNEL-READING-GUIDE.md) |
| Find source code by function name | [Index: source functions](../FUNCTIONS.md) · [Read: kernel](development/kernel/KERNEL-READING-GUIDE.md) |

## Other ways to navigate

- **Follow a course:** [Textbook A/B/C](learn/README.md)
- **Browse command examples:** [Supplementary guides and labs](guides/README.md)
- **Look up exact specifications:** [Technical reference](reference/README.md)
- **Find implementation guides:** [Development guides](development/README.md)
- **Troubleshoot build or flashing:** [Setup and troubleshooting](setup/SETUP.md)
