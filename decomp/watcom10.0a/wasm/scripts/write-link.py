#!/usr/bin/env python3
"""Write the period linker's response file; execution uses the shared launcher."""
import argparse
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("response")
p.add_argument("output")
p.add_argument("map")
p.add_argument("objects", nargs="+")
a = p.parse_args()
lines = ["format windows nt", "runtime console", "debug all",
         "option map=" + a.map.replace("/", "\\"),
         "name " + a.output.replace("/", "\\")]
lines += ["file " + obj.replace("/", "\\") for obj in a.objects]
lines += ["libpath ..\\..\\..\\toolchains\\watcom10.0a\\lib386",
          "libpath ..\\..\\..\\toolchains\\watcom10.0a\\lib386\\NT",
          "library clib3r", "library nt"]
Path(a.response).write_text("\n".join(lines) + "\n")
