#!/usr/bin/env python3
"""Explicit original-only wcsncmp probe, outside native fixtures."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import time
HERE=Path(__file__).resolve().parents[1];ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('original_tools',HERE/'scripts/wine-tool.py');tools=importlib.util.module_from_spec(spec);spec.loader.exec_module(tools)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
out.relative_to(Path.home()/'.ai-tmp/msvc2/vc5.10/rcdll');assert not out.exists();out.mkdir(parents=True)
for source,name in [(HERE/'build/wcsncmp-api-probe.exe','probe.exe'),(HERE/'build/debug-original.exe','debug-original.exe'),(ROOT/'toolchains/5.10/bin/MSVCRT.DLL','MSVCRT.DLL'),(HERE/'scripts/wcsncmp-api-probe.c','compiled-probe.c'),(HERE/'build/debug-original-source.json','helper-compiled-source.json')]:shutil.copyfile(source,out/name)
(out/'ranges.txt').write_bytes(b'');command=['wine',str(out/'debug-original.exe'),'api-probe',tools.windows(out/'probe.exe'),'ranges.txt'];started=time.perf_counter()
with(out/'stdout.bin').open('wb')as stdout,(out/'stderr.bin').open('wb')as stderr:
    result=subprocess.run(command,cwd=out,env=tools.environment(),stdin=subprocess.DEVNULL,stdout=stdout,stderr=stderr,timeout=30)
report=dict(command=command,status=result.returncode,elapsed_seconds=time.perf_counter()-started,wineprefix=str(tools.PREFIX),provider_sha256=sha(out/'MSVCRT.DLL'),actual_provider_sha256=sha(out/'loaded-crt.bin'),actual_executable_sha256=sha(out/'loaded-driver.bin'),probe_sha256=sha(out/'probe.exe'),files={str(f.relative_to(out)):dict(bytes=f.stat().st_size,sha256=sha(f))for f in out.rglob('*')if f.is_file()})
(out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));assert result.returncode==0 and report['provider_sha256']==report['actual_provider_sha256']and report['actual_executable_sha256']==report['probe_sha256']
