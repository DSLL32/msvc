#!/usr/bin/env python3
"""Explicit original compiler/capture transport; never imported by fixtures."""
import os
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
ROOT = PROJECT.parents[2]
SCRATCH = Path.home() / ".ai-tmp/msvc2"
PREFIX = Path.home() / ".ai-tmp/vc1.0-capture/wine"
TC = ROOT / "toolchains/5.10"


def windows(path):
    path = Path(path).resolve()
    for base, drive in ((ROOT, "R:"), (SCRATCH, "S:")):
        if path.is_relative_to(base):
            return drive + "\\" + str(path.relative_to(base)).replace("/", "\\")
    raise ValueError("path outside fixed repository/scratch drives")


def environment():
    assert (PREFIX / "dosdevices/r:").resolve() == ROOT
    assert (PREFIX / "dosdevices/s:").resolve() == SCRATCH
    env = os.environ.copy()
    env.update(WINEPREFIX=str(PREFIX), WINEARCH="win32", WINEDEBUG="-all",
               WINEDLLOVERRIDES="msvcrt=n", WINEPATH=windows(TC / "bin"),
               INCLUDE=windows(TC / "include"), LIB=windows(TC / "lib"))
    for name in ("CL", "_CL_"):
        env.pop(name, None)
    return env
