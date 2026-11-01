# Memory symbols and task slots

This page replaces earlier experimental notes. For current usage see [Memory viewer](EDIT-MEM.md) and [Internals](INTERNAL.md).

The six application slots occupy `0x20010000`–`0x20033FFF`, with 24 KiB per slot. The `mem` viewer supports task-slot navigation (`1`–`6`), kernel RAM (`k`), and XIP Flash (`K`). Symbol information is available for application images and the kernel when included in the build. `mem -s sh` and `mem -s kernel` list symbols. Symbol labels shown while browsing task memory are based on the current task-to-slot mapping and may change as tasks exit or start.

Earlier references to `/proc/syms`, five 32 KiB slots, and missing kernel symbols describe older development snapshots and are not the current interface.
