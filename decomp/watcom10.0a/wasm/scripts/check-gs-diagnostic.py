#!/usr/bin/env python3
"""Validate the cached, original-only four-site GS diagnostic; no execution."""

import hashlib
import json
from pathlib import Path
import re


HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[2]


def require(value, message):
    if not value:
        raise SystemExit(message)


def main():
    evidence = HERE / "cases/help/gs-diagnostic"
    receipt = json.loads((evidence / "receipt.json").read_text())
    events = (evidence / "events.txt").read_text()
    bound = {row["path"]: row for row in receipt["files"]}
    require(hashlib.sha256(events.encode()).hexdigest() == bound["events.txt"]["sha256"],
            "cached events do not match their capture receipt")
    original = ROOT / receipt["original_identities"]["target"]
    raw = original.read_bytes()
    require(hashlib.sha256(raw).hexdigest() == receipt["original_identities"]["target_sha256"],
            "original binary identity changed")
    image = raw[0x2200:0x2a482]
    require(image[0x167e4:0x167ea] == bytes.fromhex("81ec80000000"),
            "128-byte caller stack allocation changed")
    base = int(re.search(r"cf-observed-base=([0-9a-f]+)", events)[1], 16)
    require("cf-all-relocations-verified count=2915" in events,
            "missing actual relocated-image proof")
    require("actual-stdio-types stdin=1" in events and
            "actual-stdio-types stdout=1 stderr=1" in events, "stdio classes changed")
    lines = events.splitlines()
    sites = {}
    for index, line in enumerate(lines):
        if not line.startswith("one-shot-site "):
            continue
        name = re.search(r"name=(\S+)", line)[1]
        end = next((i for i in range(index + 1, len(lines)) if lines[i].startswith("event ")), len(lines))
        context_line = next(lines[i] for i in range(index - 1, -1, -1)
                            if lines[i].startswith("exception-context "))
        context = {k: int(v, 16) for k, v in re.findall(r"(\w+)=([0-9a-f]+)", context_line)
                   if k != "tid"}
        tail = lines[index:end]
        snapshot = next((line.split("bytes=")[1] for line in tail
                         if line.startswith("resource-buffer-raw ") or line.startswith("owner-buffer-raw ")), None)
        far = next((line for line in tail if line.startswith("resource-far-buffer ")), None)
        sites[name] = {"index": index, "context": context,
                       "buffer": bytes.fromhex(snapshot) if snapshot else None,
                       "far": {k: int(v, 10 if k == "capacity" else 16)
                               for k, v in re.findall(r"([\w-]+)=([0-9a-f]+)", far)} if far else None}
    owner = sites["owner_usage_call"]["context"]
    pointer = owner["esp"]
    require(owner["edx"] == pointer, "usage caller buffer is not its allocated stack span")
    require(sites["owner_stack_usage_return"]["context"]["esp"] == pointer,
            "usage caller did not regain the same stack span")
    pairs = [("resource_load_before", "resource_load_after", 0x1e611, 0x1e614),
             ("resource_store_before", "resource_store_after", 0x1e634, 0x1e637),
             ("resource_lgs_before", "resource_lgs_after", 0x1e653, 0x1e657),
             ("resource_nul_before", "resource_nul_after", 0x1e65b, 0x1e65f)]
    changes = {}
    for before, after, before_rva, after_rva in pairs:
        a, b = sites[before], sites[after]
        require(a["index"] < b["index"], "after-site preceded its before-site: " + before)
        for site, rva in ((a, before_rva), (b, after_rva)):
            require(site["context"]["eip"] == base + rva + 1, "wrong actual trap PC")
            require(site["far"]["pointer"] == pointer and site["far"]["capacity"] == 128 and
                    site["far"]["selector"] == site["context"]["ds"] and
                    site["far"]["caller-return"] == base + 0x1e6fe,
                    "far buffer is not the observed caller-owned flat-DS span")
            require(len(site["buffer"]) == 128, "truncated owned buffer capture")
        changes[before] = [(i, x, y) for i, (x, y) in enumerate(zip(a["buffer"], b["buffer"])) if x != y]
    entry_gs = sites["pe_entry"]["context"]["gs"]
    load_before = sites["resource_load_before"]["context"]
    load_after = sites["resource_load_after"]["context"]
    require(load_before["gs"] == entry_gs and entry_gs != load_before["ds"] and
            load_after["gs"] == load_after["ds"], "MOV GS transition not proved")
    require(changes["resource_load_before"] == [] and changes["resource_lgs_before"] == [],
            "selector/pointer load unexpectedly modified the owned buffer")
    store = sites["resource_store_before"]["context"]
    require(store["edx"] == pointer and changes["resource_store_before"] == [(0, 0x2b, store["ebx"] & 255)],
            "adjacent byte-store pair did not write exactly its observed destination/value")
    require(sites["resource_lgs_after"]["context"]["edx"] == pointer, "LGS result pointer differs")
    nul = sites["resource_nul_before"]["context"]
    require(nul["ecx"] == pointer + 53 and changes["resource_nul_before"] == [(53, 0x10, 0)],
            "adjacent NUL-store pair did not write exactly its observed destination")
    for name in ("reader_ret", "reader_return", "owner_stack_usage_return", "WriteFile"):
        require(sites[name]["context"]["gs"] == load_after["ds"],
                "GS did not remain clobbered through " + name)
    require("owned-exit-confirmed status=126" in events and
            "failure-owned-child exited-event=1 signaled=1 status=126" in events,
            "actual owned child termination was not confirmed")
    print(json.dumps({"status": "passed", "kind": "cached diagnostic proof only",
                      "cf_base": hex(base), "owned_buffer": hex(pointer),
                      "entry_gs": hex(entry_gs), "clobbered_gs": hex(load_after["gs"]),
                      "adjacent_pair_changes": changes,
                      "remaining_scope": "all future reader invocations require a selector==DS guard"}, indent=2))


if __name__ == "__main__":
    main()
