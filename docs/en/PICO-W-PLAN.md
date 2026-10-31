# Pico W RAM layout (stage 1 only)

Build with `-DPICO_BOARD=pico_w` via `tools/build_sdk.sh --no-flash ...`. Use a **fresh build directory** when switching boards.

This stage limits user slots to four and leaves 0x20038000..0x2003ffff unused. No Wi-Fi firmware, AP, DHCP, or TCP is enabled yet. The reserved memory is **not yet an allocator** and is not automatically used by SDK libraries. Kernel remains limited to 96 KiB.

Normal Pico builds retain five slots. Do not flash the experimental Pico W image until SDK compilation and layout audits pass.
