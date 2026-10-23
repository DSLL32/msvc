#!/usr/bin/env python3
"""Read-only PE inventory; this is not reconstruction acceptance or a CFG audit."""
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[4]
TARGET = ROOT / "toolchains/5.10/bin/RCDLL.DLL"


def inventory():
    data = TARGET.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    if data[:2] != b"MZ" or data[pe:pe + 4] != b"PE\0\0":
        raise SystemExit("not a PE image")
    machine, count, timestamp, _, _, optional_size, characteristics = struct.unpack_from("<HHIIIHH", data, pe + 4)
    optional = pe + 24
    if struct.unpack_from("<H", data, optional)[0] != 0x10b:
        raise SystemExit("expected PE32")
    entry = struct.unpack_from("<I", data, optional + 16)[0]
    base = struct.unpack_from("<I", data, optional + 28)[0]
    sections = []
    for n in range(count):
        offset = optional + optional_size + n * 40
        name, virtual_size, rva, raw_size, raw_offset = struct.unpack_from("<8sIIII", data, offset)
        sections.append(dict(name=name.rstrip(b"\0").decode("ascii"), rva=hex(rva), virtual_size=virtual_size, raw_size=raw_size, raw_offset=raw_offset, characteristics=hex(struct.unpack_from("<I", data, offset + 36)[0])))
    directories = []
    for n in range(16):
        rva, size = struct.unpack_from("<II", data, optional + 96 + n * 8)
        directories.append(dict(index=n, rva=hex(rva), size=size))
    def file_offset(rva):
        for section in sections:
            start = int(section["rva"], 16)
            if start <= rva < start + section["raw_size"]:
                return section["raw_offset"] + rva - start
        raise ValueError("RVA lacks file bytes: %x" % rva)

    def string(rva):
        offset = file_offset(rva)
        return data[offset:data.index(b"\0", offset)].decode("ascii")

    export_offset = file_offset(int(directories[0]["rva"], 16))
    ordinal_base, function_count, name_count, functions, names, ordinals = struct.unpack_from("<IIIIII", data, export_offset + 16)
    named = {}
    for n in range(name_count):
        index = struct.unpack_from("<H", data, file_offset(ordinals) + n * 2)[0]
        named[index] = string(struct.unpack_from("<I", data, file_offset(names) + n * 4)[0])
    exports = []
    for index in range(function_count):
        rva = struct.unpack_from("<I", data, file_offset(functions) + index * 4)[0]
        if rva:
            name = named.get(index)
            exports.append(dict(name=name, ordinal=ordinal_base + index, rva=hex(rva), analyzed_abi="stdcall", analyzed_stack_bytes={"Handler": 4, "RC": 24}.get(name)))
    report = subprocess.run(["objdump", "-p", str(TARGET)], check=True, capture_output=True, text=True).stdout
    import_text = report.split("The Import Tables", 1)[1].split("There is an export table", 1)[0]
    imports = []
    module = None
    for line in import_text.splitlines():
        match = re.search(r"DLL Name: (\S+)", line)
        if match:
            module = dict(dll=match.group(1), symbols=[])
            imports.append(module)
        elif module is not None:
            match = re.match(r"\s*[0-9a-f]+\s+<none>\s+[0-9a-f]+\s+(\S+)", line)
            if match:
                module["symbols"].append(match.group(1))
    return dict(schema=1, status="groundwork-only", target=str(TARGET.relative_to(ROOT)), sha256=hashlib.sha256(data).hexdigest(), bytes=len(data), machine=hex(machine), characteristics=hex(characteristics), timestamp=timestamp, image_base=hex(base), entry_rva=hex(entry), sections=sections, directories=directories, tls_present=bool(int(directories[9]["rva"], 16)), imports=imports, exports=exports, export_basis="objdump -p, checked against export entry and ret cleanup; signatures remain groundwork evidence")


if __name__ == "__main__":
    print(json.dumps(inventory(), indent=2))
