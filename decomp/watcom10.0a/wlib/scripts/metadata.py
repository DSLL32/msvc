#!/usr/bin/env python3
"""Read-only, hash-bound PE metadata. Never executes the original program."""

import argparse
import hashlib
import json
import pathlib
import struct


EXPECTED_SHA256 = "f20631839a9c1c38969f0c5930e26010361c6175790992181455b5c0ffb1cbb3"
ROOT = pathlib.Path(__file__).resolve().parents[4]
ORIGINAL = ROOT / "toolchains/watcom10.0a/binnt/WLIB.EXE"


def inspect(path):
    data = path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != EXPECTED_SHA256:
        raise ValueError("WLIB original hash differs; review metadata before proceeding")
    if data[:2] != b"MZ":
        raise ValueError("missing DOS signature")
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("missing PE signature")
    machine, count, timestamp, _, symbols, optional_size, _ = struct.unpack_from(
        "<HHIIIHH", data, pe + 4)
    opt = pe + 24
    if machine != 0x14c or struct.unpack_from("<H", data, opt)[0] != 0x10b:
        raise ValueError("expected i386 PE32")
    entry = struct.unpack_from("<I", data, opt + 16)[0]
    base = struct.unpack_from("<I", data, opt + 28)[0]
    sections = []
    for i in range(count):
        header = opt + optional_size + i * 40
        name = data[header:header + 8].rstrip(b"\0").decode("ascii")
        virtual_size, rva, raw_size, raw_offset = struct.unpack_from("<IIII", data, header + 8)
        flags = struct.unpack_from("<I", data, header + 36)[0]
        sections.append(dict(name=name, rva=rva, virtual_size=virtual_size,
                             raw_size=raw_size, raw_offset=raw_offset,
                             executable=bool(flags & 0x20000000)))

    def offset(rva):
        for section in sections:
            delta = rva - section["rva"]
            if 0 <= delta < section["raw_size"]:
                return section["raw_offset"] + delta
        raise ValueError("RVA not backed by file bytes: %#x" % rva)

    def string(rva):
        start = offset(rva)
        return data[start:data.index(b"\0", start)].decode("ascii")

    imports = []
    import_rva, _ = struct.unpack_from("<II", data, opt + 104)
    descriptor = offset(import_rva)
    while any(data[descriptor:descriptor + 20]):
        lookup, _, _, name, first = struct.unpack_from("<IIIII", data, descriptor)
        thunk = offset(lookup or first)
        names = []
        while True:
            value = struct.unpack_from("<I", data, thunk)[0]
            if not value:
                break
            names.append({"ordinal": value & 0xffff} if value & 0x80000000
                         else {"name": string(value + 2)})
            thunk += 4
        imports.append(dict(dll=string(name), symbols=names))
        descriptor += 20
    section_end = max(s["raw_offset"] + s["raw_size"] for s in sections)
    return dict(original=str(path.relative_to(ROOT)), sha256=digest,
                size=len(data), machine="i386", image_base=base, entry_rva=entry,
                timestamp=timestamp, coff_symbol_count=symbols,
                directories={name: dict(zip(("rva", "size"), struct.unpack_from("<II", data, opt + 96 + index * 8)))
                             for name, index in (("export", 0), ("import", 1), ("resource", 2), ("debug", 6))},
                sections=sections, imports=imports,
                overlay=dict(offset=section_end, size=len(data) - section_end),
                accepted_application_functions=2, accepted_application_bytes=143)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", type=pathlib.Path, help="compare against saved metadata")
    args = parser.parse_args()
    metadata = inspect(ORIGINAL)
    if args.check:
        if metadata != json.loads(args.check.read_text()):
            raise SystemExit("metadata differs from saved manifest")
        print("WLIB metadata matches the original; no executable was run")
    else:
        print(json.dumps(metadata, indent=2))


if __name__ == "__main__":
    main()
