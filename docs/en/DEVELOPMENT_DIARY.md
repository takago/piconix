# PicoNIX Development Diary

### 2026-11-08: Align source-level teaching material with current ABI

Corrected stale syscall-range descriptions and the uptime lab collision with syscall 40 (`select()`). Removed an obsolete patch that did not apply to the current tree; the lab now uses manual changes on a dedicated branch. Separated MCP operation from source-reading exercises and added a source-first reading map. Historical diary entries remain unchanged. Full SDK build and hardware testing were not performed for this documentation revision.


This diary records past development states, not necessarily current interfaces. Retired `docs/history/` material remains accessible through Git history; consult the current guides for supported commands and APIs.

### 2026-11-08: Separate ROM manual from kernel Flash attribution

Updated `tools/size_ram.py` to report `ROM manual` as its own category. ELF `.manual` section ranges take precedence over link-map object paths and embedded app spans, preventing double counting. Total ELF Flash/SRAM usage is unchanged; only the attribution is corrected. Added a synthetic-map regression test. Full SDK build and device validation remain pending.

### 2026-11-08: Documentation consistency and current workflows

Synchronized VFS/device inventories following the `/dev/fb` migration, documented `/dev/fbchar`, and replaced obsolete MCP-tool instructions with current `get_manual(topic)` usage. Reorganized Japanese/English indexes while retaining earlier records as history.

### 2026-11-08: Move the 8×8 LED framebuffer to `/dev/fb`

Moved the raw eight-byte LED framebuffer device from `/dev/gpio/fb` to `/dev/fb` to separate the display interface from individual GPIO controls. The `/dev/fbchar` text/icon interface is unchanged. Updated the VFS registry, `fbctl`, `fbed`, `scroll`, `kitesys`, `httpd`, Python samples, UDP remote device service, host clients, ROM manual, Japanese/English documentation and regression tests. The old path has no compatibility alias; historical development diary entries retain their original references.


### 2026-11-08: Batch `echo` writes for `/dev/fbchar`

Investigated `echo heart > /dev/fbchar` showing only `t`: `echo` previously issued one-byte writes, causing each byte to be interpreted as a separate glyph. `echo` now buffers short output (128 bytes) into a single write, while preserving partial-write retries and `-n`/`-e` semantics. GPIO direction and pull configuration writes now accept a trailing newline. Wi-Fi restart retains support for split writes; tests were adjusted. ARM syntax and host output tests passed. Full Pico SDK builds and hardware behavior remain unverified.

### 2026-11-08: Clarify GP18 buzzer and GP19 servo connections in ROM manual

Updated the ROM manual to state that GP18 drives a buzzer and GP19 a servo motor. Both pins share a PWM slice frequency, so buzzer tone settings can also affect servo timing.

### 2026-11-08: Readable `/dev/fbchar` pattern catalog

Added a finite, read-only listing via `cat /dev/fbchar` with printable ASCII range, supported icon names and usage examples. The icon names are enumerated from the same table used for drawing. Each descriptor maintains its own read offset and returns EOF at the end; reads never modify the LED display. Existing write behavior is preserved. Hardware validation is pending.

### 2026-11-08: Add `/dev/fbchar` fixed character and icon display

Added a `/dev/fbchar` device for persistent 8x8 LED display of single ASCII glyphs or named icons (heart/star/smile/sad/arrows/check/cross/clear). Moved the scroll 5x7 font to a shared header consumed by both user-space scroll and the kernel renderer. Preserved the existing eight-byte `/dev/gpio/fb` framebuffer protocol; the new text interface requires no additional syscall and works from shell and Python. Hardware validation remains pending.


### 2026-11-08: LLM field-test improvements to C, Pico W GPIO protection, and ROM manual

- Added prefix and postfix ++/-- for integer scalar locals to the small C compiler; pointers, arrays, and char lvalues remain unsupported. Integrated the earlier 2048-byte assembly output-limit diagnostics and partial-file prevention.
- Reserved Pico W radio-owned GPIO 23/24/25 (and 29) from generic GPIO access, skipped Pico-only GP25 LED initialization on Pico W, and marked pins as reserved in status output.
- Expanded the ROM manual with shell operator differences, process slot vs RAM-file resource constraints, allocated vs actual kernel SRAM, interpreter-specific incompatibilities, and board-specific GPIO safety.
- Based the changes on LLM agent experiments in C, tracing, memory, GPIO, Python temperature reading, and shell behavior. No task-specific successful programs were added to the reference. Complete Pico SDK firmware builds and on-device validation remain outstanding.

### 2026-11-08: Enabled ROM manual browsing and filename search in `fd`

- Allowed read-only previews of `/man` pages in the full-screen `fd` browser. Static ROM manuals are not periodically refreshed.
- Replaced the previous first-48-entry limitation with on-demand 48-entry batches, allowing navigation through all 64 manual pages.
- Added case-insensitive substring filename filtering via `/`; press Enter to accept, Esc to cancel, or `c` to clear. This filters the current directory only; use `grep` for file content.
- Checked ARMv6-M compilation, a two-address PIC comparison, and slot capacity using link-time arithmetic stubs. Full Pico SDK build and hardware operation remain untested.


### 2026-11-08: Moved ROM manuals into a dedicated XIP `.manual` section

- Replaced the large generated C array in `kernel/man_rom.h` with a single packed `manual.bin` holding the 64-page index and texts. The build generates this binary and embeds it with `.incbin` in an XIP-only `.manual` section.
- Updated both legacy and modular Pico SDK linker integration. The `/man` VFS reads the packed index and page bytes by offset; MCP `get_manual()` behavior is unchanged.
- The 90,570 bytes of manual source text plus the index produce a 93,232-byte ROM image. Verified deterministic generation, byte-for-byte page/index correspondence, and section placement in a minimal ARM link. Full Pico SDK firmware builds and hardware execution remain unverified.
- The old generated `kernel/man_rom.h` should be removed.


### 2026-11-08: Added detailed tracing, memory and device investigation references

- Added 13 `/man` pages covering actual trace columns/events/return values, select/socket analysis, symbol addressing, symptom-driven memory inspection, and discovery/control/verification/recovery for GPIO, PWM, I2C, ADC and the LED matrix.
- Cross-checked `trace.c`, `hexdump.c`, `mem.c`, `pins.c`, `pwmctl.c`, `i2cctl.c` and `fbctl.c`. Noted that I2C scanning performs real one-byte reads, `trace show` stops recording, and MCP has time/output limits.
- Marked illustrative traces as examples and stressed authorization before hardware changes. ROM generation and static checks completed; end-to-end hardware execution remains unverified.

### 2026-11-08: Added runnable-oriented language labs to ROM manual

- Added four C, three Thumb-1 assembly, five Python-like interpreter labs, and an MCP agent verification workflow. Each gives source, command sequence, expectations, limits, and troubleshooting.
- Cross-checked the examples with the current language implementations and existing assembly examples; updated the ROM index.
- ROM generation and static checks passed. Full firmware build and hardware execution of all examples remain unverified.


### 2026-11-08: Expanded agent-first ROM manual workflows

- Added 21 task-oriented `/man` pages for MCP programming, compilation, memory inspection, tracing, GPIO/PWM/I2C and networking.
- Documented noninteractive execution, the MCP three-second / 4 KiB limits, cautious recovery after transport failures, and verification before hardware changes.
- Added troubleshooting guidance for the limited on-device C compiler, Thumb-1 assembler and Python interpreter.
- ROM generation and static consistency are tested; the new examples have not all been exercised on hardware.


### 2026-11-08 — Expand ROM manuals for hands-on LLM agent work (hardware unverified)

Expanded `/man` into a task-driven reference for agents developing programs, inspecting memory, tracing system activity and controlling devices on PicoNIX. Checked the actual parsers, buffers and supported APIs of `cc`, `as` and the Python-like interpreter; documented language subsets, unsupported features, short examples, build/run/verify workflows and error recovery. Added `/man/workflows`, `/man/limits`, language-specific examples, `/man/syscalls` and `/man/troubleshooting`; revised the original pages. The generated ROM manual now has 17 pages (~29 KiB of text). Deterministic generation and static consistency checks passed. Full Pico SDK linking and execution of all examples on physical hardware remain unverified.


### 2026-11-08 — Expose agent-focused ROM manuals under `/man` (hardware unverified)

Added task-oriented manuals for LLM-driven programming, memory inspection, tracing and device control. At build time, the manual text is converted into a ROM index and exposed as read-only `/man` and `/man/languages` files. `get_manual()` returns the index; an optional `topic` reads one page over HTTP chunked transfer. `help` points to `/man`. Optional `mcpd --shell` remains unchanged. Full target build and hardware networking are not yet verified.

## 2026-11-08 — Improved intermittent MCP HTTP response failures (no recurrence reported on hardware)

The host MCP client intermittently aborted with `incomplete chunked read` while `mcpd --shell` ran commands such as `trace run -a pwmctl freq 523` and `hexdump /dev/shm 0 512`. Updated `mcpd` to wait approximately 3 seconds rather than 0.6 seconds on TCP send `EAGAIN`, read command output in 256-byte rather than 64-byte units, and expand the JSON-escaping send buffer from 256 to 768 bytes. Preserved the 4 KiB command-output cap, MCP tool behavior, and 24 KiB user-slot limit. Provisional ARM linking and two-address PIC comparison passed. The developer reported that the previously intermittent error stopped occurring on the device after applying the patch. Because several parameters changed together, the individual root cause and reliability under all conditions remain unproven.

## 2026-11-08 — Streaming MCP/LLM client with default low thinking effort

Changed `host/mcp_llm_client.py` to always request `stream=True` and to specify `reasoning_effort="low"` by default when calling Ollama's OpenAI-compatible endpoint. Available `reasoning_content` or `reasoning` is printed incrementally as gray `[THINK]` text, separate from final answers, tool calls, and results. Fragmented tool names, arguments, and IDs are reassembled from streaming deltas before MCP calls. Python syntax and simulated stream-handling tests passed. Actual thinking text depends on the Ollama model/API exposing it. Updated the Japanese and English usage guides.

## 2026-11-08 — Synchronized host MCP clients with current tool APIs

Updated `host/mcp_client.py` to invoke argument-free `get_manual()` rather than supplying the removed `topic` parameter. Updated `host/mcp_llm_client.py` guidance to fetch the complete manual, consult `help`, and prefer non-interactive shell commands. Tool discovery remains dynamic. Preserved explicit `mcpd --shell` opt-in: default startup exposes only `get_temperature`, while `--shell` additionally exposes `get_manual`, `execute_command`, and `write_file`. The developer verified tool publication with `--shell` on hardware. Always enabling the shell was considered but rejected in favor of progressive MCP instruction and deliberate enabling of command execution.

## 2026-11-08 — Fixed PIC mismatch from MCP manual's pointer table

A string-pointer array introduced while enumerating seven `get_manual()` sections generated absolute ARM relocations (`R_ARM_ABS32`), causing `mcpd.bin` to differ when linked at two RAM addresses. Replaced that representation with a PIC-compatible alternative, and verified removal of the problematic relocations and successful two-address PIC comparison. These provisional ARM checks are not the same as a complete Pico SDK firmware build.

## 2026-11-08 — Clarify non-interactive MCP command restrictions

Updated the `execute_command` tool description and embedded `get_manual()` text to discourage interactive or full-screen commands (`mem`, `watch`, `vi`, `edit`, `fd`, `pong`, `gpio`, `fbed`, interactive `python`). Recommended terminating commands such as `ps`, `cat /proc/...`, `help`, and `trace show`. Updated the bilingual agent/MCP guides. This is guidance, not an enforced execution denylist. ARM compilation and provisional linking succeeded; hardware verification is pending.


## 2026-11-08 — Full MCP manual response and expanded help

Changed `get_manual(topic)` to argument-free `get_manual()` returning all seven sections via chunked JSON without a large response buffer. Expanded `help trace` with actual trace modes and usage; refreshed `cc`, `mcpd`, `scroll`, and `watch` help. Hardware testing pending.

## 2026-11-08 — Simplify MCP tools and expand embedded manual

Removed the dedicated `get_wifi_info` and `display_message` tools. Network inspection and LED output now use existing shell commands through `execute_command`. Added `network` and `display` topics to `get_manual`, with examples for `ip`, `/proc/net/wifi`, `scroll`, and `nc -u`, plus process-slot limits. Retained `get_temperature`, `write_file`, and `execute_command`. Host and hardware verification are reported separately.

## Purpose and scope

The design principles and specifications of PicoNIX are determined by its creator based on their knowledge and experience. Source code and documentation are generated and refined using ChatGPT under these decisions. The creator reviews changes and performs build and hardware checks during development. AI generation is not equivalent to verified behavior.

> **How this diary was produced:** This development diary was **automatically generated by ChatGPT** from conversations between the PicoNIX developer and ChatGPT. It reconstructs technical challenges, design decisions, and experiments from those conversations. Some dates, sequences of changes, and implementation details may be inaccurate. Consult the Git commit history and source code for authoritative records of actual changes.

PicoNIX grew into a small UNIX-like operating system for Raspberry Pi Pico (RP2040), intended to make operating-system mechanisms observable and understandable. This diary records **the development process through conversations between the developer and ChatGPT**: design choices, experiments, failures, revisions, and lessons learned. It is not a substitute for the current technical documentation.

The primary period is **October 24 through November 6, 2026**. The sequence is reconstructed from conversation records, rather than exhaustively verified against every Git commit. Events with uncertain exact dates are grouped into approximate periods. Descriptions of earlier architectures are historical and should not be mistaken for the current implementation.

### 2026-11-08 — Flush small `watch` screen updates (hardware not yet verified)

When watching short outputs such as `watch ip`, ANSI cursor-positioning updates contain no newline, so line-buffered `stdout` could retain a short incremental redraw instead of displaying it immediately. Flush `stdout` after each redraw regardless of the number of changed rows. Existing watch tests and ARM syntax checks passed; hardware display remains unverified.

### 2026-11-07 — Fixed MCP shell `[execution error]` (hardware retest pending)

`execute_command("ps")` returned `[execution error]`: `mcpd` attempted `fcntl(F_SETFL, O_NONBLOCK)` on its output-capture pipe, but the kernel's `SYS_FCNTL` rejected pipe descriptors even though pipe reads and writes already supported nonblocking behavior. Allowed pipe descriptors for `F_SETFL`. Relevant source-level MCP and shell regression checks passed; Pico W hardware revalidation remains pending.

### 2026-11-07 — Added MCP `write_file` for agent coding experiments (hardware unverified)

Added opt-in `write_file(path, content)` when starting `mcpd --shell`. It writes up to 2,048 bytes of ASCII text to RAM files immediately under `/tmp`, decoding the JSON string incrementally using a small buffer. Expanded the HTTP JSON body limit to 3,200 bytes; escaped payloads may reduce the usable maximum. Normal startup does not expose file writes. The proposed workflow is source creation → `cc` → `as` → execution. ARM syntax and MCP tool metadata tests passed, but a complete firmware link and on-device execution remain unverified. The unauthenticated service must be restricted to an isolated network.

### 2026-11-07 — Stream MCP shell output up to 4 KiB (hardware unverified)

Raised `execute_command` output limit from 399 to 4,096 bytes. To conserve the 24 KiB task slot, the MCP server now reads 64-byte chunks from the pipe, JSON-escapes them incrementally, and sends a single JSON response using HTTP/1.1 chunked transfer without retaining the entire output. Excess bytes are drained and reported as truncated. ARM syntax and regression tests passed; complete firmware and hardware/Open WebUI interoperability remain unverified.

### 2026-11-07 — Experimental MCP shell tool (hardware not yet verified)

Added opt-in `execute_command` with `mcpd --shell` and a non-interactive `sh -c` mode. The server captures bounded stdout/stderr through a pipe and enforces a nominal timeout. Normal `mcpd` startup does not publish the shell tool. Added Japanese and English agent guides documenting limitations, Linux differences, Open WebUI setup, and security risks from unauthenticated command execution. ARM-target syntax checks passed; complete firmware and hardware execution remain unverified.

#### 2026-11-07 — Color-coded terminal output for the MCP/LLM client

Improved `host/mcp_llm_client.py` to distinguish user input (cyan), LLM answers (green), MCP tool invocations and arguments (yellow), tool results (blue), and errors (red). Colors are automatically disabled for non-TTY output and when `NO_COLOR` is set. The developer confirmed that the updated display behaved as expected in use.

### 2026-11-07 — Hardware validation of C function calls and pointers

The developer reported `PASS` on PicoNIX hardware for a program calling `add()` and `square()` via `calculate()`, and for an `int *`-based `swap()` program compiled using `cc`. These results validate argument passing, return values, local variables, and indirect reads/writes for the tested cases. They do not constitute complete hardware validation of all pointer operations or recursion.

## 2026-11-07 — LLM displays the Pico W IP address on the LED matrix (hardware verified)

The developer confirmed an end-to-end demonstration on Pico W hardware: a local LLM used the MCP tool `get_wifi_info` to read the device's IP address, then called `display_message` to scroll the address on the 8×8 LED matrix. The MCP client connects to `mcpd` on TCP port 8080; `mcpd` forwards plain text to UDP `127.0.0.1:2001`, where `scroll -u 2001 &` receives it.

This verifies one real-world instance of an LLM combining two MCP tools for device-state retrieval and physical output, along with inter-process UDP communication. The scrolling display uses the X11 Misc Fixed 5×7 bitmap font, trimming unused side columns and preserving one unlit column between adjacent letters. The developer reported that the complete workflow worked on the device. UDP acknowledgements have deliberately been removed, so a successful `display_message` send by itself does not prove completed display.

### 2026-11-07 — `cc` writes source-named assembly files

Changed the fixed `/tmp/cc.s` output to a `.s` file next to the input (`cc test.c` -> `test.s`, `cc /tmp/test.c` -> `/tmp/test.s`). Updated English and Japanese compiler exercises. Host path checks and compiler regressions run; device execution not yet verified.

### 2026-11-07 — Basic pointer support in `cc` (hardware unverified)

Implemented `int *` and `char *` locals and parameters, address-of, dereference, pointer arithmetic and indexing, and pointer buffers for read/write. Six new host-side assembly tests, including `swap()`, and all existing `cc` regression suites pass. Pico hardware execution remains unverified.

### 2026-11-07 — Extended `cc` with local address-of and integer constants

Added `&c` for local scalar variables in `read`/`write`, distinct treatment of `char buf[1]` arrays, and decimal integer constants larger than 255. Existing Thumb-1 instructions construct wide constants. Host compile/assemble tests passed including a recursive numeric printing example; hardware behavior remains unverified.

### 2026-11-07 — Fixed `cc` argument and local-variable addressing (hardware unverified)

A program calling `add(a,b)` followed by `square(sum)` printed `FAIL`: stack-relative loads referred to the wrong variable after argument pushes or alignment changes. `cc` now uses callee-saved `r4` as a stable frame pointer and adjusts SP only after evaluating odd-count argument lists. Host-side compile/assemble regression suites passed, but the `PASS` result has not yet been validated on hardware.

### 2026-11-07: scroll and MCP LED integration (implementation; hardware unverified)

Added the `scroll` user app using select, a 1 KiB ring buffer, UDP 2001 acknowledgments, and an experimental MCP `display_message` tool (since removed). Updated README and bilingual teaching guides. Hardware verification is pending.

### 2026-11-07 — Proportional LED scrolling (hardware validation pending)

`scroll` now trims blank columns from both sides of each 5×7 glyph before rendering, with a one-column inter-character gap. A space contributes one additional blank column. Input, UDP, MCP, and ring-buffer handling are unchanged.

### 2026-11-07 — Indented assembly generated by `cc` (hardware unverified)

`cc` now prints two leading spaces before instructions, while keeping labels and assembler directives flush left. The formatter handles fragmented `.ascii` output. All seven existing host-side compiler regression suites passed, including assembly by `as`. Hardware execution remains unverified.

### 2026-11-07 — Simplified scroll UDP text input (hardware unverified)

Changed `scroll` to accept plain ASCII UDP datagrams without request IDs or OK/FULL/ERR replies. Excess characters are dropped when the ring is full. MCP `display_message` now sends raw UDP text without waiting for an ACK; a successful local send does not confirm display. Updated bilingual guides and client instructions.

### 2026-11-07 — Three independent UDP user sockets (hardware not yet validated)

- Investigated `nc: socket unavailable` while `scroll` held a UDP socket, tracing it to the single `udp_owner_pid` and shared UDP PCB/receive state.
- Added a per-FD slot ID for ownership, bind, send/receive, select, blocking wakeups, local-port reporting, and cleanup. Each UDP slot has its own 512-byte receive buffer.
- Raised the lwIP UDP PCB pool from four to eight, and updated regression checks and bilingual documentation. RP2040 hardware and final SRAM layout remain unverified.

### 2026-11-07 — Educational `cc` compiler and `as` extensions (hardware unverified)

> 2026-11-07: Switched `cc` comparisons to signed Thumb-1 branches; added `blt/ble/bgt/bge` to `as` and verified encoded condition fields (not CPU execution).
> 2026-11-07: Updated the `cc` stack test for the 256-byte frame and added 4 positive / 5 negative compiler regression cases. Verified Thumb encoding only; RP2040 execution remains untested.
> 2026-11-07: Extended educational `cc` to accept expression-valued file descriptors and byte counts for array-based `read`/`write`; added host compiler/assembler regression tests. Hardware validation pending.

### 2026-11-07 — C compiler char and array support

Added byte-sized char locals, fixed-size int/char arrays, indexing, sizeof, string initialization, and limited read/write buffer arguments. Extended the assembler with SP-relative address generation. Host compiler/assembler tests pass; hardware execution is pending.

- Extended the educational `cc` compiler with `else`, `>` and `<=` using existing Thumb-1 branches; unsigned-comparison limitations remain documented.

### 2026-11-07 — Stack locals in cc and SP-relative assembler instructions

### 2026-11-07 — Recursive-descent expressions in cc

Added precedence-aware integer arithmetic, parentheses, unary operations, and remainder. Extended `as` with register arithmetic/comparison instructions. Three host C-to-Thumb assembly tests pass; signed division and hardware execution remain unverified.


Moved integer locals from fixed r4–r7 allocation to a 64-byte stack frame (up to 16 locals). Added SP-relative `ldr/str` and stack-pointer adjustment to `as`. Verified C-to-assembly-to-Thumb encoding on the host; hardware execution remains untested.

<!-- 2026-11-07 cc subset extension -->
- Extended the educational `cc` with four integer locals, assignments, `if`, and `while`; verified assembly with the existing `as` on the host. Device execution remains untested.

- 2026-11-07: Added an educational C-subset compiler `cc` and a C→Thumb-1→`as` exercise (not yet hardware-tested).

- 2026-11-07: Added experimental user-defined functions, up to four integer parameters, per-call stack frames and recursive code generation to `cc`; assembler tests pass, hardware execution unverified.

## Purpose and scope

The design principles and specifications of PicoNIX are determined by its creator based on their knowledge and experience. Source code and documentation are generated and refined using ChatGPT under these decisions. The creator reviews changes and performs build and hardware checks during development. AI generation is not equivalent to verified behavior.

> **How this diary was produced:** This development diary was **automatically generated by ChatGPT** from conversations between the PicoNIX developer and ChatGPT. It reconstructs technical challenges, design decisions, and experiments from those conversations. Some dates, sequences of changes, and implementation details may be inaccurate. Consult the Git commit history and source code for authoritative records of actual changes.

PicoNIX grew into a small UNIX-like operating system for Raspberry Pi Pico (RP2040), intended to make operating-system mechanisms observable and understandable. This diary records **the development process through conversations between the developer and ChatGPT**: design choices, experiments, failures, revisions, and lessons learned. It is not a substitute for the current technical documentation.

The primary period is **October 24 through November 6, 2026**. The sequence is reconstructed from conversation records, rather than exhaustively verified against every Git commit. Events with uncertain exact dates are grouped into approximate periods. Descriptions of earlier architectures are historical and should not be mistaken for the current implementation.

### 2026-11-06 — Enable function instrumentation by default and add syscall-only mode

- Changed the build default of `PICONIX_KERNEL_FUNCTION_TRACE` to ON while keeping event capture OFF at boot; start capture using `trace on` or `trace run`.
- Added `trace on -s` and `trace run -s` to record only SYSCALL entry/exit; KERNEL, TIMER and SWITCH events are filtered before entering the ring. Existing standard and `-a` modes remain available.
- Kept the 32-byte ABI; extended mode selection and restoration after `trace run`. Updated bilingual guides, exercises, setup instructions and regression checks. Hardware build and load measurements remain pending.

### 2026-11-06 — Explicit function instrumentation exclusions across kernel sources

- Added `NO_TRACE` to every non-selected function definition in the eight C sources built with `-finstrument-functions`, keeping the 51 functions in `TRACE_MATCH` instrumented. Function bodies and the address/name matching table are unchanged.
- Extended the existing target-check test to catch missing exclusions and wrongly excluded targets without adding a generator or a build step.
- UART input and SYSCALL/TIMER/SWITCH events remain available. Hardware performance measurements are pending.

### 2026-11-06 — Suppress instrumentation of frequent polling probes

- Marked TCP/TTY readiness probes and scheduler wake-check routines `NO_TRACE` to avoid entry/exit hook overhead while leaving the polling logic unchanged.
- Removed three probes from `TRACE_MATCH` (54 to 51 entries); retained `tty_input_char()`, timer, switch and syscall events.
- Updated the target checks and bilingual trace guides. Hardware overhead measurements remain pending.

### 2026-11-06 — Separate changes in displayed PID with ASCII lines

- Replaced the ANSI underline on `SWITCH` rows with a hyphen-only separator before any displayed row whose PID differs from the preceding displayed row.
- `SWITCH` rows show the destination PID, while a completed blocked system call can name a task before it resumes. The separator therefore marks changes in displayed PID, not necessarily the exact context-switch instant. No kernel event or ABI change was made.

### 2026-11-06 — Revert SVC boundary events and fix syscall indentation

- Removed the experimental `SVC_LEAVE` assembly hook, kernel recorder, event definition, and display handler; `SVC_ENTER` will not be added. The SVC exception-return assembly returns to its prior form.
- Nest `SYSCALL` rows beneath an instrumented `os_svc_dispatch()` and indent internal kernel functions one level further. Preserve top-level syscall display when the dispatcher is not traced.
- Retain underline on `SWITCH` rows, the 54 selected functions, and the 32-byte event ABI.

### 2026-11-06 — Record the SVC return boundary and highlight context switches

- Added `SVC_LEAVE` in the assembly SVC handler after `os_svc_dispatch()` returns and before exception return. This does not guarantee resumption of the previous user program.
- Underlined `SWITCH` rows in the trace output with ANSI SGR, restoring terminal attributes at the end of each line. The 32-byte trace ABI and 512-record ring remain unchanged.
- Hardware validation is still needed, including the additional event volume and UART1 tracing overhead.

### 2026-11-06 — Experimentally trace TTY character input

- Added `tty_input_char()` to the selected function list, increasing the target count from 53 to 54 without changing the event ABI, trace processing, or TTY input behavior.
- Because UART1 calls it in the receive interrupt, continuous-reception tests on hardware must check for lost input and interrupt overhead. Use `trace on -a` to capture the non-SVC path; default function tracing remains SVC-only.
- Updated both trace guides and the trace-target consistency test.

### 2026-11-06 — Trace SVC dispatch and task wakeups

- Added `os_svc_dispatch()` and `task_unblock()` to the selected kernel function trace targets, bringing the count from 51 to 53 without changing the existing `SYSCALL` records or trace ABI.
- Left `tty_input_char()` unselected despite its shared USB CDC/Telnet/UART1 input role because it runs per character, including in the UART1 receive interrupt, and could increase instrumentation overhead and trace traffic.
- Updated the target-list regression test and both trace guides. Hardware validation remains necessary.

### 2026-11-06 — Clarified prior-knowledge assumptions and source-focused learning

- Stated that the documentation assumes prior UNIX/Linux, OS and C knowledge, and concentrates on RP2040-specific behavior, source reading, observation and modification.
- Expanded the English kernel-reading stages, removed duplicated sections in the Japanese guide, and added a concrete edit/test/build/hardware-validation workflow to the kernel development guide.
- No source code or command behavior changed.

### 2026-11-06 — Reorganized usage guides for beginners

- Improved the Japanese and English `FD`, `MEM`, `TRACE`, `TERMINAL`, and `DD` guides, separating introductions, first steps, output interpretation, limitations, and further reading. Added quick-start context to `EDIT`, `VI`, and `PIPE`.
- Moved the `MEM` observation questions to the bilingual exercise index, leaving operating instructions and cautions in the guide.
- No kernel or command behavior was changed; documentation links and bilingual structure were checked.

### 2026-11-06 — Separate guides and exercises

- Kept usage instructions, reproducible examples, and technical specifications in guides and references, while collecting investigation tasks in `docs/en/exercises/`.
- Reorganized ten tracing exercises, kernel reading questions, MCP/Linux comparisons, and network review questions, with matching Japanese material.
- Updated documentation indexes, roadmaps, task lookups, and cross-links. No kernel or user application sources were changed.

### 2026-11-06 — Show the requested path on open entry traces

- Record up to 16 validated bytes of the user-provided path on `open` ENTER, replacing the literal `path` placeholder when data is available.
- Preserve the existing `open/` EXIT behavior, 32-byte trace ABI, and FD-name cache. Failed opens also retain the requested name from ENTER.
- Check the pointer range, avoid additional path reads while tracing is disabled, and update the bilingual tracing guides and regression coverage.

### 2026-11-06 — Improve C readability and audit the documentation

- Reformatted simple single-line `if`, `for`, and `while` statements across C source and headers to make control flow easier to read. Preserved macros, C tokens, and existing behavior; compared the token streams before and after formatting.
- Added missing function summaries in the networking, MCP, tracing, and host utilities; regenerated `docs/ja/development/FUNCTIONS.md` from the current source.
- Checked the bilingual manual pairs, internal Markdown links, generated function index, and documented interface contracts. Kept obsolete commands in historical diary entries to preserve the chronology of development.
- Ran available host-side regression checks. ARM firmware, PIC, and SRAM layout validation remain unverified without the required cross-toolchain.

### 2026-11-06 — Clarify kernel-function tracing and maintain its target list

- Retained the `TRACE_MATCH` list in `kernel.c`, including functions from other kernel modules referenced through internal headers. Documented its distinction from the ELF-derived kernel symbol table used by `mem -s kernel`.
- Added a source-level test for duplicate names, function definitions and CMake instrumentation coverage. No trace hooks, event ABI, or target functions were changed.
- Replaced obsolete `trace kernel` and `trace run -d` examples in the README and bilingual setup guides with the current `trace run` command.

### 2026-11-06 — MCP server and local-LLM integration verified on Pico W

- The Pico W `mcpd` user application (TCP 8080, `POST /mcp`) successfully served MCP `2026-07-28` `tools/list` and `tools/call` to the official MCP Python SDK v2 on Linux. The initial `tools/list` response missed the required `resultType`; correcting the server JSON response resolved the validation failure.
- `get_temperature` retrieved the on-chip temperature from `/dev/adc/temp`, and `get_wifi_info` returned AP-mode network information from `/proc/net/wifi`. Recorded measurements included 33.2°C and, in a separate conversation, 31.8°C; the AP address was `192.168.4.1`.
- A local LLM using an OpenAI-compatible API independently selected MCP tools in response to natural-language questions about temperature, IP address, and PicoNIX status, then summarized the results. It answered a greeting without any tool call.
- The MCP Python SDK v2 field names `input_schema` and `is_error` were verified in practice. A compact non-streaming example is provided as `host/mcp_llm_client.py`, with Japanese and English guides and placeholder LLM server settings.
- This demonstrates the basic tool-discovery, tool-execution and LLM-assisted workflow; it does not establish full MCP conformance, secure deployment or long-term reliability. `mcpd` remains an independent user-space program with no MCP-specific kernel change.

### 2026-11-05 — Unified trace and Python I/O conventions

- Function-instrumented builds now include selected kernel function events in `trace on` and `trace run`; removed the dedicated `trace kernel` command. `-a` includes functions outside SVC. Interrupt/asynchronous function records have no PID.
- Preserved the 16 KiB ring and 32-byte event ABI; added selected ICMP device entry points from `net_capture.c`.
- Python file, socket and I2C operations use `-1` for operation failures; successful reads return byte arrays, EOF returns an empty array. `EAGAIN` is indistinguishable from other errors.


Extended `-finstrument-functions` to ADC, I2C, PWM, GPIO, framebuffer, and socket implementation files. Only selected public entry points running during synchronous SVC processing are recorded. The option remains disabled by default. Python read/recv/recvfrom now return `-1` on I/O failure, preserving byte arrays on success and `[]` for zero-length results. EAGAIN is also mapped to `-1`, which cannot distinguish retryable pending reads from other failures.

### 2026-11-05 — `select()`-based FD readiness wait (validation build)

- Added syscall 40, `select()`, using compact 16-FD masks for read/write readiness with immediate, finite, and infinite waits. Supports TCP/UDP, TTYs, and pipes.
- Added non-consuming TCP accept/receive and UDP receive readiness checks. Checking a pending TCP accept must not consume it.
- Replaced short-interval polling in `nc` and HTTP receive timeouts with `select()`. Existing blocking `accept()` and `recv()` remain available.
- Updated bilingual guides and contract tests. Final build, RAM layout, PIC, and hardware behavior are not yet validated.

### 2026-11-05 — Blocking TCP/UDP receives (validation build)

- `accept()`, TCP `read()`/`recv()`, and UDP `recvfrom()` now block by default; `O_NONBLOCK` retains immediate `OS_EAGAIN` returns.
- The scheduler checks socket readiness for `BLOCKED` tasks, avoiding lwIP poll reentrancy and using the existing saved-SVC-return mechanism.
- `nc`, `httpd` and timeout-based C examples explicitly request nonblocking sockets where needed. Hardware, final layout and PIC tests remain outstanding.

#### Unified error values for Python I/O (2026-11-05)

`read()`, `recv()`, and `recvfrom()` now return byte arrays on success or integer `-1` on failure. Socket `OS_EAGAIN` also returns `-1` instead of `[]`; the latter represents zero-byte results only. Negative results from `write()`, `send()`, and `sendto()` are normalized to `-1`, as are failed `close()` and `ioctl()` operations. Syntax errors and invalid arguments still stop execution. Temperature and networking examples and both language guides were updated. Note that `-1` alone cannot distinguish transient `EAGAIN` from another error. Review of kernel function tracing confirmed that instrumentation currently targets `kernel.c` only and records selected functions during SVC; tracing was not changed here.

## 2026-11-05: Unify Python I2C errors and rename fbwave.py

Tiny Python now uses only `i2c_write()`, `i2c_read()` and `i2c_xfer()`; the `i2c_try_*` functions are removed. Failed transfers return `-1`; successful writes return a byte count and successful reads return a byte list. List-vs-integer equality comparisons permit `data != -1` checks. `open()` returns `-1` on failure so callers can recover. Both earlier wave sample names were consolidated into `fbwave.py`, with a 150 ms fixed-speed fallback on I2C failure. ARM compilation and hardware validation remain separate checks.

## November 5: Experimental kernel-function tracing and readable call nesting

The existing `trace run hello` exposed system-call entries and exits, but not **which kernel functions were called between them**, such as `vfs_rw()` and `device_rw_tty()`. To make this path observable for teaching, an experimental `trace kernel command [args...]` mode was added. It supplements, rather than replaces, the existing system-call tracing in `trace run`.

`CMakeLists.txt` now defines `PICONIX_KERNEL_FUNCTION_TRACE`, **disabled by default**. When enabled, `-finstrument-functions` is applied primarily to `kernel/kernel.c`, and `__cyg_profile_func_enter()` / `__cyg_profile_func_exit()` provide function-entry and -exit hooks. Only selected functions relating to VFS, file descriptors, RAM files, TTY, devices, and pipes are recorded, and **only while executing an SVC handler**. It is not a complete trace of all kernel calls, interrupts, or scheduling. The feature shares the existing 16 KiB trace ring without enlarging the event record.

The trace display in `userland/apps/trace.c` was refined through several iterations. The `Type` column distinguishes `SYSCALL`, `KERNEL`, and other event categories; the `Function` column renders an entry as `vfs_rw` and its exit as `vfs_rw/`. Kernel calls are indented by two spaces per level under their system call, helping students follow the call hierarchy. `Arg`, `Ret`, and `Info` were clarified, with `--` for values **not actually recorded**, rather than invented kernel-function arguments or return values. `trace kernel -d 0..8 command` limits **display depth only**; it does not reduce instrumentation or ring-buffer usage. Compiler optimization and overwritten records can make the displayed hierarchy incomplete.

Examples include `trace kernel cat /tmp/test.txt`, connecting `open` and `read` to VFS and RAM-file operations, and `trace kernel omikuji`, following `/dev/rand` reads through `vfs_rw`, `device_rw_rand`, and `rand32`. Relevant sources include the hooks and recording logic in `kernel/kernel.c`, command execution and presentation in `userland/apps/trace.c`, and the event declarations in `include/piconix_trace.h`. Usage and limitations are described in the [tracing guide](reference/commands/TRACE.md).

Because compiler instrumentation affects even functions that are not ultimately recorded, enabling the feature increases Flash use, execution overhead, and stack use. The ordinary build therefore retains `OFF`; teaching or debugging builds can explicitly enable `-DPICONIX_KERNEL_FUNCTION_TRACE=ON`. The conversation included revisions and tests of output formatting and depth filtering, but **this diary does not claim exhaustive on-device verification across boards and code paths**. The displayed call hierarchy is generated from events; it is not a dump of raw MSP stack frames.

## November 5: Documenting CPU modes, stacks, and task switching

A discussion between the developer and ChatGPT examined the Cortex-M0+ **Thread and Handler modes**, **PSP and MSP**, and **SVC and PendSV** to make PicoNIX's kernel design easier to understand. Thread versus Handler describes normal execution versus exception handling, not simply privilege level. PicoNIX user tasks execute in privileged Thread mode with task-specific PSP stacks; SVC, PendSV, and SysTick handlers execute using the shared MSP. PSP and MSP are CPU stack-pointer registers, not memory regions themselves.

The task-switching path was mapped to `isr_pendsv` in `kernel/context.S` and `os_schedule()` in `kernel/kernel.c`, connecting automatic exception-frame stacking with software preservation of remaining registers, saved PSP values, `pend()` requests, and `isr_systick()` timekeeping. **SVC is the system-call entry mechanism; PendSV is the exception used for scheduling.** Requesting PendSV does not guarantee switching to a different task. Handler mode also does not universally disable interrupts: exception priorities govern nesting.

A central question was **how blocking I/O works without a separate kernel stack per process**. For TTY input, `device_rw_tty()` records the wait buffer and conditions in task state, blocks the task, and lets the kernel call return without retaining its call frames. When input arrives, `wake_readers()` and `task_unblock()` deliver data and update the return value in the saved SVC exception frame. `wait_frame_ready` helps prevent wake-up processing before the saved frame is ready. This does **not** establish that every device operation is asynchronous or free of short polling waits.

The discussion contrasted this approach with Research UNIX V5/V6 and Linux, where a process may sleep within a kernel function and later resume the same kernel call chain using its own kernel stack. **The need for per-process kernel stacks is not solely about kernel preemption; it also depends on whether blocked kernel execution must be preserved.** The explanation and source-reading exercises were added to the Japanese and English [kernel reading guides](development/kernel/KERNEL-READING-GUIDE.md). This was a documentation update only; it did not modify the kernel or include new on-device verification.

## November 5: Explicit I²C file descriptors in Python

The BH1750 Python helpers previously opened and closed `/dev/i2c/1` for every transfer. The C applications `bh1750` and `i2cctl` already open the bus once and use `ioctl(OS_I2C_TRANSFER)` on that descriptor. The Python helpers now take the descriptor explicitly: `i2c_write(fd, addr, data)`, `i2c_read(fd, addr, count)`, and `i2c_xfer(fd, addr, data, count)`. The caller owns the descriptor and closes it when finished. No changes were made to the kernel or C applications. The old Python call signatures are incompatible. ARM-target syntax checking passed; hardware validation remains outstanding.

## November 3–4: Shared screen control and more responsive tools

Duplicated editor logic was reduced, while terminal-size queries and ANSI control were consolidated in `screen.h`. `watch` moved beyond its fixed 20-line-by-78-column display, and `mem` gained a variable dump area of up to 36 lines instead of 16. `mb` adjusted the position of its fixed game area rather than expanding game mechanics. `pong` gained Up/Down arrow-key controls. Screen buffers retained explicit limits to respect the 24 KiB user slot, including the reserved 4 KiB stack.

The shell gained `Ctrl+A`, `Ctrl+E`, `Ctrl+W`, and `Ctrl+K` editing, plus Tab completion. A second Tab could show multiple candidates close to the prompt. These small changes highlighted how much care terminal redraw and noncanonical input require even in a compact system.

## November 4: The `fd` browser and PIC constraints

A small FDclone-style file manager, `fd`, was introduced. It supports arrow-key directory navigation, file viewing, launching the existing `edit` program, and periodic refresh while viewing. The first viewer was limited to the initial 2 KiB. Line-by-line scrolling and streaming reads allowed larger pseudo-files such as `/proc/sym/kernel` to be explored. The kernel-symbol file subsequently gained `lseek()`, and `fd` recorded a small index of offsets every 16 lines to revisit known positions efficiently.

A PIC validation failure exposed a subtle C restriction: the initializer `static const char *notice = "";` embedded an address-dependent pointer, so the binary differed when linked at two RAM positions. The initialization was changed and the C programming guide gained an explanation of why code supported by more general PIC systems can fail PicoNIX's relocation-free image test. An editor that briefly opened and exited was also traced to inherited `O_NONBLOCK` settings; terminal-mode handling around child launches was adjusted.

## November 4: VFS dot entries and foreground TTY handoff

The difficulty navigating upwards in `fd` prompted a distinction between **resolving** `.` and `..` in paths and **enumerating** those entries through `getdents()`. The VFS now lists both; `ls` hides them by default and shows them with `-a`, `fd` uses the returned parent entry, and `find`/`tree` skip both to avoid recursive loops.

Some interactive programs intermittently failed on restart with `terminal required`. A possible race let a newly spawned child execute before the shell handed over foreground TTY ownership. A foreground-spawn API was introduced to combine process creation with TTY ownership transfer. This addresses a plausible race by design, but it is not proof that every triggering condition has been eliminated.

## November 4: Wi-Fi diagnostics and the v1.3.0 release candidate

Occasional Telnet stalls in Pico W STA mode, absent in a nearby AP-mode setup, pointed towards environmental network differences. The initial `MAC: N/A` and `RSSI: N/A` output was not a failed reading; it was a placeholder. MAC address reporting was added for both modes, and STA RSSI was sampled periodically and cached. The indicator was simplified to `RSSI: -44 dBm (100%)`; the percentage is an RSSI-derived approximation, not measured throughput or packet-loss quality. AP mode shows its own MAC address and leaves upstream RSSI as `N/A`. The developer reported a hardware reading of `-44 dBm`.

The `refactor-20261103` branch combined the source layout changes, interactive tools, `fd`, VFS/TTY behavior, Wi-Fi diagnostics, and learning materials. After build and on-device checks, it was merged into `main`; a missing `fd` entry in the root README was also corrected. **v1.3.0 is the proposed next release**, but this diary update does not claim that the public release has already been published. Consult the Git history and GitHub Releases for the authoritative tag and publication date.

## November 4: `cat -n` and review for teaching

`cat` gained the optional `-n` flag to number every input line, including blank lines. It works with a file, standard input, and pipelines; the ordinary unnumbered byte-copy behavior remains unchanged. A host regression test exercises the real user-space C library, empty input, long lines, and binary data.

The review reconciled command-help descriptions with their implementations (`cat`, `ls`, `edit`, `dd`, `grep`, and others) and improved beginner exercises in Japanese and English. The C build guide now distinguishes the selected ARM compiler from the optimization setting: applications use `-Oz` and `-fPIC`. Documentation-link and host regression checks passed; **firmware linking and on-device operation still require verification in an ARM/Pico SDK environment**. The review is not a claim that every runtime path has been exhaustively tested.

## November 4: Packet inspection and ICMP experiments

The earlier decision not to ship a dedicated `ping` command was reconsidered for network teaching. Pico W gained proposed `ping` and `tcpdump` tools backed by `/dev/net/icmp` for ICMP Echo and `/dev/net/packet` for summarized IPv4 traffic. `tcpdump -x` can show the payload length and its first 16 bytes. Unlike Linux `tcpdump`, this interface is not a general Ethernet-frame capture facility and does not observe arbitrary traffic between other devices. The design uses the existing file-I/O system calls rather than exposing a general raw-socket API.

A registration mistake meant that `ping` and `tcpdump` could be built but still not appear in the kernel's runnable-command table. A later patch registered them and added a consistency check. Source-level and host checks do not by themselves demonstrate correct packet transmission and capture on Pico W hardware.

## November 4: Giving each command responsibility for its arguments

The initial `cat -n` failure exposed shell-side argument-to-path rewriting that treated `-n` as a file name. After a temporary special case, the design was simplified: the shell handles tokenization, pipes, redirection, and launching, while each command validates its own arguments; the VFS resolves relative paths. Argument handling for all 46 user commands was reviewed at the source level, and command-specific cases including `omikuji` were removed from the shell.

For `omikuji`, handling a standard-input read error as an implicit background default was abandoned in favor of an explicit rule: no argument means ten seconds, a numeric argument selects the delay, and only `-i` reads standard input.

## November 4: A compact C library with 64-byte streams

Existing implementations of `fgets()`, `sprintf()`, `snprintf()`, and `atoi()` were reused rather than duplicated. Functions including `scanf()` and `sscanf()` were added, followed by a small `FILE` abstraction with one 64-byte buffer shared between reading and writing. The interface gained `fopen()`, `fclose()`, `fread()`, `fwrite()`, `fprintf()`, `fscanf()`, and `fflush()`. Input lookahead is shared among `scanf()`, `getchar()`, and `fgets()`; standard output is line-buffered, and standard error is unbuffered. Normal-exit flushing and screen-control programs also required attention.

For teaching, it is useful to contrast direct descriptor I/O (`read()` and `write()`) with buffered C streams. The implementation also makes the need for `fflush(stdout)` before an interactive prompt visible. Host tests and ARM syntax checks were reported, but full PIC linking and on-device regression testing remained separate tasks.

## November 4: Interactive `omikuji` and library reuse

`omikuji -i` became a repeated-input example using `scanf()` to consume whitespace-separated integers. For example, `echo "3 4" | omikuji -i` processes two successive delays, ignores invalid tokens, and ends on EOF. Ordinary `omikuji` runs once after ten seconds; `omikuji 5` runs once after five. The countdown calls `fflush(stdout)` so students can observe why buffering affects interactive output. The `hexdump` application also replaced its private hex formatting with existing formatted-output functions.

## November 4: Documenting differences from Linux and implementation limits

The [Differences from Linux and limits](reference/DIFFERENCES.md) reference was added to distinguish PicoNIX from a full Linux/POSIX system. It explains the custom process-launch mechanism rather than `fork()`/`exec()`, the absence of POSIX process groups and full signal semantics, the absence of user accounts and filesystem permissions (all programs are effectively privileged), the RAM/device/pseudo-file VFS, USB CDC/UART1/Pico W Telnet terminals, and fixed limits on slots and descriptors. It also distinguishes absent features from deliberately simplified implementations.

These sections record design discussions and generated patches. They **do not establish a v1.3.0 release or prove that all changes passed hardware testing**; consult the actual source, tests, and Git history for authoritative status.

## November 4: Self-addressed networking and packet inspection on hardware

The developer confirmed that `ping` and `tcpdump` worked on Pico W hardware. A `ping` to `127.0.0.1` succeeded, but one to the Wi-Fi interface's own `192.168.4.1` address timed out. The investigation suggested that lwIP's per-interface loopback queue was not being polled for the Wi-Fi interface. The network polling path was revised, after which the developer reported successful pings to the local Wi-Fi address. This does not establish that every TCP and UDP self-connection path has been tested on hardware.

The format of `tcpdump -x` was refined for readability on a terminal: a one-line packet summary followed by one indented line containing up to 16 payload bytes in hexadecimal and ASCII. Ordinary `tcpdump` shows only the summary, while `-x` enables the detail line.

```text
RX TCP 192.168.4.2:52000 > 192.168.4.1:80 LEN=60 DATA=16 ACK PSH
    DATA:  47 45 54 20 2f 20 48 54 54 50 2f 31 2e 30 0d 0a | GET / HTTP/1.0..|
```

This is an illustrative layout, not captured packet output. Some patches initially carried an extra top-level directory and therefore did not overwrite files in the working repository. The patch packaging convention was corrected to make archives directly extractable at the Git repository root. The developer confirmed that a two-line version worked; the last `DATA:` label refinement still requires separate on-device confirmation.

## November 4: Interface listing, editor paths, and the browser PWM tester

A read-only `ip` command was added so learners can inspect network interfaces, including `lo0` and the Wi-Fi interface. The latter's actual lwIP name is displayed rather than assuming `wan0`. An initial patch did not land at the expected path, but after the archive layout was corrected the developer confirmed that `ip` worked.

The editor was changed to allow a second Ctrl+Q to discard unsaved changes. Another problem made `edit test.py` work under `/tmp` while `edit ./test.py` was rejected. The shared path normalization used by `edit` and `vi` was updated, retaining the rule that editable RAM files are directly under `/tmp`. These patches and their tests should not be confused with full device verification of every editor path.

The GP19 servo problem in the browser PWM tester was ultimately identified by the developer as a **loose cable connection**. Separately, browser requests were serialized with the shared PWM-frequency constraint of GP18 (buzzer) and GP19 (servo) in mind. Moving the servo slider now enables GP19 automatically; pressing a buzzer key stops the servo and switches to GP18. The servo Start/Stop control became a checkbox. The developer reported that this interaction worked as intended. The event should not be described as a confirmed PWM driver defect.

## November 4: Deferring the browser terminal (`ttyd`)

A lightweight WebSocket-based browser terminal was considered as an alternative to using a Telnet client. Hosting it as a kernel service would avoid placing the WebSocket server in a 24 KiB user slot, but HTTP Upgrade and WebSocket framing would increase kernel code and state complexity. A user-space approach would instead require an interface to connect the service to a TTY.

The developer decided **to prioritize a small, straightforward kernel over Flash capacity or an additional access method**, and deferred `ttyd`. USB CDC, UART1, and Pico W Telnet remain the supported terminal options. This was a deliberate scope decision for an educational OS, not a failed implementation attempt.

## November 4: Japanese source comments and a generated function index

With the v1.3.0 features and documentation reorganized, the next goal was to make **the source code itself easier for students to read**. Work used only the latest ZIP supplied that day, without copying files from older versions. Comments across C sources, headers, assembly, and Python tools were reviewed in Japanese. Process management, PIC execution, system calls, VFS, TTYs, pipes, networking, and memory management received particular attention to design choices and fixed-resource limits, rather than merely restating each operation. Obvious code was not intended to receive redundant commentary; function purpose, important arguments and return values, and constraints were explained where useful.

As a starting point for reading the implementation, `tools/gen_functions.py` was added to extract function definitions and their preceding descriptions into the file-by-file [`FUNCTIONS.md`](../ja/development/FUNCTIONS.md). It records function names, source line numbers, linkage scope such as `static`, and summaries, with links to the relevant lines. The first generated index showed descriptions for only **285 of 729 functions**. After checking extraction gaps and missing comments, descriptions were supplied for the remaining **444 functions**, giving all 729 indexed functions a summary. A regression test was also updated to flag missing descriptions. The lightweight source parser and summary coverage do **not** prove that every explanation is semantically correct.

The added comments made some function boundaries harder to see, so spacing was standardized: **one blank line separates a function from the following function's description, while the description remains adjacent to its definition**. Interior formatting and boundaries involving intervening type or constant definitions were left alone. The formatting pass adjusted **536 boundaries across 58 source files**, then regenerated `FUNCTIONS.md` to update source line links.

The work was intended to leave **program behavior, APIs, and memory layout unchanged**. Checks compared non-comment C content, Python syntax trees, and generated index output, and **all 86 existing regression tests passed**. However, **a full ARM firmware rebuild and on-device verification were not performed**. Comments and generated navigation aid code review and teaching; they are not substitutes for verifying the implementation itself.

## November 2–3, 2026: System-wide refactoring and regression tests (stages 1–7)

Starting with the latest source ZIP, the project undertook incremental refactoring intended to preserve existing behavior. Changes were delivered as archives containing only modified files, with diffs and host-based regression checks against the preceding stage.

Stage 1 addressed task switching during lwIP polling and socket cleanup triggered from the UART1 Ctrl+C interrupt path. Stages 2–3 reviewed Telnet's required `ERR_ABRT` return after `tcp_abort()`, received-pbuf ownership, TCP receive capacity, send failures, half-close, and deferred close handling. The historical `tcp_receive: valid queue length` assertion **remained unresolved**; preventive fixes are not proof of a root-cause resolution.

Stage 4 consolidated GPIO path parsing and improved user-pointer bounds checking. Stage 5 tightened HTTP request parsing and EOF handling in user-space `httpd`. Stage 6 unified FD inheritance, duplication, and release, including cleanup of inherited descriptors when starting `getty` on another TTY. Stage 7 consolidated task completion and unblock paths used by `waitpid()`, pipes, and terminal input.

A build failure was then discovered in Pico W STA mode: conditional compilation left calls to AP-only `ap_dhcp_server_start()` and `ap_dhcp_active` visible from `net_init()`. The conditional boundary was fixed, and AP/STA compile checks were added. **The developer reported a successful build after this fix.** Subsequent validation should distinguish ordinary Pico, Pico W AP, and Pico W STA configurations.

## November 3: Foreground TTY ownership and trace interrupts (stage 8)

A three-stage pipeline could lose its foreground TTY owner when its final process exited before the others. The exit path was revised to transfer ownership to a surviving foreground process and defer TTY-mode restoration until the pipeline was finished. The trace ring, written from SysTick, UART1, and SVC contexts, was also protected by a short interrupt-disabled critical section.

Host-based regression checks were reported as passing, but this stage still required a full Pico SDK build and hardware load tests.

## November 3: Reusing the 16 KiB network reservation (stage 9)

The network memory reservation was reviewed rather than simply increasing stacks everywhere. For Pico W, the lwIP heap was enlarged from 4 to 10 KiB and the 4 KiB idle stack moved into the 16 KiB reserved region, leaving roughly 2 KiB for boundaries and overhead. The pbuf pool increased from four to eight 512-byte buffers, while remaining in kernel RAM. The regular Pico layout was unchanged.

A new `/proc/net/memory` interface exposes heap, pbuf pool, and idle-stack use, high-water marks, and allocation-failure counters. Link-time layout verification was also extended. The reported 63 passing host regression tests did **not** establish a successful final Pico SDK link, measured hardware stack headroom, or long-duration TCP stability. The earlier TCP assertion is still a separate investigation.

## November 3: Reduced flicker in `watch` and `mem`

The original `watch` cleared and redrew the whole terminal on every cycle. It was changed to capture command output through a pipe and update only lines that differ from the previous display. Because the pipe holds only 256 bytes, output must be drained before waiting for the child to exit. The display uses about 3.2 KiB of buffers, with limits of 20 lines and 78 bytes per line; excess output is drained and indicated as truncated.

Both `watch` and `mem` also gained temporary ANSI autowrap suppression while rendering. Testing on physical USB, UART, and Telnet terminals remains important, particularly for interrupted output and terminal-mode restoration.

## November 3: Shared-memory assembly teaching example

A small Thumb assembly sample, `counter.s`, increments a 32-bit value at the start of shared memory (`0x20032000`). The result can be checked without a temporary file using `dd if=/dev/shm of=- bs=1 count=4 | hexdump`. This example was added to the Japanese and English `MEMORY-LAYOUT.md` documents. It illustrates shared state across process launches, **not** atomic updates under concurrency. Assembly and execution on PicoNIX hardware were not yet verified.

## November 3: Loopback adopted; PicoNIX `ping` and `curl` declined

The Pico W lwIP configuration gained the `lo0` loopback interface for local TCP/UDP communication through `127.0.0.1`. With the polled raw API, queued loopback packets are processed from `net_poll()` to avoid re-entering the TCP input path. The design keeps existing user commands such as `nc`, `httpd`, and `tcp_echod`, and the built-in Telnet server. Dedicated PicoNIX `ping` and `curl` commands were considered but ultimately **not adopted**. The date range follows the conversation-based reconstruction; it is not a verification of exact Git commit timestamps.

## November 3: Python TCP/UDP support, repeated ADC reads, and file seeking

The small Python interpreter gained `socket()`, `connect()`, `listen()`, `accept()`, `bind()`, `send()`, `recv()`, `sendto()`, and `recvfrom()` using existing socket system calls on Pico W. String literals also gained escapes including `\n`, `\r`, `\t`, and `\xHH`. No temperature-specific Python built-ins were retained: `/dev/adc/temp` is accessed through the existing `open()` and `read()` APIs. The TCP and UDP temperature servers were organized as `userland/examples/python/temp_tcp.py` and `temp_udp.py`, while the initial `/tmp/test.py` example was changed to sample the temperature five times. Hardware verification of the network examples was still pending at this stage.

To avoid reopening the ADC device for each sample, the ADC read logic was changed so that a new measurement is taken on the next `read()` after the previous textual result has been consumed; partial reads continue the same measurement. Rather than adding another system call, the existing **system call 21, `lseek()`**, gained `SEEK_CUR` and `SEEK_END`. Ordinary `/tmp` files and `/dev/shm` support all three origins, while `/dev/mem` retains only `SEEK_SET`. Host-side compilation and static tests were performed, but these do not establish comprehensive hardware behavior. For current APIs, see [Python](development/apps/python/PYTHON.md), [System calls](reference/SYSCALLS.md), and [C library](development/apps/c/LIBC.md).

## November 3: Standardizing user programs on `-Oz` and repairing the PIC runtime

After adding Python sockets, its task image exceeded the usable portion of a 24 KiB user slot (with 4 KiB reserved for stack), resulting in `task image overlaps reserved stack`. The limit of 16 variables, 64 elements per list, the 1,024-byte string/list pool, and binary integer literals were retained. Duplicated socket argument parsing was consolidated, and Clang optimization changed from `-Os` to `-Oz`. The reported Python PIC file size was **13,304 bytes**, with the link and placement checks for the 24 KiB slot passing. The file size alone does not establish runtime stack headroom.

The size experiment led to **`-Oz` becoming the standard for every user program**; the kernel retained Pico SDK `MinSizeRel` (`-Os`). Across 43 programs, the measured pre-link Clang object-size total changed from 112,457 bytes with `-Os` to 104,842 bytes with `-Oz`. These are not final Flash/RAM usage or performance measurements.

During migration, Clang emitted ARM EABI memory helpers such as `__aeabi_memcpy` that were missing from the user-space C library, preventing the shell from linking. Shared helper implementations were added to `libc.c`. A subsequent PIC check for `omikuji` found `.got` or `.dynamic` sections. A proposed application-level rewrite of `omikuji.c` was **rejected**: instead, library declarations in `stdio.h`, `stdlib.h`, `string.h`, and `unistd.h` were given `hidden` visibility, allowing calls to functions within the same PIC image without GOT references. **The developer reported that the build then completed successfully.** This does not imply that all commands and networking paths had already passed hardware regression testing. The approach keeps optimization and PIC ABI constraints in the shared runtime and build configuration rather than imposing special cases on individual applications.

## v1.2.1: Python networking and smaller user programs

After v1.1.0, the small Python interpreter gained TCP/UDP networking, along with improvements to device and file operations. All user-space programs were also standardized on `-Oz` to reduce code size. These changes were released as **v1.2.1**. This section summarizes the preceding November 2–3 account rather than claiming a separate implementation event on the date of this update.

## November 3: Separating source code by responsibility

Applications formerly mixed under `userland/` were moved to `userland/apps/`; shared library code, startup code, and linker settings moved to `userland/lib/`. The editor-specific `editor_common.h` remained with the apps, while reusable terminal control in `screen.h` belonged in the library. Three CMake image-embedding templates moved into `cmake/`, and shared tracing definitions moved into `include/`.

Even a nominally structural change affected CMake paths, tests, relative includes, and documentation links. The migration included checks of old-file removal and eventually a fully assembled source ZIP. The developer reported a successful Pico SDK build on the refactoring branch. The goal was not only a tidier tree, but a codebase whose changes could be tested and explained.

## November 3: Documentation as a staged undergraduate learning path

Documentation was reorganized into Setup, Literacy, Development, and Reference. Development was subdivided into C, Python, and assembly application programming; system-call extension; device drivers; and kernel development. Device interfaces, circuit information, and pin assignments were placed under Reference, with links from Setup. This lighter historical diary remained at the root of `docs/`.

A **ten-topic** curriculum was assembled: basic UNIX commands, standard I/O and IPC, device I/O, processes, memory and execution, terminals, system monitoring, networking, programming, and OS extension. Hands-on exercises were separated from detailed API references, with observation and reflection prioritized. English and Japanese documentation links were checked together.

## November 1–2: ARP diagnosis, mDNS, and v1.1

In STA mode, Linux sometimes failed to connect over Telnet after idle periods. Its neighbor entry could remain `INCOMPLETE`, while manually adding the Pico W's MAC address restored connectivity. The team tried periodic Gratuitous ARP (GARP), initially every 60 seconds. When GARP reached Linux, it could update the pending neighbor entry and allow Telnet to connect. The interval was temporarily reduced to five seconds for diagnosis.

PicoNIX counters showed that GARP send requests were issued regularly, and further instrumentation counted ARP Requests and Replies passed to lwIP's link-output function. This demonstrated a crucial distinction: **a successful send API return does not prove the packet arrived at the receiving host**. In one experiment, Linux received zero replies to 30 `arping` probes, while the PicoNIX ARP Reply counter increased by 30 between measurements. Those counts are consistent, but individual responses were not correlated with individual probes.

On the Linux Wi-Fi interface (`iwlwifi`), captured GARP packets sometimes disappeared for long periods and then arrived in bursts. Moving the **same Linux PC to a wired port on the BUFFALO router** produced precise five-second GARP arrivals. Switching to another access point also produced nearly regular five-second delivery over Linux Wi-Fi. This made an AP/client wireless interaction much more plausible than a simple fault in PicoNIX's 250 µs polling loop. The evidence did **not** establish whether the BUFFALO access point or the Linux driver was individually responsible. The intended GARP interval was restored to 60 seconds. Setting Linux `arp_accept=1` also allowed GARP to create a neighbor entry, although this did not establish a root-cause fix.

The project also introduced **mDNS**, allowing connection without memorizing an IP address. STA mode uses a Flash-ID-derived hostname such as `piconix-4353ba2e.local`; AP mode uses `piconix.local`. The first implementation encountered a panic caused by exhaustion of lwIP's `MEMP_SYS_TIMEOUT` pool. Increasing the timeout resources resolved that particular issue, and the developer subsequently reported that mDNS worked on hardware. The available conversation does not independently establish successful tests of both STA and AP modes. mDNS and ARP remain separate mechanisms: successful name resolution does not guarantee MAC-address resolution.

mDNS also pushed the 64 KiB kernel RAM allocation beyond its limit. Reducing the number of RAM files was considered, but the adopted approach moved the 16 KiB trace ring to previously unused SRAM at `0x20034000..0x20037fff`. This preserved tracing and all six user slots while freeing room inside the kernel's primary memory region.

## v1.1.0 release prompted by mDNS support

**mDNS support on Pico W was the principal change that prompted the v1.1.0 release.** In STA mode, a device could be reached by a Flash-ID-derived hostname such as `piconix-XXXXXXXX.local`; in AP mode, the hostname was `piconix.local`. This made access possible without manually entering an IP address. mDNS provides name resolution; it does not itself resolve the ARP communication issue investigated earlier.

At the v1.1.0 release, PicoNIX included a shell, independent user commands, VFS, device files, TTY handling, pipes, a small Python-like interpreter, an assembler, an editor, `trace`, `mem`, `lsof`, network services, and mDNS. Its purpose was not to reproduce Linux wholesale; it was to **make UNIX concepts visible in a small, inspectable implementation**.

Recurring lessons included:

1. **Test on hardware.** Compilation and successful API returns are not sufficient evidence.
2. **Build observability.** Use `trace`, `mem`, `/proc`, tests, and packet counters to understand what happened.
3. **Revisit placement before removing features.** XIP, PIC, and SRAM layout matter.
4. **Clarify responsibility boundaries.** Keep kernel/user, TTY/screen, and VFS/device concerns distinct.
5. **Change one variable at a time.** Relate experiments directly to hypotheses.
6. **Design for teaching.** A system that students can inspect and modify matters more than an unchecked feature count.

## November 2: I²C sensors and generic Python file I/O

Building on the existing device-file model for the LED matrix, GPIO, and PWM, the developer chose to add I²C. **GP26 (formerly ADC0) and GP27 (formerly ADC1) were reassigned to I²C1 SDA/SCL**, retaining ADC2 on GP28. This fixed pin assignment avoids disrupting the existing LED-matrix wiring. The proposed kernel device is `/dev/i2c/1`. The user command was renamed from `i2c` to **`i2cctl`** and designed for scanning, reads, writes, and transfers using repeated START.

An earlier Linux kernel module named `smile.c`, written by the developer for the BH1750 light sensor, provided a concrete migration exercise. The new user-space `bh1750.c` uses PicoNIX device access rather than Linux `i2c_client` and kernel threads. It targets address `0x23`, powers on the sensor, selects continuous high-resolution measurement, reads two bytes, and converts the reading to lux using integer arithmetic. The key design lesson was that **sensor-specific logic belongs in a user program**, not in the kernel.

This also exposed limitations in the small Python-like interpreter. Prototype changes added **integer lists, byte strings**, indexing, `len()`, `bytes()`, and I²C helper functions, with a proposed 64-element list limit and 32-byte I²C transfer limit. Floating-point support was discussed but **deferred** until its impact on the **24 KiB user slot** could be measured. Fixed-point integer arithmetic remained sufficient for the BH1750 example.

The next insight was that byte strings could drive the LED framebuffer directly. Instead of adding more hardware-specific Python functions, the design moved toward generic **`open()`, `read()`, `write()`, `close()`, and integer-argument `ioctl()`** wrappers. GPIO-specific helpers were removed in the prototype, allowing the same file-descriptor interface to address `/dev/gpio/fb`, PWM, GPIO, UART, and RAM files. I²C-specific helpers were retained for now because combined transfers require structured arguments. **Hexadecimal `0x` and binary `0b` literals** were also added to make sensor addresses and bit patterns easier to express.

Japanese and English documentation for I²C and Python was expanded. Initially these changes were delivered as patch ZIPs. Subsequent work corrected build include paths, missing application registration, and a scan bug that falsely detected every I²C address. **Hardware testing then confirmed `i2cctl scan`, approximately 500 lx readings from `bh1750 5`, Python I²C transfers and lux calculations, framebuffer writes, and display enable via `ioctl()`.** Python integer floor division (`//`), multiline `bytes([...])`, and REPL continuation were improved and tested in scripts and the REPL. These checks do not constitute exhaustive regression testing or a final SRAM-size audit. This work belongs to **development after the v1.1.0 release**.

## November 2: dd, vi, and binary transfers over the network

A small modal `vi` editor was added alongside `edit`. At this point, movement uses `h`, `j`, `k`, and `l`; arrow-key navigation remains a future improvement. The new `dd` utility can copy files and raw memory through `/dev/mem`. Its `if=-` and `of=-` options use standard input and output, while the TCP half-close behavior of `nc -N` makes binary pipelines between Linux and PicoNIX practical.

Hardware tests reported successful RAM-to-`/tmp` dumps and file/memory-data transfers using `dd`, `nc`, and shell pipelines. The combined `DD-VI.md` document was retired in favor of separate bilingual `DD.md` and `VI.md` guides, with `NC.md` updated accordingly. See [dd and binary transfer](reference/commands/DD.md).

## November 2: SRAM reorganization, independent RAM files, and shared memory

The RAM-file data bytes were separated from kernel-owned file metadata and placed in a fixed-address region. Capacity increased from 1 KiB per file to **11 files × 2 KiB**. An independent **2 KiB shared-memory region** was introduced as `/dev/shm`, accessible via the device, physical addresses through `/dev/mem`, and direct pointers in PIC programs. Raw writes to RAM-file data intentionally do not update logical file sizes in the metadata table.

The new 256 KiB main SRAM layout assigns 56 KiB to the kernel, 6 × 24 KiB to user slots, 2 KiB to SHM, 22 KiB to RAM files, 16 KiB to trace, and 16 KiB to networking. Trace and memory inspection address references, PIC slot placement, the lwIP heap pointer, and layout verification scripts were updated together. **The Pico W ELF audit reported 41,460 allocated kernel bytes, 15,884 bytes of linker-address headroom, and PASS for the static layout checks.** Linker headroom is not runtime free heap.

On hardware, `/proc/memory` showed the expected layout and `/proc/ramfiles` showed file data addresses spaced by 2 KiB. After writing `hello` to `/dev/shm`, the 16-byte reads through `/dev/shm` and `/dev/mem` at `0x20032000` matched. Regular file reads and physical-address reads of RAM-file contents also matched; raw memory edits appeared in file reads. Both `mem` and `trace` were reported to work, and Pico W communication was reported functional. These reports do not establish an exhaustive AP/STA regression test. See [Memory layout](reference/MEMORY-LAYOUT.md).

## November 2: Python strings, bitwise operators, and memory limits

The small Python-like interpreter gained string variables and `print()` with multiple arguments, such as `print([0x13, 0x12], "hello", 123)`. `input()` was changed to return a string, with `int(input())` and `str(123)` providing conversions. Old scripts expecting numeric input require changes.

Bitwise operators (`<<`, `>>`, `&`, `|`, `^`, `~`) were then added to make LED framebuffer manipulation more natural. The first build failed with **`task image overlaps reserved stack`**: the 24 KiB task slot must reserve 4 KiB for stack. The implementation was made smaller by sharing array and string storage through a C `union`, consolidating expression parsing, and shrinking line bookkeeping. The developer subsequently reported a successful build and working bitwise expressions on the device. This did **not** measure worst-case runtime stack consumption.

A BH1750-based Python LED-matrix animation was also tested on hardware: waves move more quickly in brighter light and slowly in darkness. The working example was documented in `PYTHON.md` without adding another boot-time RAM file. Together these changes demonstrate sensor input, computation, and framebuffer output using a small UNIX-like interface. Stack high-water measurement and further regression tests remain useful future work.

## November 2: 2 KiB editors and final documentation consistency review

After RAM files had been expanded to 2 KiB each, both `vi` and `edit` still used 1,024-byte editing buffers. Saving a larger file could silently lose its tail. Their static buffers were increased to **2,048 bytes**. When a read fills the buffer exactly, an additional one-byte read checks for excess content or a read error before allowing editing. `VI.md` and `EDIT.md` were updated accordingly. This increases static data use without changing the reserved 4 KiB stack or 24 KiB task slot. Editing and re-saving files at the 2 KiB boundary still needs separate hardware verification.

Because the goal is operating-system education rather than teaching the full Python language, the opening of `PYTHON.md` now identifies the interpreter as a **limited subset**, not a CPython-compatible implementation. The BH1750-driven wave example remains in documentation instead of using an additional boot-time RAM-file slot.

A documentation-wide search reviewed the current key limits (56 KiB kernel, six 24 KiB application slots, eleven 2 KiB RAM files, 2 KiB SHM, and the **then-current** 1,024-byte Python source limit). That Python source limit was subsequently raised to 2,048 bytes on the same day. An outdated **64 KiB** kernel check in English `WIFI-MODES.md` was corrected to **56 KiB**. Historical values in the diary and `docs/history/` remain where they describe earlier designs. This textual check is not an exhaustive hardware test or a sentence-by-sentence bilingual translation audit.

## November 2: Shared Python pool and 2 KiB source and assembler output limits

After expanding RAM files and the `vi` and `edit` buffers to 2 KiB, the Python interpreter and assembler were also updated to accept **2,048-byte source files**. Simply enlarging Python's source buffer initially caused a link failure because the task image overlapped the 4 KiB stack reservation in the 24 KiB user slot. Removing multi-argument `print()` was considered, but the chosen approach preserved that feature and reduced array storage overhead instead.

Python now stores array and string data in a **shared pool of 256 32-bit words (1,024 bytes)** rather than reserving a full 64-element array for every variable. The limits remain 16 variables, 64 elements per array, and 64 bytes per string. Scalar integers do not consume pool space. Reassignment compacts and reuses the pool without `malloc()`, and exhaustion produces an error. The revised 2,048-byte source version compiled for ARM.

On hardware, the developer verified that replacing arrays and retaining other arrays and strings worked, and that the existing BH1750 light-controlled wave still ran. One attempted pool-limit test had an unintended 34-element array among otherwise 32-element arrays, giving 258 elements and an error on the eighth assignment. This was an over-capacity test, **not** evidence of a boundary-condition defect. The exact 256-element boundary and exhaustive reassignment cases have not yet been verified.

The assembler's source maximum was likewise raised to **2,048 bytes**. The machine-code/data output maximum was expanded to **2,040 bytes**, leaving 8 bytes for the `PNX1` header within a 2 KiB RAM file. The developer reported successful operation after this change. The six 24 KiB user slots and their 4 KiB stack reservations remain unchanged.

These changes prioritize predictable fixed memory usage for OS education rather than expanding PicoNIX Python toward full Python compatibility. See [Python](development/apps/python/PYTHON.md) and [Assembler](development/apps/asm/ASM.md) for the current limits.

## October 31–November 1: Pipes, TTY recovery, and documentation

The drive for UNIX-like behavior led to pipes, `dup2`, EOF handling, and file-descriptor management, allowing shell pipelines. Multi-line paste, nested shell processes, foreground termination, and lingering TTY state revealed interactions between otherwise working features. Some apparent runtime problems were also resolved by discarding stale build directories.

Development practices became more explicit: changes should be based on the latest supplied source archive, not mixed with files from older ZIPs; deliverables should contain only changed files in their original directory structure; and Japanese and English documentation should remain aligned with the implementation. Developer instructions for adding system calls and device nodes were separated from user manuals.

## October 30–31: Pico W networking

In an October 30 test, a script containing the stand-alone expression `1+2+3` produced a syntax/runtime error when invoked as `python /tmp/test.py`. This highlighted the need to distinguish unsupported interpreter syntax from file-loading failures.

Pico W support introduced CYW43 and lwIP, Wi-Fi station and access-point modes, DHCP, TCP/UDP sockets, Telnet, HTTP, and remote device control. Responsibilities were deliberately separated: **Telnet (TCP 23) and UDP device control (UDP 2000) remained kernel services; HTTP became a user-space `httpd` program**.

The initial networking work involved difficult debugging. DHCP DISCOVER packets reached PicoNIX, and the code could generate an OFFER, yet sending it sometimes led to a stop. Counters and packet captures were used to separate receive callbacks, DHCP processing, buffer handling, and transmission. Telnet initially failed because the configured lwIP listen-PCB pool had insufficient capacity, requiring a resource-setting change.

As network use increased, partial writes and temporary inability to send data became relevant. Garbled or truncated `mem` and `trace` output over Telnet motivated retries on `EAGAIN` and adjustments to Telnet buffering.

## v1.0.0 release: networking as the milestone

**v1.0.0 was released with Pico W networking as its principal milestone.** Wi-Fi station and access-point modes, TCP/UDP communication, Telnet, HTTP, and UDP device control extended PicoNIX from a standalone embedded OS to one that could also be accessed over a network. This networking support was the defining change for v1.0.0.

Later, **mDNS support prompted the v1.1.0 release**, enabling hostname-based access. Thus, networking marks v1.0.0, while mDNS marks v1.1.0. The exact release dates are not asserted here.

## October 29–30: VFS organization and scarce SRAM

Path resolution, device listing, and repeated file-handling logic were reviewed while preserving `/bin`, `/dev`, `/proc`, and `/tmp`. Splitting the VFS into separate source files was explored, but parts were eventually folded back into the kernel because extra module boundaries did not always simplify a very small system. Utilities such as `find` and `tree` also helped exercise VFS behavior.

Memory allocation remained a recurring design constraint. Task-slot size and count, the trace ring, the 64 KiB kernel region, and Wi-Fi reservations were reconsidered repeatedly. The guiding question became not simply what could be removed, but **what memory was actually allocated and where**. Intermediate 32 KiB-slot designs should not be confused with the later **six 24 KiB slots** used in the configuration leading up to v1.1.

Device allocation also required coordination with real hardware: GPIO pins reserved for UART, PWM, and the LED matrix could not be presented as freely available general-purpose pins.

## October 28–29: Terminals, devices, and observability

Terminal behavior was unified across USB CDC and UART1 where possible. The design gained `stty`, canonical and noncanonical input, and `O_NONBLOCK` via `fcntl`. The division of responsibilities mattered: the kernel manages TTY behavior, while applications manage screen presentation. LF-to-CRLF conversion was constrained to TTY output.

Line editing, history, arrow keys, pasting, Ctrl+C, and terminal reset were not trivial. Real-device tests uncovered damaged paste input and unexpected newlines. These incidents shaped the terminal abstraction rather than being treated as isolated cosmetic bugs.

Interactive `gpio`, `mem`, `trace`, and `lsof` tools made the system observable. `mem` evolved to show user and kernel symbols alongside memory contents; `trace run hello` could expose the system calls of a simple user program. These features were particularly valuable for teaching.

Games such as `moonbuggy` (`mb`) and `pong` were also introduced. Their need for responsive keyboard input provided a practical reason to improve noncanonical terminal handling. GPIO, ADC, PWM, an LED matrix, and a buzzer were increasingly exposed through device-file interfaces.

## October 27: PIC, XIP, and independent commands

User programs moved toward **position-independent code (PIC)** so the same binary could run in different fixed RAM slots. Startup code (`crt0`), argument passing, linker configuration, and PIC-image checks were improved. The kernel remained fixed-address, while user applications were linked as PIC.

To reduce SRAM pressure, kernel instructions and read-only data were moved toward execute-in-place (XIP) from Flash. User application images were stored in Flash and copied into RAM slots when launched. During this work the build encountered `.bss` overflows, `__aeabi_*` dependencies, `libgcc` linkage issues, and PIC validation failures. A proposed easy reduction in tracing capacity was not adopted; the preferred strategy was to improve placement and linkage instead.

Commands previously embedded in the shell—including `ls`, `pwd`, `echo`, and `rm`—were separated into user programs. This required better `argv` handling, process startup, current-working-directory inheritance, and path resolution. At one stage external commands failed with `unknown command`, showing that making an environment more UNIX-like often requires changing the execution model, not merely adding utilities.

Tools such as `/dev/mem`, memory inspection, and system-call tracing also emerged, laying the groundwork for an educational OS that could explain its own behavior.

## October 26: Naming PicoNIX and defining constraints

The name **PicoNIX** was chosen on October 26. The startup banner, `piconix#` prompt, help text, and documentation were aligned with that identity. This marked a shift from an experiment to a maintained project.

Compilation options had an immediate effect on RAM usage: build settings affected size and linker results, so the project adopted `MinSizeRel` (`-Os`) and emphasized size and SRAM-layout checks. Instead of reducing RAM file capacity immediately, the project favored revisiting memory placement and build settings first.

A small Python-like interpreter was tested on real hardware. Integer arithmetic, variables, conditionals, loops, I/O, and GPIO25 control were exercised. Multi-line pasting into the terminal sometimes corrupted or lost input. 

Another early lesson was consistency: a device that could be opened should also appear in directory listings. Differences between the device table and file listing behavior motivated later VFS consolidation.

## October 24–25: Starting from a small operating system

The project began with a practical question: how much of the UNIX model could be implemented on an RP2040? Rather than simulate a command line alone, the early design separated the kernel from user programs and introduced task execution, system calls, and basic process operations. SysTick, PendSV, and SVC formed the foundation for scheduling and kernel entry, alongside a USB CDC terminal, shell, and operations such as `spawn`, `waitpid`, `kill`, and `sleep`.

Full POSIX compatibility was never the immediate goal. The priority was to expose core UNIX concepts—processes, files, and I/O—on a microcontroller without an MMU. Early versions used a relatively generous memory arrangement, including **three 64 KiB user RAM slots** at one stage. Later versions repeatedly changed the number and size of slots as executable placement improved.

The initial shell, RAM files, device access, UART and USB I/O also revealed hardware-specific problems. USB CDC transmission waits and connection state could affect responsiveness. This made hardware testing essential from the beginning.

## Historical accuracy and references

This is a narrative reconstruction from conversations, not an exhaustive Git audit. A code change proposed or produced by ChatGPT is not, by itself, evidence of a successful hardware test. Exact commit dates, every ZIP revision, and all hardware logs have not been independently correlated. Proposed-but-rejected approaches and intermediate configurations are distinguished from the later implementation as far as the available record permits. Commit hashes and links to test logs may be added later.

For the current behavior and usage, consult [Internal architecture](reference/INTERNAL.md), [Networking](reference/network/NETWORK.md), [TTY](reference/commands/TTY.md), [Pipes](reference/commands/PIPE.md), [Tracing](reference/commands/TRACE.md), and [Kernel development](development/kernel/KERNEL-DEVELOPMENT.md), and the [Kernel reading guide](development/kernel/KERNEL-READING-GUIDE.md).

---

**Related links:** [Documentation index](README.md) | [Project overview](../../README.md) | [日本語](../ja/DEVELOPMENT_DIARY.md)
