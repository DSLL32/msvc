#!/usr/bin/env python3
"""Bind frozen authored debugger probe images/configs. No executable is run."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class PE:
    def __init__(self, path):
        self.data = path.read_bytes()
        pe = self.u32(0x3c)
        assert self.data[pe:pe + 4] == b"PE\0\0"
        machine, count, _, _, _, optional, _ = struct.unpack_from("<HHIIIHH", self.data, pe + 4)
        assert machine == 0x14c and self.u16(pe + 24) == 0x10b
        self.base, self.entry, self.size = self.u32(pe + 52), self.u32(pe + 40), self.u32(pe + 80)
        self.directories = [self.u32(pe + 24 + 96 + i * 8) for i in range(16)]
        self.sections = []
        for i in range(count):
            at = pe + 24 + optional + 40 * i
            self.sections.append({"rva": self.u32(at + 12), "size": self.u32(at + 16), "raw": self.u32(at + 20), "flags": self.u32(at + 36)})

    def u32(self, at):
        return struct.unpack_from("<I", self.data, at)[0]

    def u16(self, at):
        return struct.unpack_from("<H", self.data, at)[0]

    def offset(self, rva):
        for row in self.sections:
            if row["rva"] <= rva < row["rva"] + row["size"]:
                return row["raw"] + rva - row["rva"]
        raise ValueError("unmapped RVA")

    def text(self, rva):
        at = self.offset(rva)
        return self.data[at:self.data.index(b"\0", at)].decode("ascii")

    def imports(self):
        at = self.offset(self.directories[1])
        result = []
        while any(self.data[at:at + 20]):
            lookup, _, _, name, first = struct.unpack_from("<IIIII", self.data, at)
            i = 0
            while True:
                item = self.u32(self.offset((lookup or first) + i * 4))
                if not item:
                    break
                if item & 0x80000000:
                    raise ValueError("unexpected authored ordinal import")
                result.append({"dll": self.text(name), "name": self.text(item + 2), "iat": first + 4 * i})
                i += 1
            at += 20
        return result

    def exports(self):
        at = self.offset(self.directories[0])
        result = {}
        count, functions, names, ordinals = (self.u32(at + i) for i in (24, 28, 32, 36))
        for i in range(count):
            name = self.text(self.u32(self.offset(names + i * 4)))
            ordinal = self.u16(self.offset(ordinals + i * 2))
            result[name] = self.u32(self.offset(functions + ordinal * 4))
        return result

    def thunk(self, iat):
        needle = b"\xff\x25" + struct.pack("<I", self.base + iat)
        found = []
        for row in self.sections:
            if row["flags"] & 0x20000000:
                data = self.data[row["raw"]:row["raw"] + row["size"]]
                found.extend(row["rva"] + i for i in range(len(data) - 5) if data[i:i + 6] == needle)
        if len(found) != 1:
            raise ValueError("authored import lacks one canonical thunk")
        return found[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    build, output = args.build.resolve(), args.output.resolve()
    receipt = json.loads((build / "receipt.json").read_text())
    for name, expected in receipt["outputs"].items():
        if sha(build / name) != expected:
            raise ValueError("frozen output mismatch")
    for name, expected in receipt["sources"].items():
        if sha(build / name) != expected:
            raise ValueError("frozen source mismatch")
    child, dll = PE(build / "observer-probe.exe"), PE(build / "observer-probe.dll")
    map_text = (build / "observer-probe.map").read_text()
    main_va = int(re.search(r"^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+_main\s+([0-9a-fA-F]{8})\b", map_text, re.M)[1], 16)
    main_rva = main_va - child.base
    rows = ["image %x %x %x %x %x %x 2" % (child.base, child.size, child.entry, main_rva,
            child.data[child.offset(child.entry)], child.data[child.offset(main_rva)])]
    counts = {"GetCurrentProcessId": 0, "GetCommandLineA": 0, "GetStdHandle": 1, "GetFileType": 1,
              "GetConsoleMode": 2, "SetConsoleMode": 2, "CreateFileA": 7, "ReadFile": 5,
              "CloseHandle": 1, "WriteFile": 5, "ExitProcess": 1, "_ProbeValue@8": 2, "_ProbeBadReturn@8": 2}
    selected, excluded = [], []
    for item in child.imports():
        if item["name"] not in counts:
            excluded.append(item)
            continue
        item["rva"] = child.thunk(item["iat"])
        selected.append(item)
        rows.append("thunk %x %x %u %u %s" % (item["rva"], item["iat"], counts[item["name"]], item["name"] == "ExitProcess", item["name"]))
    if {row["name"] for row in selected} != set(counts):
        raise ValueError("authored selected API set mismatch")
    for scope, pe, names in (("image", child, ("fixture_stage", "fixture_callback_entries", "fixture_thread_entries")),
                             ("dll", dll, ("fixture_dll_entries", "fixture_bad_return_entries"))):
        exports = pe.exports()
        for name in names:
            rows.append("marker %s %x %s" % (scope, exports[name], name))
    alternative = next(row["iat"] for row in selected if row["name"] == "GetCurrentProcessId")
    output.mkdir(parents=True, exist_ok=False)
    (output / "sites.txt").write_text("\n".join(rows) + "\n")
    for fault in (1, 2):
        (output / ("fault-%u.txt" % fault)).write_text("\n".join(rows + ["fault %u %x _ProbeValue@8" % (fault, alternative)]) + "\n")
    manifest = {"kind": "authored-only probe configuration; no runtime execution", "build": str(build), "build_receipt_sha256": sha(build / "receipt.json"),
                "images": receipt["outputs"], "selected_thunks": selected, "uninstrumented_authored_imports": excluded,
                "configuration": {path.name: sha(path) for path in output.glob("*.txt")},
                "limitation": "selected authored call sites only; not an application graph or complete CRT/API coverage proof"}
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({"output": str(output), "thunks": len(selected), "configs": manifest["configuration"]}, indent=2))


if __name__ == "__main__":
    main()
