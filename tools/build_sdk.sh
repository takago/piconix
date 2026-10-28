#!/usr/bin/env bash
set -euo pipefail
: "${PICO_SDK_PATH:?Set PICO_SDK_PATH to a Pico SDK checkout}"
cd "$(dirname "$0")/.."
python3 tools/test_audit.py
python3 tools/test_pwm_paths.py
python3 tools/test_ram_pipeline.py
python3 tools/test_libc.py
python3 tools/test_trace.py
python3 tools/test_spawn_args.py
cmake -S . -B build -DPICO_BOARD=pico -DPICONIX_KERNEL_RAM=ON -DCMAKE_BUILD_TYPE=MinSizeRel "${@}"
cmake --build build -j "$(nproc)"
