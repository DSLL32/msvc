#!/usr/bin/env python3
"""Freeze/build authored Win32 observation tools; never execute their EXEs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[4]
BINARY = Path(__file__).resolve().parents[1]
SCRATCH = Path.home() / ".ai-tmp/msvc2"
BORROWED = SCRATCH / "watcom10.0a/wlib/startup-diagnostic/helper-build-v3/diagnose-startup.c"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def windows(path):
    path = path.resolve()
    for base, drive in ((ROOT, "R:"), (SCRATCH, "S:")):
        if path.is_relative_to(base):
            return drive + "\\" + str(path.relative_to(base)).replace("/", "\\")
    raise ValueError("outside the shared drives")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if sha(BORROWED) != "bb2d7176ba54d76367d0de370220516657c8b26a59580eb644a58d53aed37eb0":
        raise ValueError("reviewed transport pattern source changed")
    output = args.output.resolve()
    windows(output)
    output.mkdir(parents=True, exist_ok=False)
    names = ("observe-help.c", "observer-state.c", "observer-state.h", "observer-probe.c", "observer-probe-dll.c")
    for name in names:
        shutil.copyfile(BINARY / "scripts" / name, output / name)
    shutil.copyfile(BINARY / "cases/help/observer-probe-plan.json", output / "probe-plan.json")
    record = {"status": "building only; no observer/child execution", "builder_path": str(Path(__file__)), "builder_sha256": sha(Path(__file__)), "borrowed_source_sha256": sha(BORROWED),
              "sources": {name: sha(output / name) for name in names}, "probe_plan_sha256": sha(output / "probe-plan.json"), "commands": []}
    env = os.environ.copy()
    for name in ("CL", "_CL_", "LINK", "_LINK_", "INCLUDE", "LIB"):
        env.pop(name, None)
    env.update(WINEPREFIX=str(Path.home() / ".ai-tmp/vc1.0-capture/wine"), WINEDEBUG="-all",
               INCLUDE=windows(ROOT / "toolchains/5.10/include"), LIB=windows(ROOT / "toolchains/5.10/lib"))
    cl = windows(ROOT / "toolchains/5.10/bin/CL.EXE")
    link = windows(ROOT / "toolchains/5.10/bin/LINK.EXE")

    def run(command):
        start = time.monotonic()
        done = subprocess.run(command, cwd=output, env=env, input=b"", stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=45)
        index = len(record["commands"])
        log = output / ("build-%02d.log" % index)
        log.write_bytes(done.stdout + done.stderr)
        record["commands"].append({"argv": command, "status": done.returncode, "elapsed_seconds": time.monotonic() - start,
                                    "log": log.name, "log_sha256": sha(log)})
        (output / "receipt.json").write_text(json.dumps(record, indent=2) + "\n")
        if done.returncode:
            sys.stdout.buffer.write(done.stdout); sys.stderr.buffer.write(done.stderr)
            raise SystemExit("authored observation tool build failed")

    def compile(source, obj, extra=()):
        run(["wine", cl, "/nologo", "/c", "/W3", "/Od", "/MT", "/I" + windows(output),
             "/I" + windows(ROOT / "lib/recon/include"), *extra, "/Fo" + windows(output / obj), windows(output / source)])

    compile("observer-state.c", "observer-state.obj")
    compile("observe-help.c", "observe-help.obj")
    compile("observe-help.c", "observe-probe.obj", ("/DOBS_AUTHORED_PROBE",))
    compile("observer-probe-dll.c", "observer-probe-dll.obj")
    compile("observer-probe.c", "observer-probe.obj")
    run(["wine", link, "/nologo", "/dll", "/out:" + windows(output / "observer-probe.dll"),
         "/implib:" + windows(output / "observer-probe-dll.lib"), windows(output / "observer-probe-dll.obj"), "kernel32.lib"])
    for obj, exe in (("observe-help.obj", "observe-help.exe"), ("observe-probe.obj", "observe-probe.exe")):
        run(["wine", link, "/nologo", "/subsystem:console", "/out:" + windows(output / exe),
             windows(output / obj), windows(output / "observer-state.obj"), "kernel32.lib"])
    run(["wine", link, "/nologo", "/subsystem:console", "/map:" + windows(output / "observer-probe.map"),
         "/out:" + windows(output / "observer-probe.exe"), windows(output / "observer-probe.obj"),
         windows(output / "observer-probe-dll.lib"), "kernel32.lib"])
    record["status"] = "all authored Win32 tools built; no EXE/DLL runtime test performed"
    record["outputs"] = {name: sha(output / name) for name in ("observe-help.exe", "observe-probe.exe", "observer-probe.exe", "observer-probe.dll", "observer-probe.map")}
    (output / "receipt.json").write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps({"output": str(output), "status": record["status"], "outputs": record["outputs"],
                      "build_seconds": sum(item["elapsed_seconds"] for item in record["commands"])}, indent=2))


if __name__ == "__main__":
    main()
