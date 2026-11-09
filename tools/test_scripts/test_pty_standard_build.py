#!/usr/bin/env python3
"""Check that the PTY test is registered as a standard application."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
cmake=(root/'CMakeLists.txt').read_text()
main=(root/'kernel/main.c').read_text()
assert 'mcpd scroll pty_test)' in cmake
assert 'userland/apps/${APP}.c' in cmake
assert 'PICONIX_BUILD_PTY_SMOKE' not in cmake+main
assert 'register_image(56,"pty_test",piconix_pty_test_image_start,piconix_pty_test_image_end);' in main
assert (root/'userland/apps/pty_test.c').exists()
print('PASS: PTY test is a standard registered application')
