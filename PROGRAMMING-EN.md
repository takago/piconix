# Writing a PicoNIX Application

[日本語](PROGRAMMING-JP.md) | [English](PROGRAMMING-EN.md)

PicoNIX applications are freestanding binaries linked to a fixed RAM address, not ordinary desktop executables. Start by modifying Blink before adding a new command.

## 1. The fixed RAM slots

| Slot | Start address | Application |
|---|---|---|
| 1 | `0x20010000` | Shell |
| 2 | `0x20020000` | Blink |
| 3 | `0x20030000` | Omikuji or Python |

Each slot spans 64 KiB, including a 4 KiB reserved stack area. All application images are embedded in flash. PicoNIX has no dynamic linker, relocation support, or file-to-executable loader.

## 2. First experiment: change Blink

Edit `ram_apps/blink.c`, for example by changing the duration passed to `SYS_SLEEP` to `250`. Rebuild with the command in [Setup](SETUP-EN.md), flash the UF2, and enter `blink` in the Shell.

## 3. System calls

RAM applications do not call kernel C functions directly. They execute SVC with the syscall number and up to three arguments in registers `r0` through `r3`; the result is returned in `r0`.

Typical calls include `SYS_EXIT`, `SYS_SLEEP`, `SYS_OPEN`, `SYS_CLOSE`, `SYS_READ`, and `SYS_WRITE`. Use the enumerations and register conventions in existing `ram_apps/*.c` as your starting point. Kernel-visible syscall numbers must remain consistent with `src/kernel.c` and `include/piconix.h`.

Do not assume a C runtime, standard library, or libc-compatible I/O. The RAM-image checker rejects unresolved symbols and relocation dependencies. Some compiler optimizations introduce helpers such as `__aeabi_*`, which a freestanding image cannot use unless explicitly provided.

## 4. Adding a new application

Create a freestanding `ram_apps/<name>.c` using an `app_entry` entry point and SVC calls. Allocate an available fixed task slot, add its image to the CMake application list, register it in kernel startup, and add command dispatch in `ram_apps/shell.c`. Respect exclusive ownership of shared physical slots. Keep additions small and avoid altering the syscall ABI unnecessarily.

## 5. Building and validating

```sh
python3 tools/test_audit.py
python3 tools/test_ram_pipeline.py
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
LC_ALL=C LANG=C PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DCMAKE_BUILD_TYPE=MinSizeRel
```

The host RAM pipeline currently tests Shell and Blink with LLVM. The SDK build with ARM GCC is still required to validate all four application images.

## 6. Files and standard I/O

Applications can use `/dev` and `/sys/proc` pseudo-files, and limited RAM-backed files in `/tmp`. File-descriptor inheritance and `dup2` are simplified: duplicated descriptors do not share file offsets like POSIX descriptors. Shell redirection supports `<`, `>`, `>>` with unquoted single-word paths. There are no pipes or generic file-based program launches.

Shell resolves relative file names using its current directory; applications should pass valid paths to the kernel. Opening `/dev/uart1` for output uses write-only mode, not regular-file `O_CREAT` or `O_TRUNC` flags.

## 7. Common pitfalls

Watch for RAM-slot overflows, unresolved ARM helper functions, incorrect syscall numbers, conflicting applications in slot 3, and background processes attempting console input. Consult [Internal Architecture](INTERNAL-EN.md) for task layout and file-descriptor details.
