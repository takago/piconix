# vi — minimal modal editor

*Usage guide*

**Before you start:** Understand normal mode versus insert mode. If unsure which mode you are in, press `Esc` to return to normal mode.


`vi` is a small modal editor separate from `edit`. Run `vi /tmp/hello.txt` or `vi /etc/rc` to edit a RAM file directly under `/tmp` or `/etc`. Its buffer holds up to 2,048 bytes, matching the RAM-file limit. It is not a full vi implementation. If a file exceeds 2,048 bytes, the editor rejects it instead of silently truncating its contents.

| Key | Action |
|---|---|
| `h`, `j`, `k`, `l` | Move cursor in normal mode |
| Arrow keys (up/down/left/right) | Move cursor in both normal and insert modes |
| `i`, `a` | Enter insert mode |
| `Esc` | Return to normal mode |
| `x` | Delete character |
| `dd` | Delete line |
| `o` | Open next line |
| `:w`, `:q`, `:q!`, `:wq`, `:x` | Write, quit, discard and quit, write and quit |
| `:%s/OLD/NEW/g` | Replace all literal occurrences throughout the file (no regex) |

The normal-mode `dd` command for deleting a line is distinct from the separate [binary-copy `dd` command](DD.md).

The editing area starts at the left edge with no line-number gutter. Arrow keys require a terminal that sends ANSI escape sequences.

## First editing exercise

1. Start `vi /tmp/hello.txt`.
2. Press `i` and type some text.
3. Press `Esc` to return to normal mode.
4. Type `:wq` and press Enter to save and quit.
5. Check the file with `cat /tmp/hello.txt`.

If there are unsaved changes, `:q` displays a warning instead of quitting. Use `:q!` only when you want to discard edits. Exiting with `Ctrl+C` does not save.

## MCP terminal operation and substitution

Read `get_manual({"topic":"vi"})` first. Start with `terminal_open({"command":"vi /tmp/test.txt"})`, then inspect `terminal_read({})`. Do not launch an interactive editor through `execute_command`.

To replace every literal `1000` with `100`, send:

```text
terminal_send({"key":"ESC"})
terminal_send({"text":":%s/1000/100/g"})
terminal_send({"key":"ENTER"})
terminal_read({})
terminal_send({"key":"ESC"})
terminal_send({"text":":wq"})
terminal_send({"key":"ENTER"})
```

This is **literal whole-file substitution**, not Vim regex; `/` delimiters cannot be escaped. Replacements that would exceed 2,048 bytes are rejected without changing the text. `:help` shows a short command hint. `:set`, undo and `/pattern` search are unsupported. Verify the saved file independently with `cat /tmp/test.txt`. A successful `terminal_send` does not prove that the edit was saved, and `terminal_read` always returns the current screen; it does not confirm that the file was saved.

---

**Related links:** [Pipe guide](PIPE.md) | [Guide index](README.md) | [日本語](../../../ja/reference/commands/VI.md) | [Documentation](../../README.md) | [edit](EDIT.md)
