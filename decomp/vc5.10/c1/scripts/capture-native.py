#!/usr/bin/env python3
"""Capture a native INT3 frontier using Linux GDB; no original execution."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time
from provenance import freeze, native_sources

PROJECT = Path(__file__).resolve().parents[1]
ROOT = PROJECT.parents[2]
SCRATCH = Path.home() / ".ai-tmp/msvc2/vc5.10/c1"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True)
    args = parser.parse_args()
    assert args.name.replace("-", "").replace("_", "").isalnum()
    output = SCRATCH / args.name
    output.mkdir(parents=True, exist_ok=False)
    binary = PROJECT / "build/native/c1-host"
    assert subprocess.run(["make", "-q", "build/native/c1-host"], cwd=PROJECT,
                          stdout=subprocess.DEVNULL).returncode == 0, "rebuild the stale native binary first"
    freeze(native_sources(), output / "source-snapshot.json")
    binary_sha = sha(binary)
    candidate_path = PROJECT / "cases/lifecycle/candidate.json"
    candidate = json.loads(candidate_path.read_text())
    import_boundary = candidate.get("expected_frontier", {}).get("kind") == "import-boundary"
    breakpoint_command = "break recon_initterm\n" if import_boundary else ""
    arguments = ("begin", "end") if import_boundary else ("module", "reason", "reserved")
    commands = """set pagination off
set confirm off
%s\
run
bt
python
import gdb,json
frame=gdb.newest_frame()
args={name:int(gdb.parse_and_eval('(unsigned int)'+name)) for name in %r}
value={'symbol':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'args':args}
caller=frame.older()
value['caller']={'symbol':caller.name(),'args':{}}
if frame.name()=='recon_initterm':
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int'))) for name in ('module','reason','reserved')}
identity=gdb.parse_and_eval('*c1_host_identity')
value['module_identity']={key:identity[key].string() for key in ('name','path','sha256')}
value['module_identity']['flags']=int(identity['flags'])
value['module_token']=int(gdb.parse_and_eval('(unsigned int)c1_host_module'))
value['crt_attached']=int(gdb.parse_and_eval('c1_crt_attached'))
value['raw_dllmain']=int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain'))
value['win32_last_error']=int(gdb.parse_and_eval('last_error'))
value['x87_control']=int(gdb.parse_and_eval('$fctrl'))
value['onexit_begin']=int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin'))
value['onexit_end']=int(gdb.parse_and_eval('(unsigned int)c1_onexit_end'))
value['onexit_first']=int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')) if value['onexit_begin'] else None
value['crt_adjust_fdiv']=int(gdb.parse_and_eval('c1_crt_adjust_fdiv'))
value['dll_instance']=int(gdb.parse_and_eval('(unsigned int)c1_dll_instance'))
value['initializer_begin']=int(gdb.parse_and_eval('(unsigned int)&c1_initializers[0]'))
value['initializer_end']=int(gdb.parse_and_eval('(unsigned int)&c1_initializers[1]'))
value['initializer_slots']=[int(gdb.parse_and_eval('(unsigned int)c1_initializers[0]'))]
open(%r,'w').write(json.dumps(value,indent=2)+'\\n')
end
quit
""" % (breakpoint_command, arguments, str(output / "state.json"))
    (output / "commands.gdb").write_text(commands)
    started = time.perf_counter()
    result = subprocess.run(["gdb", "--batch", "-x", str(output / "commands.gdb"),
                             "--args", str(binary)], cwd=PROJECT,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=15)
    elapsed = time.perf_counter() - started
    (output / "gdb.txt").write_bytes(result.stdout)
    assert result.returncode == 0
    if import_boundary:
        assert b"Breakpoint 1, recon_initterm (" in result.stdout
    else:
        assert b"Program received signal SIGTRAP" in result.stdout
    state = json.loads((output / "state.json").read_text())
    assert state["symbol"], "missing raw GDB function identity"
    assert sha(binary) == binary_sha
    (output / "candidate.json").write_bytes(candidate_path.read_bytes())
    (output / "functions.json").write_bytes((PROJECT / candidate.get("audit_config", "functions.json")).read_bytes())
    if candidate.get("functions"):
        binding = json.loads((PROJECT / "build/trial-audit-binding.json").read_text())
        assert all(sha(ROOT / path) == value for path, value in binding["inputs"].items())
        for source, target in (("trial-audit.json", "audit.json"), ("trial-audit-binding.json", "audit-binding.json")):
            (output / target).write_bytes((PROJECT / "build" / source).read_bytes())
    receipt = {"schema": 1, "kind": "native-gdb-frontier", "seconds": elapsed,
               "native_sha256": binary_sha,
               "rebuilt_sha256": sha(PROJECT / "build/c1.dll"),
               "candidate_sha256": sha(output / "candidate.json"),
               "functions_sha256": sha(output / "functions.json"),
               "source_hashes": {str(path.relative_to(PROJECT)): sha(path)
                                 for path in sorted((PROJECT / "src").glob("*")) if path.is_file()},
               "audit_binding_sha256": sha(output / "audit-binding.json") if (output / "audit-binding.json").exists() else None,
               "artifacts": {p.name: {"sha256": sha(p), "size": p.stat().st_size}
                             for p in sorted(output.iterdir()) if p.is_file()}}
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"output": str(output), "symbol": state["symbol"], "seconds": elapsed}))


if __name__ == "__main__":
    main()
