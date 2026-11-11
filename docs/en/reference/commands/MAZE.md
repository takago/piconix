# `maze`: Maze Game

`maze` generates a 21×9 maze. Move `@` from the start (1,1) to goal `G` (17,7); `#` marks a wall. The maze has no computer opponent, which helps isolate an agent's screen recognition, route planning, and terminal input accuracy.

```sh
maze             # New maze seeded by /proc/ticks (1 ms SysTick)
maze -s 1234     # Reproduce a particular maze
```

On launch and when restarting in default mode, `maze` reads the kernel's `/proc/ticks` and uses it as its pseudorandom seed. The visible `Seed` field allows a map to be reproduced later using `maze -s <Seed>`. If the tick value has not changed, the next seed is incremented. With `-s`, pressing `r` **replays the same maze**. SysTick is not a cryptographic source of randomness.

A randomized depth-first search generates a connected perfect maze, so the goal is always reachable. A breadth-first search (BFS) computes `Shortest` for each generated map.

| Key | Action |
|---|---|
| Arrow keys or `h`/`j`/`k`/`l` | Move one cell |
| `r` | Generate a new maze by default; restart the same map with `-s`; reset counters |
| `q` | Quit |

Coordinates use (0,0) at the upper-left. `Player: (x,y)` shows the current location. `Moves` counts **successful movements**; entering a wall increments `Wall hits` without moving. `Shortest` is the shortest route for that maze. At the goal, `Extra moves` and integer `Efficiency` (`100 × Shortest / Moves`) are displayed. Movement stops at the goal until restart.

For MCP, launch from the persistent PTY shell using `terminal_send` and inspect the screen with `terminal_read`; **do not launch through `execute_command`**. Record the displayed seed to reproduce an agent's trial.

[Command index](README.md) | [日本語](../../../ja/reference/commands/MAZE.md)
