# Minimal VFS directory enumeration

## ABI
- `getdents(int fd, piconix_dirent_t *entry, size_t size)` returns one directory entry (`1`), end of directory (`0`), or a negative OS error.
- `piconix_dirent_t` has a 32-byte NUL-terminated `name` and a `uint8_t is_dir`.
- New syscall 26, appended to the existing syscall enum. Existing syscall numbers unchanged.
- No dynamic allocations, inode layer, or mount tables.

## Behavior
- `/bin` and `/tmp` are directory-only FDs. Their old newline-delimited `read()` behavior has been removed; enumerate entries with `getdents()`.
- Other directory paths can now be opened read-only and enumerated.
- `ls`, `tree`, and `find` use `getdents()`. `tree`/`find` retain their traversal-depth restrictions.
- `/proc/task` entries are reported as files, as in the prior `walk.h` implementation.
- The `/bin` shell built-in command footer in `ls` is retained.
- Physical directory entries are produced from live kernel state (`tmp_files`, registered programs, tasks, GPIO/PWM/ADC pin config).
- Error and buffer contracts: one fixed-size entry per call, caller capacity >= sizeof(entry). Non-directory FD returns `OS_EBADF`.

## Validation
- All host-side static tests pass except `test_ram_pipeline.py`, blocked by missing ARM GCC/libgcc in this environment.
- ARM-targeted object compilation passes for `ls.c`, `find.c`, `tree.c`, `syscalls.c` with Clang.
- SDK kernel build, PIC link checks, and physical-device tests must be performed on the user's machine.

## Suggested device checks
```
ls /
ls /bin
ls /tmp
ls /dev
ls /dev/gpio
ls /proc
ls /proc/task
ls /proc/sym/userland
tree -L 3 /proc
find /proc -name sh
find /tmp
```
`cat /bin` and `cat /tmp` are no longer supported; use `ls /bin` and `ls /tmp`.
