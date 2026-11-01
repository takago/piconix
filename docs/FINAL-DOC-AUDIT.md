# PicoNIX documentation final review / 最終点検

## Scope / 対象

- Reviewed the combined documentation set: history-separated repository, bilingual corrections, and filesystem documentation patch.
- Compared 30 current Japanese documents with their 30 English counterparts by filename, heading counts, and code-fence counts.
- Checked local Markdown link targets for existence across the repository.
- Compared important fixed values against the current C implementation (`kernel/kernel.c`, `include/piconix.h`, and `kernel/net_socket.h`).
- Did not run a complete firmware build or hardware tests. This is a static documentation review, not a formal proof of complete translation equivalence.

## Verified / 確認済み

- Current docs: 30 Japanese and 30 English, with no missing filename counterpart.
- Relative Markdown links: no missing target files detected.
- Six user slots of 24 KiB; two 256-byte pipes; four user TCP socket slots; UDP transfer limit of 512 bytes; syscall `dup2` number 18 and `dup` number 37 match the source.
- Trace ring **corrected** to 512 records × 32 bytes = 16 KiB (`TRACE_CAPACITY` in `kernel/kernel.c`). Earlier docs incorrectly said 1,024 events, and INTERNAL.md incorrectly said 16-byte records.

## Editorial changes / 編集上の修正

- Removed unrelated `/tmp/test.s` and `/tmp/test.py` sample-file description from the Wi-Fi modes guide where present; these samples are documented in the filesystem guide.
- Corrected the trace record size and ring capacity in both languages and in both TRACE.md and INTERNAL.md.

## Remaining limits / 未検証事項

- Exact semantic equivalence of every paragraph in all 30 pairs was not established by automated structural checks. Translation wording and individual examples may still need a human bilingual review.
- Source-code consistency was sampled for key constraints; it was not an exhaustive line-by-line verification of every command, option, and device path.
- No firmware build or on-device validation was performed.
- Historical notes are intentionally not required to have Japanese/English counterparts.
