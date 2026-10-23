#!/usr/bin/env python3
"""Bind pending callee names to real MAP publics; graph discovery stays in Rust."""
import json
from pathlib import Path
import re
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
metadata = root / 'cases/help/main-frontier'
pins = json.loads((metadata / 'main-original-pins.json').read_text())
publics = {}
for line in (root / 'build/wlib.map').read_text().splitlines():
    match = re.fullmatch(r'\s*0001:([0-9a-fA-F]{8})[+*]?\s+(\S+)\s*', line)
    if match:
        publics[match[2]] = 0x410000 + int(match[1], 16)
rebuilt = []
for pin in pins:
    address = publics[pin['name']]
    rebuilt.append(dict(name=pin['name'], start=address, end=address,
                        pending_extent=True))
(root / 'build/main-runtime-pins.json').write_text(json.dumps(rebuilt, indent=2) + '\n')
command = [sys.argv[1], '--repo', '../../..', '--project', '.', '--profile', 'cl',
           '--discovery', 'cfg', '--map-format', 'watcom10', '--config',
           'cases/help/main-frontier/audit-main-candidate.json', '--runtime',
           'cases/help/main-frontier/main-original-pins.json', '--rebuilt-runtime',
           'build/main-runtime-pins.json', '--rebuilt', 'build/WLIB.EXE', '--map',
           'build/wlib.map', '--check-ret', '--check-references', '--check-ownership',
           '--check-extents', '--report', 'build/main-static-graph.json']
raise SystemExit(subprocess.call(command, cwd=root))
