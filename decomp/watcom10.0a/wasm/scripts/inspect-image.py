#!/usr/bin/env python3
"""Read-only metadata for the WASM CF image; does not load or execute it."""

import hashlib
import json
from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parents[4]
TARGET = ROOT / "toolchains/watcom10.0a/binb/WASM.EXE"
STUB = ROOT / "toolchains/watcom10.0a/binnt/WASM.EXE"
EXPECTED = "a79e0dc11a26b098728aa988cfc974113e7ac5d28c81baded1f077ed27be0513"
EXPECTED_STUB = "8fee7d6ab3f2c06a0a874135e3898386898679e7dd53993fcc508e296364767d"


def main():
    raw = TARGET.read_bytes()
    stub = STUB.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    stub_digest = hashlib.sha256(stub).hexdigest()
    if digest != EXPECTED or stub_digest != EXPECTED_STUB:
        raise SystemExit("WASM original/stub identity changed; review required")
    if raw[:2] != b"MZ":
        raise SystemExit("missing MZ header")
    header = struct.unpack_from("<H", raw, 8)[0] * 16
    magic, offset, size, reloc, memory, entry = struct.unpack_from("<6I", raw, header)
    if magic != 0x4643 or offset + size > len(raw) or not entry < size <= memory:
        raise SystemExit("invalid CF header")
    image = raw[offset:offset + size]
    cursor = reloc
    sites = []
    groups = []
    while True:
        count = struct.unpack_from("<H", image, cursor)[0]
        cursor += 2
        if not count:
            break
        high, low = struct.unpack_from("<HH", image, cursor)
        cursor += 4
        site = (high << 16) | low
        group = []
        for index in range(count):
            if index:
                delta = struct.unpack_from("<H", image, cursor)[0]
                cursor += 2
                site += delta
            if site + 4 > size:
                raise SystemExit("relocation extends beyond initialized image")
            group.append(site)
        sites.extend(group)
        groups.append({"count": count, "first": group[0], "last": group[-1]})
    if cursor != size or len(sites) != len(set(sites)):
        raise SystemExit("unexpected relocation stream end or duplicate sites")
    print(json.dumps({
        "schema": 1,
        "target": str(TARGET.relative_to(ROOT)),
        "target_sha256": digest,
        "target_size": len(raw),
        "loader": str(STUB.relative_to(ROOT)),
        "loader_sha256": stub_digest,
        "loader_size": len(stub),
        "cf": {
            "header_file_offset": header,
            "image_file_offset": offset,
            "initialized_size": size,
            "allocation_size": memory,
            "entry_rva": entry,
            "relocation_stream_rva": reloc,
            "relocation_count": len(sites),
            "relocation_groups": groups,
            "relocation_stream_end_rva": cursor,
            "initialized_sha256": hashlib.sha256(image).hexdigest(),
        },
        "status": "read-only; no execution or accepted reconstructed functions",
    }, indent=2))


if __name__ == "__main__":
    main()
