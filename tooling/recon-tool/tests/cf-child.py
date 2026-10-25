#!/usr/bin/env python3
"""Build/run bounded authored native-child controls; never invokes Wine."""
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time

HERE=Path(__file__).resolve().parent
SCRATCH=Path.home()/".ai-tmp/msvc2/recon-tool-cf-child"


def main():
    SCRATCH.mkdir(parents=True,exist_ok=True)
    folder=Path(tempfile.mkdtemp(prefix="child-controls-",dir=SCRATCH))
    binary=folder/"gate-child"
    command=["gcc","-m32","-O0","-g","-fno-pie","-no-pie",str(HERE/"cf-gate-child.c"),"-o",str(binary)]
    t=time.monotonic()
    build=subprocess.run(command,capture_output=True,timeout=15)
    build_time=time.monotonic()-t
    (folder/"build.stdout").write_bytes(build.stdout);(folder/"build.stderr").write_bytes(build.stderr)
    if build.returncode:
        sys.stderr.buffer.write(build.stderr);return build.returncode
    result=folder/"child.json"
    script=folder/"commands.gdb"
    script.write_text("set pagination off\nset confirm off\nset python print-stack full\nset startup-with-shell off\nset disable-randomization off\nbreak probe_ready\nrun\npython\nimport sys\nsys.path.insert(0,"+repr(str(HERE))+")\nimport cf_child_controls\ntry:\n cf_child_controls.run("+repr(str(result))+")\nfinally:\n import gdb\n if gdb.selected_inferior().pid: gdb.execute('kill',to_string=True)\nend\nquit\n")
    t=time.monotonic()
    with (folder/"gdb.stdout").open("wb") as out,(folder/"gdb.stderr").open("wb") as err:
        process=subprocess.Popen(["gdb","-q","-batch","-x",str(script),str(binary)],stdout=out,stderr=err)
        try:
            process.wait(timeout=15)
        except subprocess.TimeoutExpired:
            # Stop this debugger before enumerating its own immediate children,
            # including a child that has not yet reached probe_ready. Do not
            # signal any shared server or unrelated process.
            try:os.kill(process.pid,signal.SIGSTOP)
            except ProcessLookupError:pass
            children=Path('/proc/%d/task/%d/children'%(process.pid,process.pid))
            for value in children.read_text().split() if children.exists() else []:
                pid=int(value)
                try:
                    status=Path('/proc/%d/status'%pid).read_text()
                    parent=next(int(row.split()[1]) for row in status.splitlines() if row.startswith('PPid:'))
                    if parent==process.pid:
                        os.kill(pid,signal.SIGKILL)
                except (ProcessLookupError,FileNotFoundError):
                    pass
            process.kill();process.wait(timeout=5)
            raise RuntimeError("native child controller deadline exhausted; owned child/debugger stopped")
    execute_time=time.monotonic()-t
    if process.returncode or not result.exists():
        sys.stderr.write((folder/"gdb.stderr").read_text()+"\nprobe logs: "+str(folder)+"\n")
        return 1
    receipt=json.loads(result.read_text())
    if Path('/proc/%d/exe'%receipt['owned_pid']).resolve()==binary:
        raise RuntimeError("authored child remained alive after debugger teardown")
    receipt.update(build_seconds=build_time,gdb_setup_execution_checks_seconds=execute_time,
                   command=command,logs=str(folder),
                   gate_sha256=hashlib.sha256((HERE.parent/"watcom/gate.py").read_bytes()).hexdigest(),
                   checker_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                   gdb_controls_sha256=hashlib.sha256((HERE/"cf_child_controls.py").read_bytes()).hexdigest(),
                   child_source_sha256=hashlib.sha256((HERE/"cf-gate-child.c").read_bytes()).hexdigest(),
                   binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                   owned_child_termination_confirmed=True,debugger_exit_status=process.returncode)
    (folder/"receipt.json").write_text(json.dumps(receipt,indent=2)+"\n")
    print(json.dumps(receipt,indent=2))
    return 0


if __name__=="__main__":
    sys.exit(main())
