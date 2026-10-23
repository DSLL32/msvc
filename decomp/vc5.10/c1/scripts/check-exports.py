#!/usr/bin/env python3
"""Separate DLL catalog/ABI checks; common graph discovery stays in Rust."""
import json
from pathlib import Path
import re
import subprocess

from groundwork import inspect, ROOT

PROJECT = Path(__file__).resolve().parents[1]
original = inspect(ROOT / "toolchains/5.10/bin/C1.DLL")
rebuilt = inspect(PROJECT / "build/c1.dll")
catalog = lambda image: [(item["name"], item["ordinal"], item["forwarder"])
                          for item in image["exports"]]
assert catalog(original) == catalog(rebuilt)
assert original["export_module_name"] == rebuilt["export_module_name"] == "c1n32_l.dll"
for export in rebuilt["exports"]:
    cleanup = int(export["name"].split("@")[1])
    start = rebuilt["image_base"] + export["rva"]
    disassembly = subprocess.check_output(["objdump", "-d", "-Mintel",
        "--start-address=" + str(start), "--stop-address=" + str(start + 32),
        str(PROJECT / "build/c1.dll")], text=True)
    assert re.search(r"\bret\s+0x%x\b" % cleanup, disassembly)
    # Both exports are intentionally stubs during this first lifecycle case.
    assert "int3" in disassembly
print(json.dumps({"exports": catalog(rebuilt), "stub_cleanup_bytes": [4, 12],
                  "internal_module_name": rebuilt["export_module_name"]}))
