#!/usr/bin/env python3
"""All RAM applications in CMake must be registered for execution."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
cmake = (root / "CMakeLists.txt").read_text()
main = (root / "kernel/main.c").read_text()
apps = set(re.search(r"foreach\(APP IN ITEMS (.*?)\)", cmake, re.S).group(1).split())
registered = set(re.findall(r'register_image\(\d+,\s*"([^"]+)"', main))
# The embedded file uses "shell" as an image name but "sh" as its command name.
registered.discard("sh")
registered.add("shell")
registered.discard("as")
registered.add("asm")
assert apps == registered, f"unregistered={sorted(apps-registered)} unexpected={sorted(registered-apps)}"
for app in apps:
    assert f"piconix_{app}_image_start" in main, f"missing embedded image declaration: {app}"
print(f"PASS: {len(apps)} RAM applications registered")
