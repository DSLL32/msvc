#!/usr/bin/env python3
"""Validate source provenance and inventory without running Wine or target code."""
import hashlib
import json
from pathlib import Path
from pe_inventory import inventory

BINARY = Path(__file__).resolve().parents[1]
plan = json.loads((BINARY / "cases/plan.json").read_text())
assert plan["state"] in ["first-case-released","first-case-complete-paused"]
assert plan["reconstruction_released"] is True
assert plan["first_case_limit"] == 1
assert inventory() == json.loads((BINARY / "cases/pe-inventory.json").read_text())
for seed in plan["source_seeds"]:
    path = BINARY / seed["destination"]
    assert hashlib.sha256(path.read_bytes()).hexdigest() == seed["sha256"], str(path)
print("RCDLL inventory and %d input-only source seeds verified; only the first help case is released." % len(plan["source_seeds"]))
