# `tictactoe`: Tic-Tac-Toe

`tictactoe` is a 3×3 game against COM. By default PLAYER uses `X` and moves first; with `-c`, COM uses `X` and moves first while PLAYER uses `O`. Three aligned marks win; a full board without a winner is a draw. COM exhaustively searches all legal moves and cannot be defeated if it plays correctly.

```sh
tictactoe          # PLAYER first (X)
tictactoe -c       # COM first (X), PLAYER second (O)
```

| Key | Action |
|---|---|
| Arrow keys or `h`/`j`/`k`/`l` | Select a square (left/down/up/right) |
| SPACE | Place your mark in an empty square |
| `r` | Restart with the same starting-side option |
| `q` | Quit |

Columns are `a`–`c`, rows `1`–`3`. The last played mark is shown as `[X]` or `[O]`, and the selected square uses reverse video. The display includes move count and `Invalid moves: PLAYER n  COM 0`; selecting an occupied square increments the invalid counter. COM replies after approximately 400 ms.

For MCP use the persistent PTY with `terminal_send` and `terminal_read`; **do not use `execute_command`**. Submit SPACE with `terminal_send({"key":"SPACE"})` and read the screen again to confirm COM has completed its reply and `Turn: PLAYER` is shown.

[Command index](README.md) | [日本語](../../../ja/reference/commands/TICTACTOE.md)
