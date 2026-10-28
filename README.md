# PicoNIX

Choose a language / 言語を選択してください．

- [日本語 — PicoNIXの概要](README-JP.md)
- [English — PicoNIX overview](README-EN.md)

- [UART1・端末操作（日本語）](UART-JP.md)
- [UART1 and terminals (English)](UART-EN.md)

### Pinout command

Run `pins` to display the Raspberry Pi Pico 40-pin header as an ASCII diagram (USB connector at top), with live GPIO direction/level, framebuffer ownership, UART, ADC, and PWM duty/frequency from existing GPIO/PWM devices. Labels are obtained dynamically from existing GPIO/PWM devices; framebuffer ownership and PWM settings reflect the current state.

### Process inspection

`cat /proc/fb` displays the live 8x8 framebuffer pixels and enable state.
`cat /proc/tasks` lists running tasks; `cat /proc/task/N` shows the task's PCB fields, saved context (when available), and open file descriptors. `ls /proc/task` lists active PIDs. These interfaces are read-only. Register values are saved context, not a live CPU snapshot.
