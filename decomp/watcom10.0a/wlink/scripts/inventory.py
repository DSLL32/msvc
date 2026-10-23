#!/usr/bin/env python3
"""Read-only inventory of the exact pristine and GS-patched WLINK images."""
import hashlib
import json
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
TOOLCHAIN = ROOT / "toolchains/watcom10.0a"
ORIGINAL_HASH = "ae8bb02cce0e92ed1a0c5c003a7312bb05f8bf3c252e1efa27a4835fb83b9ff1"
PATCHED_HASH = "509c749b2258029a25a0071e34fa13b76a2fd4b267591a2bb12e9a7be5fd665b"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def require(condition, message):
    if not condition:
        raise SystemExit(message)


def inventory():
    original = TOOLCHAIN / "binnt/WLINK.EXE"
    patched = TOOLCHAIN / ".patched/WLINK.EXE"
    data = original.read_bytes()
    capture = patched.read_bytes()
    require(digest(data) == ORIGINAL_HASH, "Unexpected pristine WLINK identity")
    require(digest(capture) == PATCHED_HASH, "Unexpected patched WLINK identity")
    patch_path = TOOLCHAIN / ".patched/WLINK-GS-patch.json"
    patch = json.loads(patch_path.read_text())
    expected = bytearray(data)
    substitutions = []
    for row in patch["patches"]:
        offset = int(row["file_offset"], 0)
        before, after = bytes.fromhex(row["before"]), bytes.fromhex(row["after"])
        require(len(before) == len(after), "Patch changes image layout")
        require(data[offset:offset + len(before)] == before, "Patch preimage mismatch")
        expected[offset:offset + len(before)] = after
        substitutions.append(dict(row))
    require(bytes(expected) == capture, "Undocumented patched-image changes")
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    require(data[pe:pe + 4] == b"PE\0\0", "Not PE")
    machine, count, timestamp, _, _, optional_size, _ = struct.unpack_from("<HHIIIHH", data, pe + 4)
    optional = pe + 24
    require(machine == 0x14c and struct.unpack_from("<H", data, optional)[0] == 0x10b, "Not i386 PE32")
    entry, base = struct.unpack_from("<I", data, optional + 16)[0], struct.unpack_from("<I", data, optional + 28)[0]
    sections = []
    for index in range(count):
        record = optional + optional_size + index * 40
        name = data[record:record + 8].split(b"\0", 1)[0].decode("ascii")
        virtual_size, rva, raw_size, offset = struct.unpack_from("<IIII", data, record + 8)
        sections.append(dict(name=name, rva=rva, virtual_size=virtual_size, raw_offset=offset, raw_size=raw_size))

    def file_offset(rva):
        for section in sections:
            if section["rva"] <= rva < section["rva"] + section["raw_size"]:
                return section["raw_offset"] + rva - section["rva"]
        raise SystemExit("Unmapped RVA")

    def string(rva):
        offset = file_offset(rva)
        return data[offset:data.index(b"\0", offset)].decode("ascii")

    import_rva = struct.unpack_from("<I", data, optional + 104)[0]
    imports = []
    descriptor = file_offset(import_rva)
    while any(data[descriptor:descriptor + 20]):
        lookup, _, _, name, first = struct.unpack_from("<IIIII", data, descriptor)
        symbols = []
        index = 0
        while True:
            item = struct.unpack_from("<I", data, file_offset((lookup or first) + index * 4))[0]
            if item == 0:
                break
            symbols.append({"iat_rva": first + index * 4, "symbol": "#%d" % (item & 0xffff) if item & 0x80000000 else string(item + 2)})
            index += 1
        imports.append({"dll": string(name), "symbols": symbols})
        descriptor += 20
    overlay = max(row["raw_offset"] + row["raw_size"] for row in sections)
    return {
        "original": str(original.relative_to(ROOT)), "original_sha256": digest(data),
        "capture_image": str(patched.relative_to(ROOT)), "capture_sha256": digest(capture),
        "bytes": len(data), "image_base": base, "entry_rva": entry, "timestamp": timestamp,
        "sections": sections, "imports": imports, "resource_directory_rva": struct.unpack_from("<I", data, optional + 112)[0],
        "overlay": {"offset": overlay, "bytes": len(data) - overlay, "sha256": digest(data[overlay:])},
        "gs_substitutions": substitutions,
        "runtime_validation": "not performed by this inventory",
        "accepted_application_functions": 0
    }


if __name__ == "__main__":
    print(json.dumps(inventory(), indent=2))
