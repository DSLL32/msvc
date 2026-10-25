#!/usr/bin/env python3
"""Emit section-free i386 COFF weak aliases and matching GNU linker aliases.

This is build metadata, never function bodies or a reconstruction registry.
The auxiliary record is IMAGE_WEAK_EXTERN_SEARCH_ALIAS (3), with an ordinary
external canonical target: missing targets remain unresolved-link errors.
"""
import argparse
import json
from pathlib import Path
import re
import struct

FIELDS = {"win32_alias", "win32_target", "native_alias", "native_target"}


def unique_keys(pairs):
    value = {}
    for key, item in pairs:
        if key in value:
            raise ValueError("duplicate JSON key: " + key)
        value[key] = item
    return value


def load(path):
    raw = path.read_bytes()
    if len(raw) > 1024 * 1024:
        raise ValueError("alias map exceeds 1 MiB")
    value = json.loads(raw, object_pairs_hook=unique_keys)
    if (
        not isinstance(value, dict)
        or set(value) != {"schema", "aliases"}
        or type(value["schema"]) is not int
        or value["schema"] != 1
    ):
        raise ValueError("alias map requires schema1 and aliases only")
    rows = value["aliases"]
    if not isinstance(rows, list) or not 1 <= len(rows) <= 1024:
        raise ValueError("alias count must be 1..1024")
    for row in rows:
        if not isinstance(row, dict) or set(row) != FIELDS:
            raise ValueError("alias entry requires four symbol names only")
        for key, name in row.items():
            if key.startswith("native_"):
                pattern = r"[A-Za-z_.$][A-Za-z0-9_.$]*"
            else:
                pattern = r"[A-Za-z_?$@.][A-Za-z0-9_?$@.]*"
            if (
                not isinstance(name, str)
                or len(name) > 4096
                or not re.fullmatch(pattern, name)
            ):
                raise ValueError("unsupported symbol spelling: " + key)
    for prefix in ("win32", "native"):
        aliases = [row[prefix + "_alias"] for row in rows]
        targets = {row[prefix + "_target"] for row in rows}
        if len(set(aliases)) != len(rows) or set(aliases) & targets:
            raise ValueError("duplicate/self/chained " + prefix + " aliases")
    pairs = {}
    reverse = {}
    for row in rows:
        win = row["win32_target"]
        native = row["native_target"]
        if (win in pairs and pairs[win] != native) or (
            native in reverse and reverse[native] != win
        ):
            raise ValueError("canonical Win32/native target identities disagree")
        pairs[win] = native
        reverse[native] = win
    return rows


def coff(rows):
    targets = list(dict.fromkeys(row["win32_target"] for row in rows))
    names = targets + [row["win32_alias"] for row in rows]
    strings = bytearray()
    offsets = {}
    for name in names:
        offsets[name] = 4 + len(strings)
        strings += name.encode("ascii") + b"\0"

    def symbol(name, storage, auxiliary):
        return struct.pack("<IIIhHBB", 0, offsets[name], 0, 0, 0x20, storage, auxiliary)

    table = bytearray()
    for name in targets:
        table += symbol(name, 2, 0)
    for row in rows:
        table += symbol(row["win32_alias"], 105, 1)
        table += struct.pack("<II10s", targets.index(row["win32_target"]), 3, b"")
    # IMAGE_FILE_MACHINE_I386, no sections, table starts at the 20-byte header.
    header = struct.pack("<HHIIIHH", 0x14C, 0, 0, 20, len(targets) + 2 * len(rows), 0, 0)
    return header + table + struct.pack("<I", 4 + len(strings)) + strings


def native(rows):
    return "".join(
        "--defsym\n" + row["native_alias"] + "=" + row["native_target"] + "\n"
        for row in rows
    ).encode("ascii")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("map", type=Path)
    parser.add_argument("--coff", type=Path, required=True)
    parser.add_argument("--native-rsp", type=Path, required=True)
    args = parser.parse_args()
    if len({path.resolve() for path in (args.map, args.coff, args.native_rsp)}) != 3:
        raise ValueError("map and output paths must differ")
    rows = load(args.map)
    outputs = ((args.coff, coff(rows)), (args.native_rsp, native(rows)))
    for path, data in outputs:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, TypeError) as error:
        raise SystemExit("symbol aliases: " + str(error))
