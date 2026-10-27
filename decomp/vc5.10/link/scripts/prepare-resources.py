#!/usr/bin/env python3
"""Encode LINK's encountered STRINGTABLE entries for native startup only."""
import importlib.util
import re
from pathlib import Path

project = Path(__file__).resolve().parents[1]
repository = project.parents[2]
source = project / "src/messages.rc"
lines = source.read_text(encoding="ascii").splitlines()
if lines[:3] != ["LANGUAGE 9, 1", "STRINGTABLE", "BEGIN"] or lines[-1] != "END":
    raise ValueError("unsupported resource language/table scope")
entries = []
inside = False
for line in lines:
    if line.strip() == "BEGIN":
        inside = True
        continue
    if line.strip() == "END":
        inside = False
        continue
    if not inside or not line.strip():
        continue
    match = re.fullmatch(r'\s*(\d+)\s+"((?:[^"]|"")*)"\s*', line)
    if not match or "\\" in match[2]:
        raise ValueError("unsupported STRINGTABLE source spelling")
    entries.append(dict(kind="string", id=int(match[1]), language=1033,
                        text=match[2].replace('""', '"')))
if not entries or inside:
    raise ValueError("incomplete encountered STRINGTABLE")
spec = importlib.util.spec_from_file_location(
    "resources", repository / "lib/recon/resources.py")
resources = importlib.util.module_from_spec(spec)
spec.loader.exec_module(resources)
destination = project / ".build/native-messages.rmr"
destination.parent.mkdir(parents=True, exist_ok=True)
resources.write_resources(destination,
    "04c892989f4cc8076ef45e033215eb7aefbdf1459cbee9b13a81f8ddb718b579",
    entries)
