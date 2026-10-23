#!/usr/bin/env python3
"""Explicit Watcom compilation/linking only; never a fixture dependency.

Build recipe follows the verified wpp386 scaffold in this project.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parents[4]
PROJECT = Path(__file__).resolve().parents[1]
TOOLCHAIN = ROOT / "toolchains/watcom10.0a"
HASHES = {
    "binb/WCC386.EXE": "c3666de94f6fa6800f452dae8acf45505ecdb62f0ade2cc27cc86c2d9e8e2b6b",
    "binnt/WCC386.EXE": "8fee7d6ab3f2c06a0a874135e3898386898679e7dd53993fcc508e296364767d",
    ".patched/WLINK.EXE": "509c749b2258029a25a0071e34fa13b76a2fd4b267591a2bb12e9a7be5fd665b",
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def windows(path):
    return "R:\\" + str(Path(path).resolve().relative_to(ROOT)).replace("/", "\\")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("operation", choices=("compile", "link"))
    parser.add_argument("paths", nargs="+")
    args = parser.parse_args()
    for relative, expected in HASHES.items():
        if sha(TOOLCHAIN / relative) != expected:
            raise ValueError("original tool identity mismatch: " + relative)
    env = os.environ.copy()
    env.update(WINEPREFIX=str(Path.home() / ".ai-tmp/vc1.0-capture/wine"),
               WINEDEBUG="-all", WATCOM=windows(TOOLCHAIN))
    for key in ("WCC386", "WCC", "INCLUDE"):
        env.pop(key, None)
    if args.operation == "compile":
        source, output = map(Path, args.paths)
        command = ["/usr/bin/wine", windows(TOOLCHAIN / "binnt/WCC386.EXE"),
                   "-bt=nt", "-3r", "-od", "-d2",
                   "-i=" + windows(ROOT / "lib/recon/include"),
                   "-fo=" + windows(output), windows(source)]
        receipt = PROJECT / "build" / (output.stem + "-compile.json")
        inputs = {str(source): sha(source)}
        outputs = [output]
    else:
        response = PROJECT / "build/wlib.lnk"
        lines = ["format windows nt", "runtime console", "debug all",
                 "option map=" + windows(PROJECT / "build/wlib.map"),
                 "name " + windows(PROJECT / "build/WLIB.EXE")]
        lines.extend("file " + windows(path) for path in args.paths)
        lines.extend(["libpath " + windows(TOOLCHAIN / "lib386"),
                      "libpath " + windows(TOOLCHAIN / "lib386/NT"),
                      "library clib3r", "library nt"])
        response.write_text("\n".join(lines) + "\n")
        command = ["/usr/bin/wine", windows(TOOLCHAIN / ".patched/WLINK.EXE"),
                   "@" + windows(response)]
        receipt = PROJECT / "build/link.json"
        inputs = {path: sha(Path(path)) for path in args.paths}
        inputs[str(response)] = sha(response)
        outputs = [PROJECT / "build/WLIB.EXE", PROJECT / "build/wlib.map"]
    start = time.monotonic()
    completed = subprocess.run(command, cwd=PROJECT, env=env, input=b"",
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               timeout=45)
    record = dict(operation=args.operation, command=command,
                  toolchain_hashes=HASHES, inputs=inputs,
                  outputs={str(path): sha(path) for path in outputs if path.is_file()},
                  elapsed_seconds=time.monotonic() - start,
                  returncode=completed.returncode,
                  stdout=completed.stdout.decode("latin1"),
                  stderr=completed.stderr.decode("latin1"))
    receipt.write_text(json.dumps(record, indent=2) + "\n")
    sys.stdout.buffer.write(completed.stdout)
    sys.stderr.buffer.write(completed.stderr)
    return completed.returncode


if __name__ == "__main__":
    sys.exit(main())
