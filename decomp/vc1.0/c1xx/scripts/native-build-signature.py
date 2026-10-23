"""Invalidate native objects when effective shared contents or build flags change."""
from pathlib import Path
import sys
from shared_build_inputs import inputs, signature

root, destination = Path(sys.argv[1]), Path(sys.argv[2])
content = signature(inputs(root))[0] + "\n" + sys.argv[3] + "\n"
if not destination.exists() or destination.read_text() != content:
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(content)
