#!/bin/sh
# Apply this snapshot to a clean PicoNIX Git clone; do not commit or push automatically.
set -eu
if [ "$#" -ne 1 ]; then
  echo "Usage: ./update.sh /path/to/PicoNIX-git-clone" >&2
  exit 2
fi
SOURCE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TARGET=$(CDPATH= cd -- "$1" && pwd)
if [ "$SOURCE" = "$TARGET" ]; then echo "Source and Git clone must be different directories." >&2; exit 1; fi
if [ ! -d "$TARGET/.git" ] && [ ! -f "$TARGET/.git" ]; then echo "Not a Git clone: $TARGET" >&2; exit 1; fi
if [ -n "$(git -C "$TARGET" status --porcelain)" ]; then
  echo "Git clone contains local changes or untracked files. Commit/stash/clean them first." >&2
  exit 1
fi
python3 - "$SOURCE" "$TARGET" <<'PY'
import pathlib, shutil, subprocess, sys
source,target=map(pathlib.Path,sys.argv[1:])
# Managed project directories and root docs only; never touch .git or unrelated files.
managed={'src','ram_apps','kernel','userland','tools','examples','docs','include'}
managed_root={'CMakeLists.txt','README.md','LICENSE','update.sh'}
source_files={p.relative_to(source).as_posix():p for p in source.rglob('*') if p.is_file() and '.git' not in p.parts}
tracked=subprocess.check_output(['git','-C',str(target),'ls-files','-z']).decode().split('\0')
for rel in filter(None,tracked):
    p=pathlib.PurePosixPath(rel)
    if (p.parts[0] in managed or (len(p.parts)==1 and (p.suffix=='.md' or p.name in managed_root))) and rel not in source_files:
        (target/rel).unlink(missing_ok=True)
for rel,p in source_files.items():
    dest=target/rel
    dest.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(p,dest)
PY
# Stage renames, additions and deletions; let the user inspect before committing.
git -C "$TARGET" add -A
echo "Applied and staged. Review with:"
echo "  cd '$TARGET'"
echo "  git status --short"
echo "  git diff --cached --stat"
echo "Then commit and push manually after building/testing."
