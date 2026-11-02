# vi — minimal modal editor

[日本語](../ja/VI.md) | [Documentation](DOCS.md) | [edit](EDIT.md)

`vi` is a small modal editor separate from `edit`. Run `vi /tmp/hello.txt` to edit a RAM file directly under `/tmp`. Its buffer holds up to 2,048 bytes, matching the RAM-file limit. It is not a full vi implementation. If a file exceeds 2,048 bytes, the editor rejects it instead of silently truncating its contents.

| Key | Action |
|---|---|
| `h`, `j`, `k`, `l` | Move cursor in normal mode |
| Arrow keys (up/down/left/right) | Move cursor in both normal and insert modes |
| `i`, `a` | Enter insert mode |
| `Esc` | Return to normal mode |
| `x` | Delete character |
| `dd` | Delete line |
| `o` | Open next line |
| `:w`, `:q`, `:q!`, `:wq` | Write, quit, discard and quit, write and quit |

The normal-mode `dd` command for deleting a line is distinct from the separate [binary-copy `dd` command](DD.md).

The editing area starts at the left edge with no line-number gutter. Arrow keys require a terminal that sends ANSI escape sequences.

## First editing exercise

1. Start `vi /tmp/hello.txt`.
2. Press `i` and type some text.
3. Press `Esc` to return to normal mode.
4. Type `:wq` and press Enter to save and quit.
5. Check the file with `cat /tmp/hello.txt`.

If there are unsaved changes, `:q` displays a warning instead of quitting. Use `:q!` only when you want to discard edits. Exiting with `Ctrl+C` does not save.
