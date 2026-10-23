#!/usr/bin/env python3
"""Read-only PE groundwork inventory; not graph discovery or acceptance."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[4]
TARGET = ROOT / "toolchains/5.10/bin/CVPACK.EXE"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect():
    data = TARGET.read_bytes()
    u16 = lambda offset: struct.unpack_from("<H", data, offset)[0]
    u32 = lambda offset: struct.unpack_from("<I", data, offset)[0]
    pe = u32(0x3c)
    assert data[:2] == b"MZ" and data[pe:pe + 4] == b"PE\0\0"
    coff = pe + 4
    opt = coff + 20
    assert u16(opt) == 0x10b, "Expected PE32"
    table = opt + u16(coff + 16)
    sections = []
    for index in range(u16(coff + 2)):
        offset = table + index * 40
        sections.append({
            "name": data[offset:offset + 8].split(b"\0")[0].decode("ascii"),
            "virtual_size": u32(offset + 8), "rva": u32(offset + 12),
            "raw_size": u32(offset + 16), "raw_offset": u32(offset + 20),
            "characteristics": hex(u32(offset + 36)),
        })

    def file_offset(rva):
        for section in sections:
            delta = rva - section["rva"]
            if 0 <= delta < section["raw_size"]:
                return section["raw_offset"] + delta
        raise ValueError(f"RVA 0x{rva:x} is not file-backed")

    def cstring(rva):
        offset = file_offset(rva)
        return data[offset:data.index(b"\0", offset)].decode("ascii")

    directories = [
        {"rva": u32(opt + 96 + i * 8), "size": u32(opt + 100 + i * 8)}
        for i in range(u32(opt + 92))
    ]
    imports = []
    if directories[1]["rva"]:
        offset = file_offset(directories[1]["rva"])
        while any(data[offset:offset + 20]):
            original_thunk, _, _, name_rva, first_thunk = struct.unpack_from("<5I", data, offset)
            name = cstring(name_rva)
            thunk_offset = file_offset(original_thunk or first_thunk)
            symbols = []
            while u32(thunk_offset):
                thunk = u32(thunk_offset)
                symbols.append({"ordinal": thunk & 0xffff} if thunk & 0x80000000
                               else {"name": cstring(thunk + 2)})
                thunk_offset += 4
            local = next((path for path in TARGET.parent.iterdir()
                          if path.name.lower() == name.lower()), None)
            imports.append({"dll": name, "symbols": symbols,
                            "shipped_path": str(local.relative_to(ROOT)) if local else None,
                            "shipped_sha256": digest(local) if local else None})
            offset += 20
    marker = data.find(struct.pack("<I", 0xfeef04bd))
    version = None
    if marker >= 0:
        ms, ls = struct.unpack_from("<II", data, marker + 8)
        version = [ms >> 16, ms & 0xffff, ls >> 16, ls & 0xffff]
    return {
        "schema": 1, "stage": "groundwork-only", "target": str(TARGET.relative_to(ROOT)),
        "sha256": digest(TARGET), "file_size": len(data), "machine": hex(u16(coff)),
        "coff_timestamp": u32(coff + 4), "characteristics": hex(u16(coff + 18)),
        "image_base": hex(u32(opt + 28)), "entry_rva": hex(u32(opt + 16)),
        "entry_va": hex(u32(opt + 28) + u32(opt + 16)), "subsystem": u16(opt + 68),
        "resource_file_version": version, "sections": sections, "imports": imports,
        "export_directory": directories[0], "relocation_directory": directories[5],
        "debug_directory": directories[6], "tls_directory": directories[9],
        "compiler_inputs": [{"path": str(path.relative_to(ROOT)), "sha256": digest(path)}
                            for path in (ROOT / "toolchains/5.10/bin/CL.EXE",
                                         ROOT / "toolchains/5.10/bin/C1.DLL",
                                         ROOT / "toolchains/5.10/bin/C2.EXE",
                                         ROOT / "toolchains/5.10/bin/LINK.EXE")],
        "accepted_functions": 0, "reconstruction_started": False,
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    rendered = json.dumps(inspect(), indent=2) + "\n"
    if args.output:
        args.output.write_text(rendered)
    else:
        print(rendered, end="")
