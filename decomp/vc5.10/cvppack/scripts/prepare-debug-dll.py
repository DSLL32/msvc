#!/usr/bin/env python3
"""Explicit original-only source preparation, never invoked by native fixtures."""
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import time
from state import BINARY,ROOT,sha

spec=importlib.util.spec_from_file_location('original_tool',BINARY/'scripts/wine-tool.py')
tool=importlib.util.module_from_spec(spec);spec.loader.exec_module(tool)
case=BINARY/'cases/hello-debug-dll'
output=case/'preparation-0001'
output.mkdir(exist_ok=False)
work=Path.home()/'.ai-tmp/msvc2/vc5.10/cvppack/hello-debug-dll/preparation-0001'
work.mkdir(parents=True,exist_ok=False)
source=case/'hello-dll.c';shutil.copyfile(source,work/'hello-dll.c')
records=[]
for executable,arguments in [
    ('CL.EXE',['/nologo','/c','/MD','/Od','/Z7','/Fohello.obj','hello-dll.c']),
    ('LINK.EXE',['/nologo','/DLL','/DEBUG','/DEBUGTYPE:CV','/PDB:NONE','/NOPACK','/INCREMENTAL:NO','/OUT:hello.dll','hello.obj']),
]:
    command=['wine',tool.windows(tool.TC/'bin'/executable),*arguments]
    begin=time.perf_counter()
    result=subprocess.run(command,cwd=work,env=tool.environment(),stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=60)
    elapsed=time.perf_counter()-begin
    stem=executable.lower().replace('.exe','')
    (output/(stem+'-stdout')).write_bytes(result.stdout)
    (output/(stem+'-stderr')).write_bytes(result.stderr)
    records.append(dict(tool=executable,tool_sha256=sha(tool.TC/'bin'/executable),argv=command,cwd=tool.windows(work),status=result.returncode,seconds=elapsed))
    (output/'receipt.json').write_text(json.dumps(dict(schema=1,original_only=True,source_sha256=sha(source),commands=records),indent=2)+'\n')
    assert result.returncode==0,(executable,result.returncode,result.stdout,result.stderr)
for name in ('hello.obj','hello.dll','hello.lib','hello.exp'):
    if (work/name).exists():shutil.copyfile(work/name,output/name)
obj=json.loads((output/'receipt.json').read_text())
obj['files']=[dict(name=p.name,sha256=sha(p),bytes=p.stat().st_size) for p in output.iterdir() if p.name!='receipt.json']
obj['scratch_files']=[p.name for p in work.iterdir()]
(output/'receipt.json').write_text(json.dumps(obj,indent=2)+'\n')
print(json.dumps(obj,indent=2))
