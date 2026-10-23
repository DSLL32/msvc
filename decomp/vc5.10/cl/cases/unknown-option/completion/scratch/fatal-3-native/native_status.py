"""Decode the actual PID-owned full DWORD terminal receipt beside GDB status."""
import importlib.util
import json
from pathlib import Path
import subprocess


def read(directory,posix_status,verify_release=True):
    directory=Path(directory);reader=directory/'status_reader.py'
    if verify_release:
        build=json.loads((directory/'native-build.json').read_text())
        root=Path(__file__).resolve().parents[4]
        assert reader.read_bytes()==subprocess.check_output(['git','show',build['shared_revision']+':lib/recon/status.py'],cwd=root)
    spec=importlib.util.spec_from_file_location('cl_status_reader',reader)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    raw=(directory/'terminal-status.bin').read_bytes()
    assert len(raw)==24,'truncated/extra full DWORD status receipt'
    pid=json.loads((directory/'native-process.json').read_text())['pid']
    with module.TerminalStatus() as receipt:
        receipt.file.seek(0);receipt.file.write(raw);receipt.file.flush()
        return receipt.read(pid,posix_status)
