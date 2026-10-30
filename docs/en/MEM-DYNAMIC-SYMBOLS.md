# mem: automatically selected symbols for occupied task slots

`mem [address]` reads `/proc/tasks` to identify the program currently occupying the RAM slot containing the displayed address. It selects that program's Flash-resident symbols from `/dev/syms` and displays matching names to the right of the ASCII column. No application argument is needed.

- `mem 0x20018000` starts at an address. Keys `1`–`5` jump to task slots.
- `mem -s sh` retains the existing symbol-list command.
- The mapping is refreshed on display updates (approximately once per second), including when an application exits or a slot is reused.
- Missing task data or a missing symbol catalog leaves the symbol column empty without crashing or interrupting normal memory viewing. Explicit `mem -s` requests report errors and exit as before.
- Kernel symbols are not yet embedded in this build.

**Caveat:** the slot-to-process mapping is a snapshot; processes can change after it is read. Symbols are applied only relative to the matching task slot, not arbitrary addresses.
