#!/usr/bin/env python3
"""Test cc output naming; does not execute generated RP2040 machine code."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as d:
    d = Path(d)
    wrapper = d / 'test.c'
    wrapper.write_text('''#define main piconix_cc_main
#include "cc.c"
#undef main
int main(void) {
    char path[128];
    if (assembly_path("test.c", path, sizeof path) || strcmp(path, "test.s")) return 1;
    if (assembly_path("/tmp/test.c", path, sizeof path) || strcmp(path, "/tmp/test.s")) return 2;
    if (assembly_path("nested/name.with.dots.c", path, sizeof path) || strcmp(path, "nested/name.with.dots.s")) return 3;
    if (!assembly_path("test.s", path, sizeof path)) return 4;
    if (!assembly_path(".c", path, sizeof path)) return 5;
    if (!assembly_path("test.c", path, 6)) return 6;
    return 0;
}
''')
    exe = d / 'test'
    subprocess.run(['cc','-std=c11','-DPICONIX_HOST_TEST', '-I'+str(root/'userland/apps'), '-I'+str(root/'userland/lib'), str(wrapper), '-o',str(exe)], check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: output filename construction')
