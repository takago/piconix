# fd — file browser

> **Document type: USAGE GUIDE (not a textbook chapter).** [Related textbook chapter](../learn/A/A1.md) | [Guide index](README.md).

[日本語](../../ja/guides/FD.md) | [Documentation](../../DOCS.md)

`fd [directory]` opens a small, FDclone-inspired full-screen browser. The default directory is `/tmp`. Up/Down selects an entry, Enter/Right opens a directory or file, and Enter on the `../` entry, or Backspace/Left, goes to the parent directory. Press `r` to refresh and `q` to exit.

For `/tmp`, `/etc`, and `/proc` files, Enter opens a **read-only viewer that reloads the path roughly once per second**. This is useful for changing `/proc` status values. The viewer displays printable ASCII and replaces other bytes with dots; it is not a binary viewer. Up/Down scrolls one line and PgUp/PgDn scrolls one screen. The viewer can reach the end of large files such as `/proc/sym/kernel` while keeping only the current display window in a 2 KiB buffer. Long lines are clipped to the terminal width. File changes are not saved by the viewer. `q` or Esc returns to the list.

Press `e` on a selected file, or while viewing, to launch `edit`. Editing is supported for files directly in `/tmp` or `/etc`, consistent with the editor. Leave `edit` normally to return to `fd`. The browser clears nonblocking stdin before launching the editor and restores its input settings afterwards.

The list shows up to 48 entries (including `../` when applicable) and adapts to screen dimensions (capped at 40 rows × 110 columns). Other entries are counted but not shown. No file operations such as copying, deleting, renaming, or moving are implemented. `/dev` nodes are deliberately not previewed, since reading hardware devices may block or have side effects. A terminal supporting ANSI cursor keys is required.

The `../` entry now comes from the VFS `getdents()` interface rather than being synthesized by `fd`. The browser hides the `.` entry and avoids showing `..` at the root.

For `/proc/sym/kernel`, if the kernel supports `lseek()`, the viewer keeps a bounded index of file offsets every 16 lines and seeks to the nearest known position before reading. The index is reset when opening a file or pressing `r`. Other `/proc` files may change at any time, so they continue to use sequential scanning to avoid stale offsets. Older kernels without symbol-file seek support also fall back to sequential scanning. The index holds at most 128 offsets; distant lines past its coverage can still be read, but may require scanning.

