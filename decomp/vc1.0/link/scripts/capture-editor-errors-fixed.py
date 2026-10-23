#!/usr/bin/env python3
"""Explicit original-only /FIXED companion of the independently validated DLL."""
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import time
from case_paths import SCRATCH,windows
BINARY=Path(__file__).resolve().parents[1]
BASE=BINARY/'cases/editor-errors'


def digest(data):return hashlib.sha256(data).hexdigest()


def main():
    out=BASE/'setup-fixed';assert not out.exists(),'Refuse to overwrite fixed capture'
    module=importlib.util.spec_from_file_location('valid_empty_editor_dll',BINARY/'scripts/capture-editor-errors-setup.py');setup=importlib.util.module_from_spec(module);module.loader.exec_module(setup)
    module=importlib.util.spec_from_file_location('fixed_original_tool',BINARY/'scripts/wine-tool.py');wt=importlib.util.module_from_spec(module);module.loader.exec_module(wt)
    source=BASE/'setup/empty.obj';original=json.loads((BASE/'setup/receipt.json').read_text());assert digest(source.read_bytes())==original['files']['empty.obj']['sha256']
    work=Path(tempfile.mkdtemp(prefix='editor-errors-fixed-',dir=SCRATCH/'vc1.0/link'));shutil.copyfile(source,work/'empty.obj')
    env=wt.environment()
    for name in ['LINK','_LINK_','MSC_CMD_FLAGS']:env.pop(name,None)
    args=['wine',windows(wt.TC/'bin/LINK.EXE'),'/nologo','/dll','/nodefaultlib','/entry:Empty','/fixed','/out:fixed.dll','empty.obj'];rows=[];inspections=[]
    for i in [1,2]:
        start=time.perf_counter();r=subprocess.run(args,cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=45);elapsed=time.perf_counter()-start
        (work/('run%d.stdout'%i)).write_bytes(r.stdout);(work/('run%d.stderr'%i)).write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
        data=(work/'fixed.dll').read_bytes();image=setup.inspect(data);pe=struct.unpack_from('<I',data,60)[0];relocs=struct.unpack_from('<II',data,pe+24+96+5*8);assert relocs==(0,0)
        flags=struct.unpack_from('<H',data,pe+22)[0];assert flags&1
        image['file_characteristics']=flags;image['relocations_stripped']=True
        image['base_relocation_directory']=list(relocs);inspections.append(image);(work/('fixed-%d.dll'%i)).write_bytes(data);rows.append(dict(command=args,seconds=elapsed,exit=r.returncode,stdout=r.stdout.decode('latin1'),stderr=r.stderr.decode('latin1')))
    assert inspections[0]['normalized_sha256']==inspections[1]['normalized_sha256']
    out.mkdir()
    for p in work.iterdir():
        assert p.is_file();shutil.copyfile(p,out/p.name)
    shutil.copyfile(work/'fixed-1.dll',BASE/'inputs/fixed.dll')
    (out/'receipt.json').write_text(json.dumps(dict(status='pass',scope='Valid ordinary /FIXED DLL with no-export/no-import/no-baserelocation directories; no image rewriting',tool_sha256=digest((wt.TC/'bin/LINK.EXE').read_bytes()),object_origin='setup/empty.obj, originalCL /c /O2 /G5',runs=rows,inspection=inspections,files={p.name:dict(bytes=p.stat().st_size,sha256=digest(p.read_bytes())) for p in out.iterdir() if p.is_file()}),indent=2)+'\n');print(out/'receipt.json')
if __name__=='__main__':main()
