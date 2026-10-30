# PicoNIX Terminal Handling: Implementation and Debugging Notes

This note captures practical lessons from developing the PicoNIX shell and full-screen applications (`mb`, `gpio`, `pong`, `fbed`, `edit`, `mem`, etc.). It is based on terminal issues observed with gtkterm and the VS Code terminal in October 2026.

## 1. Two independent layers

- **PicoNIX TTY settings:** canonical/noncanonical input, echo, ONLCR, blocking mode, controlled through `ioctl()` and `fcntl()`.
- **Terminal emulator display state:** cursor visibility, text attributes, alternate screen, scroll region and scrollback, typically controlled by ANSI/DEC escape sequences.

Restoring the TTY settings does **not** necessarily restore the emulator display state.

## 2. Line endings and cursor position

| Output | Typical effect |
| --- | --- |
| `\r` (CR) | Return to the beginning of the current line |
| `\n` (LF) | Move down one line |
| `\r\n` | Move to the beginning of the next line |

With ONLCR enabled, a TTY normally converts `\n` to `\r\n`. Do not confuse an ONLCR issue with the cursor-position restoration performed by alternate-screen sequences.

**Observed issue:** The shell initially emitted `ESC[?1049l` every time it printed a prompt. The prompt appeared to return to the same line after Enter. Removing this unconditional sequence fixed the behavior.

## 3. Useful escape sequences

`ESC` is written as `\033` in C string literals.

| Sequence | Meaning |
| --- | --- |
| `ESC[?1049h` | Enter alternate screen (often saving cursor position) |
| `ESC[?1049l` | Leave alternate screen (may restore saved cursor position) |
| `ESC[?25l` / `ESC[?25h` | Hide / show cursor |
| `ESC[0m` | Reset text attributes |
| `ESC[2J` | Clear the visible display |
| `ESC[H` | Move cursor home |
| `ESC[r` | Reset the scrolling region |
| `ESC[?1007l` | Disable Alternate Scroll, where supported |

Many terminal emulators do not provide ordinary scrollback for the alternate screen. Some translate wheel events into Up/Down key inputs while the alternate screen or Alternate Scroll behavior is active.

**Do not emit `ESC[?1049l` at every normal shell prompt.** Leaving the alternate screen can restore a saved cursor position and disrupt normal output even if an app has not switched screens.

## 4. Entering and leaving a full-screen application

A common pattern is to enter the alternate screen and hide the cursor on startup, then reverse both operations on normal exit:

```c
/* Enter full-screen mode. */
printf("\033[?1049h\033[?25l");

/* ... interactive UI ... */

/* Restore the normal display. */
printf("\033[?25h\033[?1049l");
```

An application may also change canonical/echo settings. Do not rely solely on application cleanup: forced termination via Ctrl+C can bypass its normal exit path.

### Current PicoNIX approach (October 30, 2026)

- The normal shell prompt resets attributes and shows the cursor (`ESC[0m`, `ESC[?25h`), but **does not** send `ESC[?1049l`.
- After foreground execution of `mb`, `gpio`, `pong`, `fbed`, `edit`, or `mem`, `shell.c` sends `ESC[?1049l`, including after interruption.
- TTY configuration restoration and emulator display restoration remain separate concerns.
- `reset` restores terminal input settings and also leaves alternate screens, disables Alternate Scroll, resets the scrolling region and text attributes, shows the cursor, and clears the display.

Listing full-screen command names in the shell is simple but must be maintained: add newly introduced full-screen programs to the relevant restoration logic. Explicit screen-mode tracking could be considered later if needed.

## 5. Observed failures and fixes

| Symptom | Finding / action |
| --- | --- |
| New commands repeatedly appeared on the same line | Removed unconditional `ESC[?1049l` from ordinary prompt rendering |
| Cursor remained hidden after leaving a game | Restore visibility with `ESC[?25h` |
| Mouse wheel acted as Up/Down keys | Investigate alternate-screen / Alternate Scroll behavior; screen restoration helped |
| Wheel no longer sent keys, but scrollback was unavailable | Sending `echo -ne '\033[?1049l'` restored the normal screen and scrollback |
| Changed sources did not affect device behavior | Check ZIP timestamps and incremental build dependencies |

The final behavior was verified with both gtkterm and the VS Code terminal. Whether `?1007l` alone was necessary was **not** separately established.

## 6. Diagnosing from PicoNIX

`echo -e` interprets escape sequences, and `-n` suppresses the trailing newline:

```sh
# Leave alternate screen
 echo -ne '\033[?1049l'

# Show cursor
 echo -ne '\033[?25h'

# Disable Alternate Scroll (where supported)
 echo -ne '\033[?1007l'

# Test line breaks
 echo -e 'ABC\nDEF'

# Restore the display and TTY input settings
 reset
```

Leading spaces in the example are only for formatting. Prefer one sequence at a time during diagnosis: clearing the display too early can erase useful evidence.

### Verification checklist

1. Immediately after boot, successive shell commands advance to new lines.
2. The cursor is visible after normal exits from `mb`, `gpio`, etc.
3. The shell remains usable after interrupting a full-screen app with Ctrl+C.
4. Wheel events are not translated into Up/Down key inputs.
5. Scrollback works on **new output in the normal screen** (alternate-screen content may not be retained).
6. Verify with multiple terminal emulators, e.g. gtkterm and VS Code.

## 7. When source changes do not appear in the firmware

Incremental builds may rely on modification timestamps. Extracting a ZIP can restore an older timestamp, causing a changed source to appear older than existing build outputs.

```sh
unzip -o PicoNIX-patch.zip
touch userland/shell.c userland/reset.c
# Run the usual build command.
```

Touch the actual modified files. Remove specific build artifacts or clean `build` only when needed. Most importantly, verify that the flashed UF2 contains the intended changes.

## 8. Rules for new full-screen applications

- Pair alternate-screen entry and exit.
- Restore cursor visibility if you hide it.
- Treat TTY input state and emulator display state separately.
- Handle both normal and forced termination.
- Never restore the alternate screen unconditionally at every normal prompt.
- Test cursor visibility, wheel behavior and scrollback **independently** after exit.
