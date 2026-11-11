# `samegame`: A/B/C Matching-Block Puzzle

`samegame` displays a 20×12 board of **three ASCII block types: A, B, and C**. Color support is not required. It fits an 80×24-character terminal. Select a group of at least two identical letters connected orthogonally (not diagonally) to remove the entire group. Remaining blocks fall downward, and empty columns move out as the remaining columns slide left.

```sh
samegame                 # New board using the SysTick-derived seed
samegame -s 1234         # Reproduce the board for seed 1234
```

| Input | Action |
|---|---|
| Arrow keys or `h`/`j`/`k`/`l` | Select a block |
| SPACE | Remove the selected connected group |
| `:a01`, `:t12` | Remove at that coordinate directly; row is always two digits, no Enter required |
| `r` | Restart (new seed by default; repeat with `-s`) |
| `q` | Quit |

The display includes `Seed`, `Score`, `Best`, `Best seed`, `Remaining`, `Groups removed`, `Selection`, `Group`, and `Invalid`. `Best` tracks the highest score reached during the process and updates as soon as the score improves. It survives `r` restarts; `Best seed` identifies the board where that score was reached. Both are reset when the program exits, with no file persistence. Columns run from `a` to `t` and rows from `01` to `12`. Always use a two-digit row in direct commands (e.g. `:b03`). `Group` shows the size of the selected connected component. Selecting a singleton or empty cell increases `Invalid` without altering the board. The round ends when there are no removable groups; clearing the entire board is not required.

Removing `n` blocks awards `(n−2)²` points. Clearing the entire board adds a 1000-point bonus. By default the program reads `/proc/ticks` to seed its small PRNG. A fixed `-s` value reproduces the same starting board.

For MCP, start the app using the persistent PTY, **not `execute_command`**. Send `:a01` as a `terminal_send` text argument to avoid cursor navigation. After removing a group, re-read the screen with `terminal_read` because gravity and column shifts change coordinates.

[Command index](README.md) | [日本語](../../../ja/reference/commands/SAMEGAME.md)
