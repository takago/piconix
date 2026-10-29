# Pong

PicoNIX Pong supports a single-terminal game against the CPU and a two-player game using `/dev/console` (USB CDC) and `/dev/uart1`. Either terminal can launch the game.

## Running

```text
piconix# pong       # Single player vs CPU
piconix# pong -2    # Two players on two terminals
```

**Single-player mode** uses only the launching terminal. It works even if another program is using the other terminal. The CPU paddle tracks the ball at a limited speed; difficulty is fixed.

**Two-player mode** temporarily claims the other terminal via `OS_TTY_CLAIM`. It refuses to start if that terminal is in use. Each terminal displays its own paddle on the right and the opponent's paddle on the left.

## Controls

- `W` or `I`: move up
- `S` or `K`: move down
- `Q`: quit

Disable line-input mode in the terminal emulator. ANSI cursor control and an alternate screen are required.

## TTY handling and limitations

While running, Pong configures the TTYs it uses for noncanonical, echo-disabled, nonblocking input. On normal exit, it restores local settings and releases the claimed remote TTY in two-player mode. Claiming is a generic kernel TTY facility, not Pong-specific handling.

The game cannot detect the physical connection of a terminal emulator. Full firmware builds, the 32-KiB RAM image limit, and hardware behavior still require verification.
