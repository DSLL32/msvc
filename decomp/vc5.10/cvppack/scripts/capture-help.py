#!/usr/bin/env python3
"""Explicit original-only /help capture. Never called from a native fixture."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[2]
PREFIX = Path.home() / ".ai-tmp/vc1.0-capture/wine"
SCRATCH = Path.home() / ".ai-tmp/msvc2"
WORK = SCRATCH / "vc5.10/cvppack/help"
OUTPUT = HERE / "cases/help/original"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    start = time.perf_counter()
    if not PREFIX.is_dir():
        raise SystemExit("Shared Wine prefix is absent; capture will not create one")
    for letter, destination in (("r", ROOT), ("s", SCRATCH)):
        mapping = PREFIX / f"dosdevices/{letter}:"
        if not mapping.is_symlink() or mapping.resolve() != destination.resolve():
            raise SystemExit(f"Unexpected shared {letter.upper()}: mapping; refusing capture")
    if OUTPUT.exists():
        raise SystemExit("Help capture already exists; move it explicitly before recapturing")
    WORK.mkdir(parents=True, exist_ok=True)
    if any(WORK.iterdir()):
        raise SystemExit("Capture working directory is not empty")
    OUTPUT.mkdir(parents=True)
    command = ["wine", r"R:\toolchains\5.10\bin\CVPACK.EXE", "/help"]
    env = os.environ.copy()
    # Freeze shipped CRT/PDB DLL semantics instead of Wine's builtin CRT.
    overrides = "msvcrt,mspdb50=n"
    env.update(WINEPREFIX=str(PREFIX), WINEDEBUG="-all", WINEDLLOVERRIDES=overrides)
    setup = time.perf_counter() - start
    execute = time.perf_counter()
    with (OUTPUT / "stdout").open("wb") as stdout, (OUTPUT / "stderr").open("wb") as stderr:
        result = subprocess.run(command, cwd=WORK, env=env, stdout=stdout, stderr=stderr,
                                timeout=60, check=False)
    elapsed = time.perf_counter() - execute
    dependencies = [ROOT / f"toolchains/5.10/bin/{name}"
                    for name in ("CVPACK.EXE", "MSVCRT.DLL", "MSPDB50.DLL")]
    receipt = {
        "schema": 1, "kind": "unmodified-original-only-probe", "argv": command,
        "working_directory": r"S:\vc5.10\cvppack\help", "wine_prefix": str(PREFIX),
        "environment": {"WINEDEBUG": "-all", "WINEDLLOVERRIDES": overrides},
        "status": result.returncode,
        "inputs": [{"path": str(path.relative_to(ROOT)), "sha256": sha(path)}
                   for path in dependencies],
        "outputs": [{"name": name, "size": (OUTPUT / name).stat().st_size,
                     "sha256": sha(OUTPUT / name)} for name in ("stdout", "stderr")],
        "timing_seconds": {"setup": setup, "execution": elapsed,
                           "total": time.perf_counter() - start},
        "masked": False, "accepted_functions": 0,
        "note": "Groundwork behavior only; not masked-trap evidence or native validation.",
    }
    (OUTPUT / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
