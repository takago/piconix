#!/usr/bin/env bash
set -euo pipefail
: "${PICO_SDK_PATH:?Set PICO_SDK_PATH to a Pico SDK checkout}"
cd "$(dirname "$0")/.."
python3 tools/check_program_capacity.py
python3 tools/test_audit.py
python3 tools/test_pwm_paths.py
python3 tools/test_ram_pipeline.py
python3 tools/test_libc.py
python3 tools/test_trace.py
python3 tools/test_spawn_args.py
python3 tools/test_wifi_elf_audit.py
# Optional escape hatch for builds where flashing is not wanted.
flash=1
cmake_args=()
for arg in "$@"; do
    if [[ "$arg" == "--no-flash" ]]; then flash=0
    else cmake_args+=("$arg")
    fi
done
# Separate board configurations: never reuse a Pico CMake cache for Pico W.
board=pico
for arg in "${cmake_args[@]}"; do
    case "$arg" in -DPICO_BOARD=*) board="${arg#-DPICO_BOARD=}";; esac
done
build_dir="build-${board}"
cmake -S . -B "$build_dir" -DPICO_BOARD="$board" -DCMAKE_BUILD_TYPE=MinSizeRel "${cmake_args[@]}"
cmake --build "$build_dir" -j "$(nproc)"
# Resolve kernel ELF symbols after link. The embedded section has constant size,
# so the second link does not move code or data. Verify against the final ELF.
# The SDK toolchain may be selected with PICO_TOOLCHAIN_PATH instead of PATH.
NM=arm-none-eabi-nm
if [[ -n "${PICO_TOOLCHAIN_PATH:-}" && -x "$PICO_TOOLCHAIN_PATH/bin/arm-none-eabi-nm" ]]; then
    NM="$PICO_TOOLCHAIN_PATH/bin/arm-none-eabi-nm"
fi
python3 tools/make_kernel_symbols.py --nm "$NM" --elf "$build_dir/piconix.elf" \
  --output "$build_dir/piconix_kernel_symbols.bin"
cmake --build "$build_dir" -j "$(nproc)"
python3 tools/make_kernel_symbols.py --nm "$NM" --elf "$build_dir/piconix.elf" \
  --output "$build_dir/piconix_kernel_symbols.bin" --verify-only


# Flash only an already mounted Pico in BOOTSEL mode.
if (( flash )); then
    uf2="$build_dir/piconix.uf2"
    if [[ ! -f "$uf2" ]]; then
        echo "WARNING: $uf2 not found; skipping BOOTSEL flashing" >&2
    elif command -v findmnt >/dev/null 2>&1; then
        mountpoint="$(findmnt -rn -S LABEL=RPI-RP2 -o TARGET | head -n 1 || true)"
        if [[ -n "$mountpoint" ]]; then
            echo "BOOTSEL Pico detected at $mountpoint; writing $uf2"
            cp -- "$uf2" "$mountpoint/" || { echo "ERROR: UF2 transfer failed" >&2; exit 1; }
            sync
            echo "UF2 transfer complete"
        else
            echo "No mounted BOOTSEL Pico found; build only"
        fi
    else
        echo "findmnt unavailable; skipping automatic flashing"
    fi
fi
