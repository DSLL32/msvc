#!/usr/bin/env python3
"""Select the WCC App audit's external CRT body catalog.

The exact i4fd386 member has signed and unsigned entry points sharing its
conversion tail. Both canonical CRT extents remain in binary.c for restoration
and endpoint identity. App callers need those identities, not a second decoded
copy of the overlapping unsigned support body. The fdu4386 signed/unsigned pair is treated the same way. The finite ftos_digits body likewise shares its real POP/RET tail with
ftos_exponent; canonical restoration extents remain complete. All other Records pass intact.
"""
import json
import sys
from pathlib import Path

source, destination = map(Path, sys.argv[1:])
rows = json.loads(source.read_text())
assert sum(row['name'] == '__I4FD' for row in rows) == 1
assert sum(row['name'] == '__U4FD' for row in rows) == 1
assert sum(row['name'] == '__FDI4' for row in rows) == 1
assert sum(row['name'] == '__FDU4' for row in rows) == 1
selected = [row for row in rows if row['name'] not in ('__U4FD', '__FDU4', 'ftos_digits')]
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_text(json.dumps(selected, indent=2) + '\n')
