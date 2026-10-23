#!/usr/bin/env python3
"""Validate source provenance and inventory without running Wine or target code."""
import hashlib
import json
from pathlib import Path
from pe_inventory import inventory

BINARY = Path(__file__).resolve().parents[1]
plan = json.loads((BINARY / "cases/plan.json").read_text())
assert plan["state"] == "groundwork-only"
assert plan["reconstruction_released"] is False
assert plan["accepted_functions"] == []
assert inventory() == json.loads((BINARY / "cases/pe-inventory.json").read_text())
for seed in plan["source_seeds"]:
    path = BINARY / seed["destination"]
    assert hashlib.sha256(path.read_bytes()).hexdigest() == seed["sha256"], str(path)
print("RCDLL groundwork inventory and %d input-only source seeds verified; reconstruction remains disabled." % len(plan["source_seeds"]))
