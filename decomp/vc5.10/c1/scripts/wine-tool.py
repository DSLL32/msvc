#!/usr/bin/env python3
"""Explicit original compiler transport; never imported by fixtures."""
import subprocess
import sys
from wine_tool_support import environment, windows, PROJECT, TC

if __name__ == "__main__":
    raise SystemExit(subprocess.call(["wine", windows(TC / "bin" / sys.argv[1])]
                                    + sys.argv[2:], cwd=PROJECT, env=environment()))
