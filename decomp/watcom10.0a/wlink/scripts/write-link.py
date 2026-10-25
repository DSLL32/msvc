#!/usr/bin/env python3
"""Write this binary's period Watcom link grammar; never launch any process."""
import argparse
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--response", type=Path, required=True)
p.add_argument("--output", type=Path, required=True)
p.add_argument("--map", type=Path, required=True)
p.add_argument("objects", nargs="+")
a = p.parse_args()
def local(value):
    value = Path(value)
    if value.is_absolute() or ".." in value.parts or not str(value).startswith("build/"):
        raise SystemExit("Expected an owned build-relative path")
    return str(value).replace("/", chr(92))
lines = ["format windows nt", "runtime console", "debug all",
         "option map=" + local(a.map), "name " + local(a.output)]
lines += ["file " + local(value) for value in a.objects]
lines += ["libpath R:" + chr(92) + "toolchains" + chr(92) + "watcom10.0a" + chr(92) + "lib386",
          "libpath R:" + chr(92) + "toolchains" + chr(92) + "watcom10.0a" + chr(92) + "lib386" + chr(92) + "nt",
          "library clib3r", "library nt"]
a.response.write_text(chr(10).join(lines) + chr(10))
