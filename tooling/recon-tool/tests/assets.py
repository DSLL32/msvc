#!/usr/bin/env python3
"""Fixture assets stay outside the target tree and must remain unchanged."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TOOL = Path(os.environ.get("RECON_TOOL_TEST_EXE", ROOT / "target/i686-unknown-linux-gnu/debug/recon-tool"))
SUPPORT = Path(os.environ.get("RECON_TOOL_TEST_SUPPORT_DIR", ROOT))
FIXTURE = ROOT / "tests/fixture"
subprocess.run(["make", "-C", str(FIXTURE), "-j4", "cases",
    "BINARY_TOOL_SUPPORT=" + str(ROOT / "native/entry-hook.c") + " " + str(SUPPORT / "native/fixture.c")], check=True)
base = Path.home() / ".ai-tmp"
base.mkdir(exist_ok=True)
scratch = Path(tempfile.mkdtemp(prefix="recon-assets-test-", dir=base))
try:
    for changed in [False, True]:
        value = dict(assets=[dict(path="captures/response.bin", data="hex:00ff2a")],
            input_files=[dict(path="visible.txt", data="text:input")],
            env={"RECON_CHECK_ASSETS":"1", "RECON_CASE_ASSETS":"untrusted"},
            stdout="text:asset=3\n", stderr="text:", exit_code=0, reject_unexpected_files=True)
        if changed:
            value["env"]["RECON_CHANGE_ASSET"]="1"
        source=scratch/"capture.json"; packed=scratch/"capture.bin"; report=scratch/"report.json"
        source.write_text(json.dumps(value))
        subprocess.run([TOOL,"capture","pack",source,packed],check=True)
        result=subprocess.run([TOOL,"run",FIXTURE/"cases/0004-event","--capture",packed,"--report",report],capture_output=True,text=True)
        data=json.loads(report.read_text())[0]
        assert not Path(data["workspace"]).exists(), "workspace leaked"
        assert (result.returncode==0)==(not changed), result.stdout+result.stderr
        if changed:
            assert "asset" in str(data).lower()
    print("isolated asset access and mutation rejection passed")
finally:
    shutil.rmtree(scratch)
