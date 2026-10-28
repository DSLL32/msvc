#!/usr/bin/env python3
"""Read-only runtime format controls. All executions are the native analyzer.
Optional existing period-build inputs are inspected, never built or launched.
"""
import argparse
import hashlib
import json
import pathlib
import shutil
import struct
import subprocess
import tempfile
import time

p = argparse.ArgumentParser()
p.add_argument("tool", type=pathlib.Path)
p.add_argument("repo", type=pathlib.Path)
p.add_argument("--rebuilt-image", type=pathlib.Path)
p.add_argument("--rebuilt-map", type=pathlib.Path)
p.add_argument("--compiled-object", type=pathlib.Path)
a = p.parse_args()
a.tool, a.repo = a.tool.resolve(), a.repo.resolve()
scratch = pathlib.Path(tempfile.mkdtemp(prefix="borland-runtime-", dir=pathlib.Path.home() / ".ai-tmp"))
lib = a.repo / "toolchains/bc4.0/BC4/LIB"
original = a.repo / "toolchains/bc4.0/BC4/BIN/BCC32.EXE"
assert hashlib.sha256(original.read_bytes()).hexdigest() == "15342e1a2579244d839e0ed7a93f747ac3ea8721b779d105497fcae020dcadc5"
receipts = []


def identify(label, image, extra=(), good=True, runtime=True):
    out = scratch / (label + ".json")
    args = [str(a.tool), "identify-runtime", "bc4.0", str(image), "--repo", str(a.repo), "--output", str(out)]
    if runtime:
        args += ["--library", str(lib / "CW32.LIB"), "--object", str(lib / "C0X32.OBJ")]
    args += list(map(str, extra))
    start = time.monotonic()
    r = subprocess.run(args, capture_output=True)
    elapsed = 1000 * (time.monotonic() - start)
    (scratch / (label + ".log")).write_bytes(r.stdout + r.stderr)
    assert (r.returncode == 0) == good, (label, r.returncode, r.stderr.decode())
    receipts.append({"name": label, "elapsed_ms": elapsed, "status": r.returncode})
    return json.loads(out.read_text()) if good else None


rows = identify("bcc", original)
assert any((r["name"], r["start"], r["end"]) == ("__acrtused", 0x20000, 0x2004a) for r in rows)
assert any((r["name"], r["start"], r["end"]) == ("__GetExceptDLLinfo", 0x2004a, 0x2004f) for r in rows)
assert all(r["start"] < r["end"] for r in rows)
# Naming is irrelevant; PE characteristics select initialized code sections.
b = bytearray(original.read_bytes())
pe = struct.unpack_from("<I", b, 0x3c)[0]
table = pe + 24 + struct.unpack_from("<H", b, pe + 20)[0]
assert b[table:table + 8] == b"CODE\0\0\0\0"
b[table:table + 8] = b"OTHER\0\0\0"
renamed = scratch / "renamed.exe"
renamed.write_bytes(b)
assert identify("renamed-code", renamed) == rows
# A real owned byte difference removes the startup body; operands aren't a
# license to ignore surrounding instructions.
raw = struct.unpack_from("<I", b, table + 20)[0]
b[raw] ^= 1
mutated = scratch / "changed-code.exe"
mutated.write_bytes(b)
assert not any(r["name"] == "__acrtused" and r["start"] == 0x20000 for r in identify("changed-code", mutated))
b[table + 20:table + 24] = struct.pack("<I", len(b) + 1)
bad = scratch / "bad-bounds.exe"
bad.write_bytes(b)
identify("bad-bounds", bad, good=False)

if a.rebuilt_image or a.rebuilt_map or a.compiled_object:
    assert a.rebuilt_image and a.rebuilt_map and a.compiled_object
    for path in (a.rebuilt_image, a.rebuilt_map, a.compiled_object):
        shutil.copy2(path, scratch / path.name)
    rows = identify("period-runtime-map", a.rebuilt_image, ["--map", a.rebuilt_map])
    assert any(r["name"] == "__acrtused" for r in rows)
    # This is an authored scaffold object test, never a runtime classification
    # of BCC's application main or application reconstruction acceptance.
    rows = identify("period-object-map", a.rebuilt_image, ["--map", a.rebuilt_map, "--object", a.compiled_object], runtime=False)
    assert rows, "compiled OMF object must supply a real matched public"

(scratch / "receipt.json").write_text(json.dumps(receipts, indent=2) + "\n")
print(f"Borland runtime controls PASS: {len(receipts)} analyzer invocations; no Wine/target launches; {scratch}")
