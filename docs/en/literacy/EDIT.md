# edit — text editor

`edit` edits RAM-backed text files. It does not modify arbitrary memory addresses.

Run `edit /tmp/test.s` to edit or create a file. Use arrow keys to move, Enter for a newline, and TAB for 4-column tab stops (the TAB byte is preserved). Ctrl+S saves and Ctrl+Q exits; unsaved changes prevent quitting. The `edit` buffer holds up to 2,048 bytes, matching the RAM-file limit; the number of visible lines adapts to the terminal (up to 56 lines), and long lines scroll horizontally.


## Editing workflow

1. Run `edit /tmp/test.s` in the shell. This opens the existing file or creates it if it does not exist.
2. Enter text and move the cursor with the arrow keys. Backspace removes the preceding character; Delete removes the character at the cursor.
3. Press **Ctrl+S** to save. Confirm the `Saved` message at the bottom of the screen.
4. Press **Ctrl+Q** to exit. If unsaved changes remain, the editor asks you to save instead of quitting.

For a simple exercise, run `edit /tmp/hello.txt`, type and save a short line, then run `cat /tmp/hello.txt` in the shell to check what was stored.

## Limits and cautions

- Only RAM-backed files directly under `/tmp` can be edited. The editor cannot modify `/etc` files or arbitrary memory addresses.
- Ctrl+C **quits without saving**. To keep your changes, use Ctrl+S before Ctrl+Q.
- Files over 2,048 bytes are rejected rather than silently truncated.
- On a save failure, the status line shows `Save failed`. Check file size and available space.
- The editor temporarily changes terminal input and display settings; use an ANSI-compatible terminal.
