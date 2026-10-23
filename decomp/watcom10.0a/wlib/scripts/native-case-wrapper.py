#!/usr/bin/env python3
"""Native debugger wrapper: preserve the original case's ordinary argv0 input."""
import os
import sys

os.execve(sys.argv[1], [r"R:\toolchains\watcom10.0a\.patched\WLIB.EXE"] + sys.argv[2:], os.environ)
