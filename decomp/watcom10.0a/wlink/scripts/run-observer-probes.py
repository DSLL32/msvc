#!/usr/bin/env python3
"""Prepare or explicitly run the reviewed AUTHORED observer probes, never WLINK."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import struct
import subprocess
import time

ROOT = Path(__file__).resolve().parents[4]
SCRATCH = Path.home() / ".ai-tmp/msvc2"
MODES = ("bad-return", "return-overlap", "recursive", "thread", "trap", "timeout")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def windows(path):
    path = path.resolve()
    for base, drive in ((ROOT, "R:"), (SCRATCH, "S:")):
        if path.is_relative_to(base):
            return drive + "\\" + str(path.relative_to(base)).replace("/", "\\")
    raise ValueError("outside shared drives")


def bindings(build, config):
    receipt = json.loads((build / "receipt.json").read_text())
    manifest = json.loads((config / "manifest.json").read_text())
    if Path(manifest["build"]).resolve() != build or sha(build / "receipt.json") != manifest["build_receipt_sha256"]:
        raise ValueError("frozen build/config binding changed")
    expected = {str(build / name): value for name, value in receipt["sources"].items()}
    expected.update({str(build / name): value for name, value in receipt["outputs"].items()})
    expected.update({str(config / name): value for name, value in manifest["configuration"].items()})
    expected[str(build / "probe-plan.json")] = receipt["probe_plan_sha256"]
    expected[str(build / "receipt.json")] = sha(build / "receipt.json")
    expected[str(config / "manifest.json")] = sha(config / "manifest.json")
    for name, value in expected.items():
        if sha(Path(name)) != value:
            raise ValueError("frozen input changed: " + name)
    return expected


def cases(build, config, output):
    specs = [(True, "positive", 0)] + [(False, mode, 0) for mode in MODES]
    specs += [(True, mode, 0) for mode in MODES] + [(True, "positive", 1), (True, "positive", 2)]
    rows = []
    for index, (guarded, mode, fault) in enumerate(specs):
        label = ("guarded-" if guarded else "unguarded-") + mode + ("-fault%u" % fault if fault else "")
        case = output / "runs" / ("%02u-" % index + label)
        work = output / "flows" / mode
        marker = work / "marker.bin"
        if guarded:
            site = config / ("fault-%u.txt" % fault if fault else "sites.txt")
            deadline = "ADAPTIVE_POSITIVE_ELAPSED_PLUS_500_AT_MOST_4000" if mode == "timeout" else "8000"
            command = ["wine", windows(build / "observe-probe.exe"), windows(build / "observer-probe.exe"),
                       mode + ' "' + windows(marker) + '"', windows(work), windows(case / "stdout.bin"),
                       windows(case / "stderr.bin"), windows(case / "events.txt"), windows(site), deadline, "NUL"]
        else:
            command = ["wine", windows(build / "observer-probe.exe"), mode, windows(marker)]
        rows.append({"label": label, "guarded": guarded, "mode": mode, "fault": fault, "case": str(case),
                     "cwd": str(work), "marker": str(marker), "command_template": command})
    return rows


def prepare(args):
    build, config, output = args.build.resolve(), args.config.resolve(), args.output.resolve()
    windows(output)
    expected = bindings(build, config)
    output.mkdir(parents=True, exist_ok=False)
    environment = os.environ.copy()
    environment.update(WINEPREFIX=str(Path.home() / ".ai-tmp/vc1.0-capture/wine"), WINEDEBUG="-all")
    env_path = output / "host-environment.json"
    fd = os.open(env_path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "w") as stream:
        json.dump(environment, stream, indent=2, sort_keys=True); stream.write("\n")
    rows = cases(build, config, output)
    for mode in ("positive",) + MODES:
        work = output / "flows" / mode
        work.mkdir(parents=True)
        (work / "input.bin").write_bytes(b"abc")
    plan = {"kind": "fifteen authored Win32 observer runs only, no original Watcom executable",
            "build": str(build), "config": str(config), "output": str(output), "bindings": expected,
            "runner_path": str(Path(__file__).resolve()), "runner_sha256": sha(Path(__file__)),
            "environment": str(env_path), "environment_sha256": sha(env_path), "cases": rows,
            "timeout_policy": "guard timeout deadline = measured positive debug-loop milliseconds+500 <=4000; HOLD if positive exceeds3000ms",
            "counterfactual_input_policy": "same actual owned cwd/argv/input bytes for each guarded/unguarded mode; verified completion file archived then removed before next run; no full Windows environment equivalence claim"}
    (output / "run-plan.json").write_text(json.dumps(plan, indent=2) + "\n")
    print(json.dumps({"plan": str(output / "run-plan.json"), "plan_sha256": sha(output / "run-plan.json"), "runs": len(rows), "runner_sha256": plan["runner_sha256"]}, indent=2))


def parse_counters(events, kind):
    rows = re.findall(r"^probe-marker kind=" + re.escape(kind) + r" name=(\S+) address=[0-9a-f]+ value=([0-9a-f]+)$", events, re.M)
    return {name: int(value, 16) for name, value in rows}


def execute(args):
    plan_path = args.plan.resolve()
    plan = json.loads(plan_path.read_text())
    build, config, output = (Path(plan[key]).resolve() for key in ("build", "config", "output"))
    if sha(Path(__file__)) != plan["runner_sha256"] or plan["cases"] != cases(build, config, output):
        raise ValueError("reviewed runner/case vectors changed")
    if bindings(build, config) != plan["bindings"]:
        raise ValueError("reviewed inputs changed")
    env_path = Path(plan["environment"])
    if sha(env_path) != plan["environment_sha256"]:
        raise ValueError("exact host environment changed")
    env = json.loads(env_path.read_text())
    contract = json.loads((build / "probe-plan.json").read_text())
    positive_counters = dict(zip(contract["counterfactual_runs"]["completion_values_order"], contract["positive"]["completion_values"]))
    negative = {item["mode"]: item["actual_pre_termination_markers"] for item in contract["negative_cases"]}
    summary = {"status": "running", "plan_sha256": sha(plan_path), "results": []}
    positive_elapsed = None
    for row in plan["cases"]:
        started = time.monotonic()
        case, work, marker = (Path(row[key]) for key in ("case", "cwd", "marker"))
        if sorted(p.name for p in work.iterdir()) != ["input.bin"] or (work / "input.bin").read_bytes() != b"abc":
            raise ValueError("case input state changed")
        case.mkdir(parents=True, exist_ok=False)
        command = list(row["command_template"])
        if row["mode"] == "timeout" and row["guarded"]:
            if positive_elapsed is None or positive_elapsed > 3000:
                raise ValueError("positive timing does not justify adaptive timeout probe")
            command[-2] = str(positive_elapsed + 500)
        result = dict(row, command=command, host_environment_sha256=sha(env_path), before_files=[{"path": "input.bin", "sha256": sha(work / "input.bin")}])
        setup_elapsed = time.monotonic() - started
        run_started = time.monotonic()
        stdout = case / ("launcher.stdout.bin" if row["guarded"] else "stdout.bin")
        stderr = case / ("launcher.stderr.bin" if row["guarded"] else "stderr.bin")
        with stdout.open("wb") as out, stderr.open("wb") as err:
            child = subprocess.Popen(command, cwd=work, env=env, stdin=subprocess.DEVNULL, stdout=out, stderr=err, start_new_session=True)
            result["launcher_unix_pid"] = child.pid
            try:
                child.wait(timeout=15 if row["guarded"] else 10)
            except subprocess.TimeoutExpired:
                result["outer_timeout"] = True
                os.killpg(child.pid, signal.SIGKILL); child.wait(timeout=5)
        result["execution_seconds"] = time.monotonic() - run_started
        result["launcher_status"] = child.returncode
        checked = time.monotonic()
        events = (case / "events.txt").read_text(errors="strict") if (case / "events.txt").exists() else ""
        completion = marker.read_bytes() if marker.exists() else None
        values = list(struct.unpack("<5I", completion[4:])) if completion and len(completion) == 24 and completion[:4] == b"DONE" else None
        result.update(completion_values=values, completion_present=marker.exists())
        passed = not result.get("outer_timeout")
        if not row["guarded"]:
            passed &= child.returncode == 0 and values == contract["counterfactual_runs"]["completion_values"][row["mode"]]
        elif row["mode"] == "positive" and not row["fault"]:
            counters = parse_counters(events, "before-ExitProcess-JMP")
            result["actual_counters"] = counters
            found = re.search(r"observer-complete .*elapsed=(\d+)", events)
            positive_elapsed = int(found[1]) if found else None
            entries = len(re.findall(r"^api-enter ", events, re.M))
            returns = len(re.findall(r"^api-return ", events, re.M))
            rearms = len(re.findall(r"^JMP-rearmed ", events, re.M))
            result.update(entries=entries, returns=returns, rearms=rearms, positive_debug_loop_ms=positive_elapsed)
            passed &= child.returncode == 0 and values == contract["positive"]["completion_values"] and counters == positive_counters
            passed &= "owned-success-confirmed exit-event=1 signaled=1 event-status=0 actual-status=0" in events and positive_elapsed is not None
            passed &= entries == rearms and entries == returns + 1 and entries > 10
            for name, expected in (("ReadFile",2),("WriteFile",4),("_ProbeValue@8",2)):
                entered=len(re.findall(r"^api-enter .* name="+re.escape(name)+r" ",events,re.M))
                rearmed=len(re.findall(r"^JMP-rearmed name="+re.escape(name)+r" ",events,re.M))
                passed &= entered==expected and rearmed==expected
            passed &= bool(re.search(r"^bytes kind=defined-ReadFile-output .* count=3 hex=616263$",events,re.M))
            passed &= bool(re.search(r"^bytes kind=defined-ReadFile-output .* count=0 hex=$",events,re.M))
        else:
            counters = parse_counters(events, "before-owned-termination")
            result["actual_counters"] = counters
            passed &= child.returncode != 0 and completion is None and counters == negative[row["mode"]]
            passed &= "probe-marker-snapshot stopped=1 " in events and "owned-cleanup exit-event=1 signaled=1 " in events
            if row["mode"] == "timeout":
                passed &= bool(re.search(r"probe-marker-timeout-suspend previous=[0-9a-f]+ succeeded=1", events))
                deadline=re.search(r"deadline-check elapsed=(\d+) budget=(\d+) polls=(\d+)",events)
                passed &= bool(deadline) and int(deadline[1])>=int(deadline[2]) and int(deadline[3])<=20000
            if row["mode"] == "bad-return":
                passed &= bool(re.search(r"api-return-candidate name=_ProbeBadReturn@8 .* stack-result=7$",events,re.M))
            if row["mode"] == "recursive":
                passed &= bool(re.search(r"api-entry-candidate name=_ProbeValue@8 .* depth=1 ",events))
            if row["mode"] == "return-overlap":
                header=(config/"sites.txt").read_text().splitlines()[0].split()
                actual_main=int(header[1],16)+int(header[4],16)
                passed &= bool(re.search(r"api-enter .* name=_ProbeValue@8 .* return=%08x " % actual_main,events))
            if row["mode"] == "thread":
                passed &= "reason=unexpected-target-thread" in events
            if row["mode"] == "trap":
                passed &= "reason=unclassified-breakpoint" in events
            if row["fault"]:
                armed = re.findall(r"^context kind=JMP-TF-arm-readback (.+)$", events, re.M)
                delivered = re.findall(r"^context kind=JMP-step-delivered (.+)$", events, re.M)
                if not armed or not delivered:
                    passed = False
                else:
                    a = dict(re.findall(r"(\S+)=([0-9a-f]+)", armed[-1]))
                    b = dict(re.findall(r"(\S+)=([0-9a-f]+)", delivered[-1]))
                    result["fault_arm_context"], result["fault_delivered_context"] = a, b
                    passed &= all(a[key] == b.get(key) for key in a if key not in ("eip", "flags"))
                    passed &= int(a["flags"],16) & 0x100 != 0 and int(b["flags"],16) == int(a["flags"],16) & ~0x100
                    mutation=re.search(r"AUTHORED-PROBE-ONLY .* target=([0-9a-f]+)$",events,re.M)
                    passed &= bool(mutation) and b["eip"]==mutation[1]
                    passed &= "IAT-changed-during-step" in events if row["fault"] == 1 else "JMP-step-result=6 " in events
        result["after_files"]=[{"path":str(p.relative_to(work)),"sha256":sha(p)} for p in sorted(work.rglob("*")) if p.is_file()]
        expected_names=["input.bin","marker.bin"] if marker.exists() else ["input.bin"]
        passed &= [item["path"] for item in result["after_files"]]==expected_names
        result["passed"] = bool(passed)
        result["timing_seconds"] = {"setup": setup_elapsed, "execution": result["execution_seconds"], "checks": time.monotonic() - checked}
        result["artifacts"] = {p.name: {"bytes": p.stat().st_size, "sha256": sha(p)} for p in case.iterdir() if p.is_file()}
        if marker.exists():
            result["marker_sha256"] = sha(marker)
        (case / "receipt.json").write_text(json.dumps(result, indent=2) + "\n")
        summary["results"].append(result)
        (output / "receipt.json").write_text(json.dumps(summary, indent=2) + "\n")
        print(json.dumps({"case": row["label"], "passed": bool(passed), "status": child.returncode, "execution_seconds": result["execution_seconds"], "counters": result.get("actual_counters"), "completion_values": values}))
        if not passed:
            raise SystemExit("authored probe did not satisfy its contract; stopped before remaining cases")
        if marker.exists():
            marker.rename(case / "completion.bin")
    summary["status"] = "all fifteen authored observer probes passed; original WLINK not executed"
    summary["execution_seconds"] = sum(item["execution_seconds"] for item in summary["results"])
    (output / "receipt.json").write_text(json.dumps(summary, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    subs = parser.add_subparsers(dest="operation", required=True)
    prep = subs.add_parser("prepare")
    for name in ("build", "config", "output"):
        prep.add_argument("--" + name, type=Path, required=True)
    run = subs.add_parser("run")
    run.add_argument("--plan", type=Path, required=True)
    run.add_argument("--execute-reviewed", action="store_true", required=True)
    args = parser.parse_args()
    os.umask(0o077)
    prepare(args) if args.operation == "prepare" else execute(args)


if __name__ == "__main__":
    main()
