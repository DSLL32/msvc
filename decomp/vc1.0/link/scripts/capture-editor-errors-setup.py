#!/usr/bin/env python3
"""Explicit original-only valid no-export DLL setup; never a fixture dependency."""
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import time
from case_paths import ROOT,SCRATCH,PREFIX,windows

BINARY=Path(__file__).resolve().parents[1]
BASE=BINARY/'cases/editor-errors'


def digest(data):return hashlib.sha256(data).hexdigest()


def inspect(data):
    pe=struct.unpack_from('<I',data,60)[0]
    assert data[:2]==b'MZ' and data[pe:pe+4]==b'PE\0\0'
    machine,sections=struct.unpack_from('<HH',data,pe+4)
    flags=struct.unpack_from('<H',data,pe+22)[0];optional=pe+24
    assert machine==0x14c and sections and flags&0x2000
    assert struct.unpack_from('<H',data,optional)[0]==0x10b
    entry=struct.unpack_from('<I',data,optional+16)[0];assert entry
    exports=struct.unpack_from('<II',data,optional+96)
    imports=struct.unpack_from('<II',data,optional+104)
    assert exports==imports==(0,0)
    table=optional+struct.unpack_from('<H',data,pe+20)[0]
    code=None
    for i in range(sections):
        p=table+40*i;rva,size,offset=struct.unpack_from('<III',data,p+12)
        if rva<=entry<rva+size:code=data[offset+entry-rva:offset+entry-rva+8]
    assert code==bytes.fromhex('b801000000c20c00'),code
    normalized=bytearray(data);normalized[pe+8:pe+12]=bytes(4)
    return dict(machine=machine,DLL=True,entry_rva=entry,entry_bytes=code.hex(),export_directory=list(exports),import_directory=list(imports),normalized_sha256=digest(normalized),clock=dict(offset=pe+8,bytes=4,name='PE COFF TimeDateStamp'))


def main():
    out=BASE/'setup';assert not out.exists(),'Refuse to overwrite setup capture'
    spec=importlib.util.spec_from_file_location('editor_setup_wine',BINARY/'scripts/wine-tool.py');wt=importlib.util.module_from_spec(spec);spec.loader.exec_module(wt)
    work=Path(tempfile.mkdtemp(prefix='editor-errors-setup-',dir=SCRATCH/'vc1.0/link'))
    shutil.copyfile(BASE/'inputs/empty.c',work/'empty.c');env=wt.environment()
    for name in ['LINK','_LINK_','MSC_CMD_FLAGS']:env.pop(name,None)
    rows=[]
    commands=[['CL.EXE','/nologo','/c','/O2','/G5','/Foempty.obj','empty.c']]+[['LINK.EXE','/nologo','/dll','/nodefaultlib','/entry:Empty','/out:provider.dll','empty.obj']]*2
    pe_runs=[]
    for i,args in enumerate(commands):
        command=['wine',windows(wt.TC/'bin'/args[0]),*args[1:]];start=time.perf_counter()
        r=subprocess.run(command,cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=45)
        elapsed=time.perf_counter()-start;(work/('run%d.stdout'%i)).write_bytes(r.stdout);(work/('run%d.stderr'%i)).write_bytes(r.stderr)
        rows.append(dict(command=command,seconds=elapsed,exit=r.returncode,stdout=r.stdout.decode('latin1'),stderr=r.stderr.decode('latin1'),tool_sha256=digest((wt.TC/'bin'/args[0]).read_bytes())))
        assert r.returncode==0,(command,r.stdout,r.stderr)
        if i:
            data=(work/'provider.dll').read_bytes();parsed=inspect(data);pe_runs.append(parsed);(work/('provider-%d.dll'%i)).write_bytes(data)
    assert pe_runs[0]['normalized_sha256']==pe_runs[1]['normalized_sha256']
    out.mkdir()
    for name in ['empty.c','empty.obj','provider-1.dll','provider-2.dll']+[('run%d.%s'%(i,k)) for i in range(3) for k in ['stdout','stderr']]:shutil.copyfile(work/name,out/name)
    shutil.copyfile(work/'provider-1.dll',BASE/'inputs/provider.dll')
    receipt=dict(status='pass',scope='Explicit original setup outside fixtures; valid stdcall DLL entry, no exports/imports, inspect-only inputs',working_directory=windows(work),prefix=str(PREFIX),runs=rows,inspection=pe_runs,files={p.name:dict(bytes=p.stat().st_size,sha256=digest(p.read_bytes())) for p in out.iterdir() if p.is_file()})
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(out/'receipt.json')
if __name__=='__main__':main()
