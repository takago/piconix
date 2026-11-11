# `reversi`: Reversi

`reversi` is an interactive 8×8 game against the built-in opponent. By default PLAYER is Black (`B`, first) and COM is White (`W`, second). With `-c`, COM plays Black first and PLAYER plays White second. It uses the same terminal interface on physical TTYs and MCP PTYs. **Do not launch it through noninteractive `execute_command`.**

```sh
reversi            # PLAYER first (Black)
reversi -c         # COM first (Black), PLAYER second (White)
```

## Controls

| Key | Action |
|---|---|
| Arrow keys or `h`/`j`/`k`/`l` | Move the cursor (left/down/up/right) |
| SPACE | Place your stone at the cursor |
| `r`/`R` | Restart, keeping the starting-side option |
| `q`/`Q` | Quit |

A legal placement must flip at least one opponent stone. SPACE on an occupied square or a square that flips nothing leaves the board unchanged and increases PLAYER's invalid-move count.

## Board and screen

- `B` is Black, `W` is White, `.` is empty; columns are a–h and rows 1–8.
- The latest move is marked `[B]` or `[W]`; the cursor is highlighted with ANSI reverse video (SGR 7). A pass leaves the last placed stone marked.
- The screen shows stone totals, number of moves, turn, and `Invalid moves: PLAYER n  COM m`. Cursor movement and automatic passes are not invalid moves. COM chooses legal moves, so its count is normally zero.
- After a legal PLAYER move, the screen briefly shows `Turn: COM` and `COM is thinking...`, then displays COM's reply approximately one second later. `Turn: PLAYER` indicates the next player turn.

## Passes and game over

Either player passes automatically when no legal move exists. COM may take multiple consecutive turns when PLAYER has to pass. The game ends when neither side can move, displaying the winner or a draw. Press `r` to restart with the same sides, or `q` to exit.

## Controlling the game via MCP

Start with `terminal_send({"text":"reversi"})` (or `reversi -c` for COM first) and `terminal_send({"key":"ENTER"})`, inspect the 80×24 screen with `terminal_read({})`, and send keys with `terminal_send({"key":"SPACE"})`, etc. **A successful send does not imply that COM has finished drawing or responding.** Call `terminal_read({})` again to inspect `Turn: PLAYER` and the board. Press `q` to exit to the persistent PTY shell. There is no MCP-specific code in the game.

[Command index](README.md) | [MCP](MCP.md) | [日本語](../../../ja/reference/commands/REVERSI.md)
