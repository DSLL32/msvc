#!/usr/bin/env python3
"""Explicit original-build tools only; fixtures must not call this helper."""
import os
from pathlib import Path
import subprocess
import sys

from inventory import ROOT, inventory

inventory()
tools = {
    "WCC386": "R:\\toolchains\\watcom10.0a\\binnt\\WCC386.EXE",
    "WLINK": "R:\\toolchains\\watcom10.0a\\.patched\\WLINK.EXE"
}
if len(sys.argv) < 2 or sys.argv[1] not in tools:
    raise SystemExit("Expected WCC386 or patched WLINK")
binary = Path(__file__).resolve().parents[1]
if Path.cwd().resolve() != binary:
    raise SystemExit("Invoke original build tools from the binary folder")
environment = os.environ.copy()
for name in ("WCC", "WCC386", "INCLUDE", "WLINK", "WLINK_LNK"):
    environment.pop(name, None)
environment.update(WINEPREFIX="/root/.ai-tmp/vc1.0-capture/wine", WINEDEBUG="-all",
                   WATCOM="R:\\toolchains\\watcom10.0a")
arguments = []
for value in sys.argv[2:]:
    # Source/object paths are relative to the binary's stable repository cwd.
    arguments.append(value.replace("/", "\\"))
raise SystemExit(subprocess.call(["wine", tools[sys.argv[1]]] + arguments, env=environment))
