#!/usr/bin/env python3
"""Run source-level regression tests without requiring the Pico SDK.

Usage: python3 tools/run_tests.py [--all]
--all also runs test_ram_pipeline.py, which requires an ARM toolchain.
"""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
all_tests = sorted((root / "tools" / "test_scripts").glob("test_*.py"))
if "--help" in sys.argv:
    print(__doc__)
    sys.exit(0)
if len(sys.argv) > 2 or any(arg != "--all" for arg in sys.argv[1:]):
    sys.exit("usage: python3 tools/run_tests.py [--all]")
include_toolchain = "--all" in sys.argv
excluded = {"test_ram_pipeline.py"} if not include_toolchain else set()
failures = []
for test in all_tests:
    if test.name in excluded:
        print(f"SKIP: {test.name} (ARM toolchain required)", flush=True)
        continue
    print(f"RUN: {test.name}", flush=True)
    result = subprocess.run([sys.executable, str(test)], cwd=root)
    if result.returncode:
        failures.append(test.name)
if failures:
    sys.exit("FAIL: " + ", ".join(failures))
print("PASS: all selected regression tests")
