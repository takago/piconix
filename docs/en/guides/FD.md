# fd — file browser

*Usage guide*

`fd` is a small, FDclone-inspired full-screen browser for navigating directories and viewing files from a terminal. It is not a full replacement for a Linux file manager.

## Start the browser

```sh
fd
fd /etc
fd /proc
```

Without an argument, `fd` opens `/tmp`. Supply a directory to start elsewhere. An **ANSI-capable terminal with cursor-key support** is required.

## Navigate the file list

| Key | Action |
|---|---|
| `↑` / `↓` | Select an entry |
| `Enter` / `→` | Enter a directory or open a file |
| `←` / `Backspace` | Go to the parent directory |
| `Enter` on `../` | Go to the parent directory |
| `r` | Reload the list |
| `e` | Open an editable selected file in `edit` |
| `q` | Quit |

For example, run `fd /proc`, choose an entry with the arrow keys, and press `Enter` to inspect it. Press `q` or `Esc` to return from the viewer. **`q` quits from the file list but returns to the list from the viewer.**

## View a file

Pressing `Enter` on a file under `/tmp`, `/etc`, or `/proc` opens a **read-only viewer**. It reloads the path roughly once per second, making changing `/proc` values useful to observe.

| Key | Viewer action |
|---|---|
| `↑` / `↓` | Scroll one line |
| `PgUp` / `PgDn` | Scroll one screen |
| `e` | Launch `edit` when the file is editable |
| `q` / `Esc` | Return to the file list |

The viewer is intended for text. Nonprintable bytes appear as `.`, and long lines are clipped at the terminal width. **It never modifies a file.** Use `hexdump` for binary inspection.

## Edit a file

Select a writable RAM file directly under `/tmp` or `/etc` and press `e`; you can also press `e` from the viewer. Leaving `edit` normally returns to `fd`. The browser temporarily disables nonblocking input before launching the editor and restores its settings afterward. Files under `/proc` are for observation, not editing. See the [edit guide](EDIT.md).

## Limits and implementation notes

- The list displays at most 48 entries (including `../` when present). Further entries are counted but not displayed. The maximum layout is 40 rows × 110 columns.
- Copy, delete, rename, and move operations are not implemented; use shell commands instead.
- `/dev` nodes are not previewed because reading devices may block or have side effects.
- `../` comes from VFS `getdents()` rather than being invented by the browser. `.` is hidden, and `..` is omitted at the root.
- The viewer holds only the visible window in a 2-KiB buffer, allowing large files to be read sequentially.
- For `/proc/sym/kernel`, when `lseek()` is available, offsets are indexed every 16 lines (up to 128 entries). Opening another file or pressing `r` clears the index. Lines beyond the indexed range remain readable by scanning. Other `/proc` files and kernels without seek support use sequential reading.

**Troubleshooting:** If cursor keys do not work, verify terminal ANSI support. If an expected item is missing, check the current directory and the 48-entry limit; use `ls` as an alternative.

---

**Related links:** [Setup](../setup/SETUP.md) | [Guide index](README.md) | [edit](EDIT.md) | [日本語](../../ja/guides/FD.md) | [Documentation](../../DOCS.md)
