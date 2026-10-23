#!/usr/bin/env python3
"""Read-only identity/CF inspection; never launches a target executable."""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[4]
TOOLCHAIN = ROOT / "toolchains/watcom10.0a"
IDENTITIES = {
    "binb/WPP386.EXE": "647f4e754cd27fc76b3c44a47227ae3baa4ff2e2f253a8ec27216414653b1e6f",
    "binnt/WPP386.EXE": "8fee7d6ab3f2c06a0a874135e3898386898679e7dd53993fcc508e296364767d",
}


def inspect():
    images = {}
    for name, expected in IDENTITIES.items():
        data = (TOOLCHAIN / name).read_bytes()
        actual = hashlib.sha256(data).hexdigest()
        if actual != expected:
            raise ValueError(f"{name}: identity mismatch: {actual}")
        images[name] = data
    image = images["binb/WPP386.EXE"]
    header = struct.unpack_from("<H", image, 8)[0] * 16
    signature, offset, size, reloc, memory, entry, stack = struct.unpack_from(
        "<7I", image, header
    )
    if signature != 0x4643 or offset + size > len(image):
        raise ValueError("unexpected CF signature or payload bounds")
    if not (entry < size and reloc < size <= memory):
        raise ValueError("invalid CF entry, relocation or allocation bounds")
    payload = image[offset:offset + size]
    cursor = reloc
    groups = []
    targets = []
    while True:
        count = struct.unpack_from("<H", payload, cursor)[0]
        cursor += 2
        if not count:
            break
        high, low = struct.unpack_from("<HH", payload, cursor)
        cursor += 4
        target = (high << 16) | low
        group = []
        for index in range(count):
            if index:
                target += struct.unpack_from("<H", payload, cursor)[0]
                cursor += 2
            if target + 4 > memory:
                raise ValueError("relocation write exceeds allocation")
            group.append(target)
        targets.extend(group)
        groups.append({"count": count, "first": hex(group[0]), "last": hex(group[-1])})
    if cursor != size or len(set(targets)) != len(targets):
        raise ValueError("unexpected relocation tail or duplicate relocation")
    return {
        "status": "static-groundwork-only",
        "sha256": IDENTITIES,
        "cf_header_file_offset": hex(header),
        "payload_file_offset": hex(offset),
        "payload_loaded_size": hex(size),
        "unloaded_file_tail_size": len(image) - offset - size,
        "relocation_payload_offset": hex(reloc),
        "allocation_size": hex(memory),
        "entry_payload_offset": hex(entry),
        "stack_size_field": hex(stack),
        "relocation_groups": groups,
        "relocation_count": len(targets),
        "accepted_functions": 0,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["status", "inspect", "blocked"])
    parser.add_argument("target", nargs="?")
    args = parser.parse_args()
    if args.action == "blocked":
        print(f"{args.target}: blocked pending reviewed CF transport/layout, startup provenance and controller negatives; scaffold builds verified", file=sys.stderr)
        return 2
    if args.action == "status":
        print("WPP386: scaffold only; 0 accepted functions; two unmodified help baselines captured")
        return 0
    print(json.dumps(inspect(), indent=2))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, struct.error) as error:
        print(f"inspection failed: {error}", file=sys.stderr)
        sys.exit(1)
