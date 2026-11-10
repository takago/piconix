# reversi

`reversi` is an interactive 8×8 Reversi game against the built-in computer opponent. It uses the same TTY interface on a physical terminal and an MCP-managed PTY.

```sh
reversi
```

Move the selection with arrow keys (or `h`, `j`, `k`, `l`), press SPACE to place a black stone, and `q` to quit. Illegal moves are rejected. The computer selects a legal reply using a small deterministic board evaluation. Passes are automatic and the game ends when neither side can play.

For MCP use `terminal_open` to launch `reversi`, `terminal_read` to inspect the screen, and `terminal_send` to send keys. Because `terminal_send` does not wait for rendering, use `terminal_read` to check the turn when needed. The game contains no MCP-specific code.

The most recent move is marked `[B]` or `[W]` using the existing three-character cell width. SGR 7 reverse video still indicates the cursor. After Black moves, the game displays `Turn: COM` and `COM is thinking...` for approximately one second before rendering the White reply. On a pass, the latest placed stone remains marked. MCP clients should wait until `Turn: PLAYER` appears before making the next move.

An invalid placement is counted when SPACE is pressed on an occupied square or on an empty square that flips no stones. The screen displays separate counts as `Invalid moves: PLAYER n  COM m`, including after game over. Moving the cursor and automatic passes do not count. The COM selects only legal moves, so its count is normally zero.
