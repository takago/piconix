# vi — minimal modal editor

[日本語](../ja/VI.md) | [Documentation](DOCS.md) | [edit](EDIT.md)

`vi` is a small modal editor separate from `edit`. Use `vi /tmp/hello.txt` to edit a RAM file. RAM files hold up to 2048 bytes, but the current `vi` editor buffer is limited to 1024 bytes. It is not a full vi implementation.

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
