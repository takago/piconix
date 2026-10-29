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
# Optional escape hatch for builds where flashing is not wanted.
flash=1
cmake_args=()
for arg in "$@"; do
    if [[ "$arg" == "--no-flash" ]]; then flash=0
    else cmake_args+=("$arg")
    fi
done
cmake -S . -B build -DPICO_BOARD=pico -DPICONIX_KERNEL_RAM=ON -DCMAKE_BUILD_TYPE=MinSizeRel "${cmake_args[@]}"
cmake --build build -j "$(nproc)"

# Flash only an already mounted Pico in BOOTSEL mode.
if (( flash )); then
    uf2="build/piconix.uf2"
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
