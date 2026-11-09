# fd — file browser

*Usage guide*

`fd` is a small, FDclone-inspired full-screen browser for navigating directories and viewing files from a terminal. It is not a full replacement for a Linux file manager.

## Start the browser

```sh
fd
fd /etc
fd /proc
fd /man
```

Without an argument, `fd` opens the current working directory. Supply a directory to start elsewhere. An **ANSI-capable terminal with cursor-key support** is required.

## Navigate the file list

| Key | Action |
|---|---|
| `↑` / `↓` | Select an entry |
| `Enter` / `→` | Enter a directory or open a file |
| `←` / `Backspace` | Go to the parent directory |
| `Enter` on `../` | Go to the parent directory |
| `/` | Filter the current directory by a partial filename (live) |
| `Enter` / `Esc` while searching | Apply / cancel the filter |
| `c` | Clear the filter |
| `r` | Reload the list |
| `e` | Open an editable selected file in `edit` |
| `q` | Quit |

Press `/` and enter a substring such as `trace` to filter filenames without case sensitivity. Use `↓` or `↑` to move between batches when more than 48 entries exist. This is a **filename search in the current directory**, not a search inside files.

For example, run `fd /proc`, choose an entry with the arrow keys, and press `Enter` to inspect it. Press `←`, `q`, or `Esc` to return from the viewer. **`q` quits from the file list but returns to the list from the viewer.**

## View a file

Pressing `Enter` on a file under `/tmp`, `/etc`, `/proc`, or `/man` opens a **read-only viewer**. For mutable files such as `/proc`, the viewer reloads roughly once per second. Static `/man` ROM pages are not refreshed.

| Key | Viewer action |
|---|---|
| `↑` / `↓` | Scroll one line |
| `PgUp` / `PgDn` | Scroll one screen |
| `/` | Search the displayed file's contents (case-insensitive) |
| `Enter` / `Esc` while entering text | Start / cancel a content search |
| `n` / `N` | Jump to the next / previous matching line (wrap around) |
| `e` | Launch `edit` when the file is editable |
| `←` / `q` / `Esc` / `Backspace` | Return to the file list |

In the file viewer, press `/`, type a term such as `socket`, and press Enter to jump to the first matching line. Matching text is highlighted. Use `n` for the next match and `N` for the previous match. This is separate from filename filtering on the directory screen. Search scans the file incrementally without retaining it in RAM.

The viewer is intended for text. Nonprintable bytes appear as `.`, and long lines are clipped at the terminal width. **It never modifies a file.** Use `hexdump` for binary inspection.

## Edit a file

Select a writable RAM file directly under `/tmp` or `/etc` and press `e`; you can also press `e` from the viewer. Leaving `edit` normally returns to `fd`. The browser temporarily disables nonblocking input before launching the editor and restores its settings afterward. Files under `/proc` and `/man` are read-only and cannot be edited. See the [edit guide](EDIT.md).

## MCP terminal operation

Read `get_manual({"topic":"fd"})`, then start `terminal_open({"command":"fd /tmp"})`. Read the screen using `terminal_read({})` before sending navigation keys such as `terminal_send({"key":"DOWN"})`. **`q` behaves differently in the file viewer and directory list**, so verify the current screen after each transition. Pressing `e` launches `edit`, not `vi`. To use `vi`, exit fd, then open a new session with `terminal_open({"command":"vi /tmp/NAME"})`.

## Limits and implementation notes

- Directory entries are loaded in 48-entry batches. Navigate between batches with arrow keys; filtering searches the whole current directory, not just the visible entries. The maximum layout is 40 rows × 110 columns.
- Copy, delete, rename, and move operations are not implemented; use shell commands instead.
- `/dev` nodes are not previewed because reading devices may block or have side effects.
- `../` comes from VFS `getdents()` rather than being invented by the browser. `.` is hidden, and `..` is omitted at the root.
- The viewer holds only the visible window in a 2-KiB buffer. Content search reads 128-byte blocks and supports terms up to 31 ASCII characters. Matches are line-local; a search cannot span a newline.
- For `/proc/sym/kernel` and `/man` files, when `lseek()` is available, offsets are indexed every 16 lines (up to 128 entries). Opening another file or pressing `r` clears the index. Lines beyond the indexed range remain readable by scanning. Other `/proc` files and kernels without seek support use sequential reading.

**Troubleshooting:** If cursor keys do not work, verify terminal ANSI support. If an item is missing, press `c` to clear any active filter and browse subsequent pages. For content search, press `/` in the viewer or run `grep PATTERN /man/FILE` from the shell.

---

**Related links:** [Setup](../../setup/SETUP.md) | [Guide index](README.md) | [edit](EDIT.md) | [日本語](../../../ja/reference/commands/FD.md) | [Documentation](../../README.md)
