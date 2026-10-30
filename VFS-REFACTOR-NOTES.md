# Minimal VFS refactoring notes

This pass deliberately keeps the existing FD representation, ID allocation,
RAM file store, dynamic `/proc/task/<pid>` lookup, GPIO/ADC/PWM paths and
read/write backends inside `kernel/kernel.c`.

Changes:

- Added a small fixed virtual path registry (`vfs_registry`) for `/bin`,
  `/tmp`, `/proc/*` and fixed PWM/ADC files.
- Both `vfs_open()` and `fd_path()` consult the same mappings, removing
  duplicate path-to-ID and ID-to-path lists.
- Retained the existing `dev_registry` independently because `/proc/devices`
  lists those physical devices only.
- Preserved PWM pin eligibility checks and the original `/tmp` open flags.
- Renamed entrypoint dispatch to `vfs_open()` / `vfs_rw()`; all backends,
  descriptor layouts and syscall numbers remain unchanged.
- Updated a source-inspection test for the renamed open dispatcher and added
  `tools/test_vfs.py` to check the shared registry.

## Verification in the provided execution environment

- Baseline: 14 PASS / 2 FAIL (16 test scripts).
- Refactored: 15 PASS / 2 FAIL (17 test scripts).
- `test_help.py`: preexisting assertion error for `edit`.
- `test_ram_pipeline.py`: needs `arm-none-eabi-gcc` and `libgcc`; unavailable.
- Pico SDK build and ELF/UF2 size checks **not run** (cross compiler/SDK absent).
- Actual RP2040 device execution **not tested**.

With the supplied toolchain and SDK, run `./tools/build_sdk.sh --no-flash`
and then verify `/bin`, `/dev`, `/proc`, `/tmp`, `lsof`, `ls`, `cat`, `rm`,
`fcntl`, `ioctl`, `lseek` and `dup2` on device.

Scope explicitly excludes `/proc/sym/*` and RAM_FILE display changes.
