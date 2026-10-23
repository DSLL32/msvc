#!/usr/bin/env python3
"""Read original PE metadata; no execution, masking or reconstruction."""
import argparse
import hashlib
import json
from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parents[4]
TARGET = ROOT / "toolchains/5.10/bin/C1.DLL"


def inspect(path):
    data = path.read_bytes()

    def u16(offset):
        return struct.unpack_from("<H", data, offset)[0]

    def u32(offset):
        return struct.unpack_from("<I", data, offset)[0]

    pe = u32(0x3c)
    if data[:2] != b"MZ" or data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("not a PE image")
    coff = pe + 4
    optional = coff + 20
    if u16(optional) != 0x10b:
        raise ValueError("expected PE32")
    table = optional + u16(coff + 16)
    sections = []
    for index in range(u16(coff + 2)):
        offset = table + index * 40
        sections.append({
            "name": data[offset:offset + 8].rstrip(b"\0").decode("ascii"),
            "rva": u32(offset + 12), "virtual_size": u32(offset + 8),
            "raw_size": u32(offset + 16), "raw_offset": u32(offset + 20),
            "characteristics": u32(offset + 36),
        })

    def raw(rva):
        if rva < u32(optional + 60):
            return rva
        for section in sections:
            delta = rva - section["rva"]
            if 0 <= delta < section["raw_size"]:
                return section["raw_offset"] + delta
        raise ValueError("RVA has no file bytes: %#x" % rva)

    def string(rva):
        offset = raw(rva)
        return data[offset:data.index(b"\0", offset)].decode("ascii")

    directories = [
        {"rva": u32(optional + 96 + i * 8),
         "size": u32(optional + 100 + i * 8)}
        for i in range(u32(optional + 92))
    ]
    imports = []
    if directories[1]["rva"]:
        descriptor = raw(directories[1]["rva"])
        while any(data[descriptor:descriptor + 20]):
            original, _, _, name, iat = struct.unpack_from("<5I", data, descriptor)
            symbols = []
            thunk = raw(original or iat)
            while u32(thunk):
                value = u32(thunk)
                symbols.append({"ordinal": value & 0xffff} if value & 0x80000000
                               else {"name": string(value + 2)})
                thunk += 4
            imports.append({"module": string(name), "iat_rva": iat,
                            "symbols": symbols})
            descriptor += 20
    exports = []
    export_name = None
    if directories[0]["rva"]:
        offset = raw(directories[0]["rva"])
        export_name = string(u32(offset + 12))
        base, count, names, addresses, name_table, ordinal_table = struct.unpack_from(
            "<6I", data, offset + 16)
        by_index = {u16(raw(ordinal_table) + i * 2): string(u32(raw(name_table) + i * 4))
                    for i in range(names)}
        for i in range(count):
            rva = u32(raw(addresses) + i * 4)
            if rva:
                forwarded = directories[0]["rva"] <= rva < (
                    directories[0]["rva"] + directories[0]["size"])
                exports.append({"ordinal": base + i, "name": by_index.get(i),
                                "rva": rva,
                                "forwarder": string(rva) if forwarded else None})
    debug = []
    if directories[6]["rva"]:
        for i in range(directories[6]["size"] // 28):
            offset = raw(directories[6]["rva"]) + i * 28
            kind, size, _, pointer = struct.unpack_from("<4I", data, offset + 12)
            entry = {"type": kind, "size": size}
            if kind == 2 and size >= 4:
                payload = data[pointer:pointer + size]
                entry["signature"] = payload[:4].decode("ascii", errors="replace")
                if payload[:4] == b"NB10" and len(payload) >= 16:
                    entry["pdb_path"] = payload[16:].split(b"\0", 1)[0].decode("ascii")
            debug.append(entry)
    resource_groups = []
    if directories[2]["rva"]:
        resource_base = raw(directories[2]["rva"])

        def entries(relative):
            offset = resource_base + relative
            count = u16(offset + 12) + u16(offset + 14)
            for i in range(count):
                name, child = struct.unpack_from("<2I", data, offset + 16 + i * 8)
                if name & 0x80000000:
                    at = resource_base + (name & 0x7fffffff)
                    length = u16(at)
                    name = data[at + 2:at + 2 + length * 2].decode("utf-16le")
                yield name, child & 0x7fffffff, bool(child & 0x80000000)

        for kind, child, directory in entries(0):
            if not directory:
                raise ValueError("unexpected resource root leaf")
            items = []
            for identifier, level, directory in entries(child):
                if not directory:
                    raise ValueError("unexpected resource language parent leaf")
                for language, leaf, directory in entries(level):
                    if directory:
                        raise ValueError("unexpected resource fourth directory level")
                    payload_rva, size, codepage, _ = struct.unpack_from(
                        "<4I", data, resource_base + leaf)
                    items.append({"identifier": identifier, "language": language,
                                  "size": size, "codepage": codepage})
            resource_groups.append({"type": kind, "items": items})
    return {
        "target": str(path.relative_to(ROOT)),
        "sha256": hashlib.sha256(data).hexdigest(), "file_size": len(data),
        "machine": u16(coff), "coff_timestamp": u32(coff + 4),
        "characteristics": u16(coff + 18),
        "image_base": u32(optional + 28), "entry_rva": u32(optional + 16),
        "size_of_image": u32(optional + 56), "sections": sections,
        "export_module_name": export_name, "exports": exports, "imports": imports,
        "tls_directory": directories[9], "debug": debug,
        "resource_groups": resource_groups,
        "local_import_files": {
            item["module"]: next((str(candidate.relative_to(ROOT))
                                  for candidate in path.parent.iterdir()
                                  if candidate.name.lower() == item["module"].lower()), None)
            for item in imports
        },
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", type=Path,
                        help="compare to checked-in metadata, without modifying it")
    args = parser.parse_args()
    result = inspect(TARGET)
    if args.check:
        if json.loads(args.check.read_text()) != result:
            parser.exit(1, "original PE inventory differs\n")
        print("C1.DLL metadata matches frozen groundwork inventory")
    else:
        print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
