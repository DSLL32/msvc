#!/usr/bin/env python3
"""Explicit original-toolchain builds only; never used by native fixtures."""
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
TOOLCHAIN = ROOT / "toolchains/5.10"
PREFIX = Path.home() / ".ai-tmp/vc1.0-capture/wine"


def windows(path):
    path = Path(path).resolve()
    scratch = Path.home() / ".ai-tmp/msvc2"
    for drive, base in (("R:", ROOT), ("S:", scratch)):
        if path.is_relative_to(base):
            return drive + "\\" + str(path.relative_to(base)).replace("/", "\\")
    raise ValueError("original build path outside fixed R:/S: roots")


def environment():
    if not (PREFIX / "system.reg").is_file():
        raise ValueError("shared Wine prefix must already exist")
    env = os.environ.copy()
    env.update(WINEPREFIX=str(PREFIX), WINEARCH="win32", WINEDEBUG="-all",
               WINESERVER="/usr/lib/wine/wineserver64",
               WINEDLLOVERRIDES="msvcrt=n",
               INCLUDE=windows(TOOLCHAIN / "include"),
               LIB=windows(TOOLCHAIN / "lib"),
               WINEPATH=windows(TOOLCHAIN / "bin"))
    for name in ("CL", "_CL_", "LINK", "_LINK_", "MSC_CMD_FLAGS"):
        env.pop(name, None)
    return env


if __name__ == "__main__":
    raise SystemExit(subprocess.call(
        ["wine", str(TOOLCHAIN / "bin" / sys.argv[1])] + sys.argv[2:],
        env=environment()))
