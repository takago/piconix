# `sudoku`: 4×4 and 9×9 Sudoku

`sudoku` is a single-player number-placement puzzle. Fill every row, column, and box without duplicates. The default is 9×9 (digits 1–9 and 3×3 boxes); `-4` selects 4×4 (digits 1–4 and 2×2 boxes).

```sh
sudoku                 # Default 9x9, SysTick-derived seed
sudoku -4              # Introductory 4x4
sudoku -4 -s 1234      # Repeat the same 4x4 puzzle
sudoku -s 1234         # Repeat the same 9x9 puzzle
```

Each size has one **unique-solution base puzzle**. The seed permutes digits, rows within bands, bands, columns within stacks, stacks, and optionally transposes the puzzle. These transformations preserve the unique solution; they do not generate an unlimited family of independent puzzles. Given cells cannot be modified. Any in-range digit can be entered into an editable cell, including a duplicate; the program does not compare each entry with the hidden answer. `Conflicts` counts cells involved in duplicates in any row, column, or box. `Rejected` counts attempts to edit givens or enter out-of-range digits. Completing a full grid with no conflicts solves the puzzle.

The ASCII display distinguishes given digits (` 5 `), empty editable cells (`[ ]`), and player-entered digits (`[3]`), even in MCP `terminal_read` text without color or reverse video. `Cursor` displays the selected coordinate.


Example 4×4 display (`-s 12345`). Fixed digits have no brackets; only editable cells are bracketed.

```text
     a   b     c   d
   +--------+ --------+
 1 |[ ]  3  |  2  [ ] |
 2 | 4  [ ] | [ ]  1  |
   +--------+ --------+
 3 | 2  [ ] |  1  [ ] |
 4 |[ ]  1  | [ ]  2  |
   +--------+ --------+
```

| Input | Action |
|---|---|
| Arrow keys or `h`/`j`/`k`/`l` | Select a cell |
| `1`–`9` (only `1`–`4` on 4×4) | Enter a digit |
| `0` or SPACE | Clear an editable cell |
| `:a1=2` | Enter `2` directly at `a1`, or `0` to erase; no Enter required |
| `r` | Reset puzzle and counters |
| `q` | Quit and restore the terminal |

Without `-s`, `r` reads `/proc/ticks` (1 ms SysTick counter) again to create a different seeded arrangement. With `-s`, `r` repeats the same puzzle. The actual seed is displayed. The PRNG is not cryptographic.

For MCP, start in the persistent PTY using `terminal_send` and inspect the display using `terminal_read`; **do not use `execute_command`** for this interactive app. A single `terminal_send` text argument containing `:a1=2` performs a direct move; verify the result on screen.

[Command index](README.md) | [日本語](../../../ja/reference/commands/SUDOKU.md)
