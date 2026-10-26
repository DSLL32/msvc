#!/usr/bin/env python3
"""Keep canonical CRT extents; omit one unselected shared-body audit row.

The exact shipped i4fd386 member contains both public entries. I4FD owns
its full CFG, including the suffix also entered by U4FD. App E8 calls retain
both canonical endpoint identities; this view makes no CRT BODY comparison.
"""
import json
import sys
from pathlib import Path
rows = json.loads(Path(sys.argv[1]).read_text())
selected = [row for row in rows if row["name"] == "__U4FD"]
if selected:
    assert len(selected) == 1
    row = selected[0]
    assert row["category"] == "runtime"
    assert row["start"] == 0x6bd01 and row["end"] == 0x6bd34
    assert not row["code_chunks"] and not row["shared"]
Path(sys.argv[2]).write_text(json.dumps([row for row in rows if row["name"] != "__U4FD"], indent=2) + "\n")
