# edit — text editor

`edit` edits RAM-backed text files. It does not modify arbitrary memory addresses.

Run `edit /tmp/test.s` to edit or create a file. Use arrow keys to move, Enter for a newline, and TAB for 4-column tab stops (the TAB byte is preserved). Ctrl+S saves and Ctrl+Q exits; unsaved changes prevent quitting. Files are limited to 1,024 bytes; 20 lines are shown. Horizontal scrolling is not implemented.
