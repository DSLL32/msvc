#!/usr/bin/env python3
"""Authenticate exact six-byte import transfer thunks without running WLINK."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from inventory import TOOLCHAIN, inventory, require

ARGUMENTS = {
    "CloseHandle": 1, "CreateFileA": 7, "DeleteFileA": 1, "ExitProcess": 1,
    "FileTimeToDosDateTime": 3, "FileTimeToLocalFileTime": 2, "FindClose": 1,
    "FindFirstFileA": 2, "FindNextFileA": 2, "GetCommandLineA": 0,
    "GetConsoleMode": 2, "GetCurrentProcessId": 0, "GetCurrentThreadId": 0,
    "GetEnvironmentStrings": 0, "GetFileType": 1, "GetLastError": 0,
    "GetLocalTime": 1, "GetModuleFileNameA": 3, "GetProcAddress": 2,
    "GetStdHandle": 1, "GetTimeZoneInformation": 1, "GetVersion": 0,
    "LoadLibraryA": 1, "LocalAlloc": 2, "ReadFile": 5, "RtlUnwind": 4,
    "SetConsoleMode": 2, "SetFilePointer": 4, "SetStdHandle": 2, "WriteFile": 5
}


def configuration():
    meta = inventory()
    data = (TOOLCHAIN / ".patched/WLINK.EXE").read_bytes()
    text = meta["sections"][0]
    code = data[text["raw_offset"]:text["raw_offset"] + text["raw_size"]]
    relocation = next(row for row in meta["sections"] if row["name"] == ".reloc")
    at, stop = relocation["raw_offset"], relocation["raw_offset"] + relocation["raw_size"]
    fixups = set()
    while at < stop:
        page, size = struct.unpack_from("<II", data, at)
        require(size >= 8 and size % 2 == 0 and at + size <= stop, "Malformed relocation block")
        for index in range(at + 8, at + size, 2):
            item = struct.unpack_from("<H", data, index)[0]
            if item >> 12 == 3:
                require(page + (item & 4095) not in fixups, "Duplicate relocation")
                fixups.add(page + (item & 4095))
            else:
                require(item >> 12 == 0, "Unsupported relocation kind")
        at += size
    thunks = []
    for library in meta["imports"]:
        require(library["dll"].upper() == "KERNEL32.DLL", "Unknown import library")
        for symbol in library["symbols"]:
            name, iat = symbol["symbol"], symbol["iat_rva"]
            require(name in ARGUMENTS, "Unknown public API signature")
            encoded = b"\xff\x25" + struct.pack("<I", meta["image_base"] + iat)
            offsets = [index for index in range(len(code) - 5) if code[index:index + 6] == encoded]
            require(len(offsets) == 1, "Import does not have exactly one canonical jump thunk")
            rva = text["rva"] + offsets[0]
            absolute = encoded[2:]
            references = [index for index in range(len(code) - 3) if code[index:index + 4] == absolute]
            require(references == [offsets[0] + 2], "Additional absolute IAT reference outside the canonical thunk")
            require(rva + 2 in fixups, "Thunk IAT operand lacks original HIGHLOW relocation")
            require(not any(rva <= item < rva + 6 and item != rva + 2 for item in fixups), "Unexpected thunk relocation")
            thunks.append({"rva": rva, "iat_rva": iat, "argc": ARGUMENTS[name], "name": name,
                           "noreturn": name in ("ExitProcess", "RtlUnwind"), "original_bytes": encoded.hex()})
    thunks.sort(key=lambda row: row["rva"])
    require(len(thunks) == 30, "Unexpected import inventory")
    require(all(b["rva"] == a["rva"] + 6 for a, b in zip(thunks, thunks[1:])), "Transfer region is not contiguous six-byte import thunks")
    return {"purpose": "target-scoped unmasked diagnostic observer only; no application function acceptance",
            "capture_sha256": meta["capture_sha256"], "image_base": meta["image_base"],
            "image_size": 0x80000, "entry_rva": meta["entry_rva"], "main_rva": 0x242e0,
            "entry_first_byte": 0xe9, "main_first_byte": 0xb8, "main_kind": 1,
            "absolute_code_iat_references": "exactly one per import, its canonical FF25 operand; no full computed-address graph claim",
            "thunks": thunks}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    config = configuration()
    (args.output / "inventory.json").write_text(json.dumps(config, indent=2) + "\n")
    lines = ["image %x %x %x %x %x %x %x" % (config["image_base"], config["image_size"], config["entry_rva"], config["main_rva"], config["entry_first_byte"], config["main_first_byte"], config["main_kind"])]
    for row in config["thunks"]:
        lines.append("thunk %x %x %u %u %s" % (row["rva"], row["iat_rva"], row["argc"], row["noreturn"], row["name"]))
    (args.output / "sites.txt").write_text("\n".join(lines) + "\n")
    print(json.dumps({"thunks": len(config["thunks"]), "config_sha256": hashlib.sha256((args.output / "sites.txt").read_bytes()).hexdigest()}))


if __name__ == "__main__":
    main()
