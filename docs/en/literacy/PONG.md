# Pong

PicoNIX Pong supports a single-terminal game against the CPU and a two-player game using `/dev/console` (USB CDC) and `/dev/ttyS1`. Either terminal can launch the game.

## Running

```text
piconix# pong       # Single player vs CPU
piconix# pong -2    # Two players on two terminals
```

**Single-player mode** uses only the launching terminal. It works even if another program is using the other terminal. The CPU paddle tracks the ball at a limited speed; difficulty is fixed.

**Two-player mode** temporarily claims the other terminal via `OS_TTY_CLAIM`. It refuses to start if that terminal is in use. Each terminal displays its own paddle on the right and the opponent's paddle on the left.

The game screen is centered using the launching terminal’s detected size. In two-player mode, both terminals use the same coordinates; a differently sized second terminal may not appear centered. Resize during a game is not supported.

## Controls

- **Up arrow**: move up
- **Down arrow**: move down
- `W`/`I` and `S`/`K`: alternative movement keys
- `Q`: quit

Arrow keys use ANSI escape sequences (`ESC [ A` / `ESC [ B`, or `ESC O A` / `ESC O B`). Each terminal decodes input independently in two-player mode.

Disable line-input mode in the terminal emulator. ANSI cursor control and an alternate screen are required.

## TTY handling and limitations

While running, Pong configures the TTYs it uses for noncanonical, echo-disabled, nonblocking input. On normal exit, it restores local settings and releases the claimed remote TTY in two-player mode. Claiming is a generic kernel TTY facility, not Pong-specific handling.

The game cannot detect whether a terminal emulator is physically connected. Each application uses a 24-KiB RAM slot, with the last 4 KiB reserved for its stack (leaving up to 20 KiB for the program image). Firmware builds and hardware behavior require separate verification.

---

## Learning navigation

**Supplementary page:** Previous returns to its related main lesson; Next resumes the core learning path.

| ← Previous | ↑ Contents | Next → |
| :--- | :---: | ---: |
| [Processes](PROCESSES.md) | [Beginner course](BEGINNER.md) | [Intermediate course](INTERMEDIATE.md) |
