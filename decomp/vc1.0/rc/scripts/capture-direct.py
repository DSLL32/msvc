#!/usr/bin/env python3
"""Original-backed direct cases for two stateless helpers; no RC CLI claim."""
from pathlib import Path
import argparse
import hashlib
import importlib.machinery
import json
import os
import shutil
import subprocess
import sys

SCRIPTS=Path(__file__).resolve().parent
RC=SCRIPTS.parent
HERE=RC/'cases/direct-helpers'
sys.path.insert(0,str(SCRIPTS))
from binary import PE
from case_paths import capture_lock
wt=importlib.machinery.SourceFileLoader('direct_wine',str(SCRIPTS/'wine-tool.py')).load_module()
HELPERS={'whitespace':dict(name='_is_space_byte@4',rva=0xf3d0,bytes=43,results=1024,calls=256,cleanup=4),
         'basename':dict(name='_copy_basename@8',rva=0xf530,bytes=93,results=600,calls=6,cleanup=8)}


def record(path):
    data=path.read_bytes()
    return dict(path=os.path.relpath(path,HERE),bytes=len(data),sha256=hashlib.sha256(data).hexdigest())


def run(executable,mode,rva,work,stem):
    pe=PE(executable)
    command=['wine',wt.windows(RC/'build/direct-run.exe'),wt.windows(executable),mode,'%x'%rva,'%x'%pe.entry,
             wt.windows(work/'results.bin'),wt.windows(str(stem)+'.events'),
             wt.windows(str(stem)+'.stdout'),wt.windows(str(stem)+'.stderr')]
    subprocess.run(command,env=wt.environment(),stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL,check=True,timeout=90)
    return Path(str(stem)+'.events').read_text()


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--record',action='store_true');args=parser.parse_args()
    if args.record:assert not (HERE/'manifest.json').exists(),'Frozen direct case: verification only'
    original=wt.TC/'bin/RC.EXE'
    target_hash=hashlib.sha256(original.read_bytes()).hexdigest()
    assert target_hash=='eb2e4bb5994303a37326cc60107ee07be01ca267daca1f9116e3149d9ea31591'
    cases=[]
    for mode,helper in HELPERS.items():
        work=wt.SCRATCH/'vc1.0/rc'/('direct-'+mode)/'work'
        stem=RC/'build'/('capture-direct-'+mode)
        with capture_lock('direct-'+mode):
            if work.exists():shutil.rmtree(work)
            work.mkdir(parents=True)
            assert run(original,mode,helper['rva'],work,stem)=='exit 0\n'
        assert Path(str(stem)+'.stdout').read_bytes()==Path(str(stem)+'.stderr').read_bytes()==b''
        assert set(p.name for p in work.iterdir())=={'results.bin'}
        assert (work/'results.bin').stat().st_size==helper['results']
        proof=Path(str(stem)+'.events.direct').read_text()
        assert 'ABI cleanup=%d nonvolatile=EBX,ESI,EDI,EBP preserved'%helper['cleanup'] in proof
        assert 'passed=%d '%helper['calls'] in proof
        destination=HERE/mode
        if args.record:
            for file,source in [('results.bin',work/'results.bin'),('stdout.bin',Path(str(stem)+'.stdout')),
                                ('stderr.bin',Path(str(stem)+'.stderr')),('proof.log',Path(str(stem)+'.events.direct'))]:
                shutil.copyfile(source,destination/file)
        else:
            assert (work/'results.bin').read_bytes()==(destination/'results.bin').read_bytes()
        cases.append(dict(id='direct-'+mode,kind='direct-function-ABI',arguments=[mode],expected_exit=0,
             function=helper,stdout=record(destination/'stdout.bin'),stderr=record(destination/'stderr.bin'),
             outputs=[dict(output_name='results.bin',**record(destination/'results.bin'))],proof=record(destination/'proof.log')))
        print('direct-%s: native %d calls; stdcall %d, nonvolatile registers and exact results verified.'%
              (mode,helper['calls'],helper['cleanup']),flush=True)
    if args.record:
        manifest=dict(target_sha256=target_hash,source_inputs=[],toolchain_dependencies=[],cases=cases,
             analysis_helper=[record(RC/'src/direct-run.c'),record(SCRIPTS/'capture-direct.py')],
             comparison='Direct helper results and ABI only; target startup is a debugger return barrier, and exit 0 is controller-directed successful completion. No claim of native CLI reachability.')
        (HERE/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
