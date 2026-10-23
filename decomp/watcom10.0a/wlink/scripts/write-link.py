#!/usr/bin/env python3
"""Write an explicit original Watcom link response, never execute it."""
from pathlib import Path
import sys

if not sys.argv[1:]:
    raise SystemExit("No reconstructed objects")
lines = [
    "format windows nt", "runtime console", "debug all",
    "option map=build\\original\\wlink.map", "name build\\original\\WLINK.EXE",
]
for name in sys.argv[1:]:
    path = Path(name)
    if path.is_absolute() or ".." in path.parts or not name.startswith("build/original/"):
        raise SystemExit("Object outside owned original build directory")
    lines.append("file " + str(path).replace("/", "\\"))
lines.extend([
    "libpath R:\\toolchains\\watcom10.0a\\lib386",
    "libpath R:\\toolchains\\watcom10.0a\\lib386\\nt",
    "library clib3r", "library nt"
])
Path("build/original/wlink.lnk").write_text("\n".join(lines) + "\n")
