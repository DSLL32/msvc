#!/usr/bin/env python3
"""Read-only PE metadata inventory; this is not a graph or acceptance audit."""
import argparse
import hashlib
import json
from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parents[4]
TARGET = ROOT / "toolchains/5.10/bin/C1XX.DLL"


def inventory(path):
    data = path.read_bytes()
    u16 = lambda off: struct.unpack_from("<H", data, off)[0]
    u32 = lambda off: struct.unpack_from("<I", data, off)[0]
    pe = u32(0x3c)
    if data[:2] != b"MZ" or data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("not a PE image")
    coff, opt = pe + 4, pe + 24
    if u16(coff) != 0x14c or u16(opt) != 0x10b:
        raise ValueError("expected i386 PE32")
    base = u32(opt + 28)
    sections = []
    sec = opt + u16(coff + 16)
    for i in range(u16(coff + 2)):
        off = sec + i * 40
        sections.append({
            "name": data[off:off + 8].rstrip(b"\0").decode("ascii"),
            "virtual_size": u32(off + 8), "rva": u32(off + 12),
            "raw_size": u32(off + 16), "raw_offset": u32(off + 20),
            "characteristics": u32(off + 36),
        })

    def fileoff(rva):
        for section in sections:
            delta = rva - section["rva"]
            if 0 <= delta < section["raw_size"]:
                return section["raw_offset"] + delta
        raise ValueError("RVA has no file-backed bytes: %x" % rva)

    def string(rva):
        off = fileoff(rva)
        return data[off:data.index(b"\0", off)].decode("ascii")

    directories = [(u32(opt + 96 + i * 8), u32(opt + 100 + i * 8))
                   for i in range(16)]
    exports = []
    exp = fileoff(directories[0][0])
    ordinal_base = u32(exp + 16)
    functions = fileoff(u32(exp + 28))
    names = fileoff(u32(exp + 32))
    ordinals = fileoff(u32(exp + 36))
    for i in range(u32(exp + 24)):
        index = u16(ordinals + i * 2)
        exports.append({"name": string(u32(names + i * 4)),
                        "ordinal": ordinal_base + index,
                        "rva": u32(functions + index * 4)})
    imports = []
    desc = fileoff(directories[1][0]) if directories[1][0] else None
    while desc is not None and any(data[desc:desc + 20]):
        ilt, iat = u32(desc), u32(desc + 16)
        slots = []
        cursor = fileoff(ilt or iat)
        while u32(cursor):
            value = u32(cursor)
            item = {"iat_rva": iat + len(slots) * 4}
            if value & 0x80000000:
                item["ordinal"] = value & 0xffff
            else:
                item["name"] = string(value + 2)
            slots.append(item)
            cursor += 4
        imports.append({"dll": string(u32(desc + 12)), "symbols": slots})
        desc += 20
    return {
        "schema": 1, "target": str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path),
        "sha256": hashlib.sha256(data).hexdigest(), "size": len(data),
        "machine": "i386", "image_base": base,
        "entry_rva": u32(opt + 16), "image_size": u32(opt + 56),
        "characteristics": u16(coff + 18),
        "export_internal_name": string(u32(exp + 12)),
        "exports": exports, "imports": imports, "sections": sections,
        "tls_directory": {"rva": directories[9][0], "size": directories[9][1]},
        "relocation_directory": {"rva": directories[5][0], "size": directories[5][1]},
        "status": "groundwork-only; no function acceptance or executable fixtures",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", type=Path, help="compare against frozen metadata")
    args = parser.parse_args()
    result = inventory(TARGET)
    if args.check:
        expected = json.loads(args.check.read_text())
        if result != expected:
            raise SystemExit("PE inventory differs from frozen groundwork metadata")
        print("C1XX.DLL groundwork inventory matches")
    else:
        print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
