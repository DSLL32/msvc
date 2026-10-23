#!/usr/bin/env python3
"""Verify this target's originals and report CF metadata, without execution."""
import hashlib
import json
from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parents[4]
EXPECTED = {
    "binb/WCC386.EXE": "c3666de94f6fa6800f452dae8acf45505ecdb62f0ade2cc27cc86c2d9e8e2b6b",
    "binnt/WCC386.EXE": "8fee7d6ab3f2c06a0a874135e3898386898679e7dd53993fcc508e296364767d",
}


def inspect():
    originals = {}
    for relative, expected in EXPECTED.items():
        path = ROOT / "toolchains/watcom10.0a" / relative
        data = path.read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        if digest != expected:
            raise ValueError("original identity mismatch: " + relative)
        originals[relative] = {"sha256": digest, "size": len(data)}

    data = (ROOT / "toolchains/watcom10.0a/binb/WCC386.EXE").read_bytes()
    header = struct.unpack_from("<H", data, 8)[0] * 16
    magic, load, size, reloc, allocation, entry = struct.unpack_from("<6I", data, header)
    if magic != 0x4643 or load + size > len(data) or not entry < reloc < size <= allocation:
        raise ValueError("invalid CF header")
    cursor = load + reloc
    groups = 0
    fixups = []
    while True:
        count = struct.unpack_from("<H", data, cursor)[0]
        cursor += 2
        if count == 0:
            break
        groups += 1
        high, low = struct.unpack_from("<HH", data, cursor)
        cursor += 4
        address = (high << 16) | low
        for index in range(count):
            if index:
                address += struct.unpack_from("<H", data, cursor)[0]
                cursor += 2
            if address + 4 > reloc:
                raise ValueError("CF fixup outside pre-relocation image")
            fixups.append(address)
    if cursor != load + size or len(set(fixups)) != len(fixups):
        raise ValueError("unexpected relocation extent or duplicate fixups")
    return {
        "status": "read-only-scaffold-no-accepted-functions",
        "originals": originals,
        "cf": {
            "header_file_offset": header,
            "image_file_offset": load,
            "loaded_size": size,
            "allocation_size": allocation,
            "entry_offset": entry,
            "relocation_offset": reloc,
            "relocation_groups": groups,
            "relocation_count": len(fixups),
            "relocation_stream_end": cursor - load,
            "logical_identity": "actual runtime VA minus observed CF allocation base",
            "code_data_extents": "not established",
        },
    }


if __name__ == "__main__":
    print(json.dumps(inspect(), indent=2, sort_keys=True))
