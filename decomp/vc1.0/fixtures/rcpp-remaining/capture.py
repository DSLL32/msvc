#!/usr/bin/env python3
"""Two successful suffix cases against the untouched RCPP, twice each."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys

HERE=Path(__file__).resolve().parent
RCPP=HERE.parent.parent/'rcpp'
sys.path.insert(0,str(RCPP))
from compare import run,location,wt

CASES=['long-suffix','unsigned-suffix']


def record(path):
    data=path.read_bytes()
    return dict(path=path.relative_to(HERE).as_posix(),bytes=len(data),sha256=hashlib.sha256(data).hexdigest())


def main():
    os.chdir(RCPP)
    cfg=json.loads((RCPP/'functions.json').read_text())
    original=wt.ROOT/cfg['target']['path']
    assert hashlib.sha256(original.read_bytes()).hexdigest()==cfg['target']['sha256']
    pinned=None
    if (HERE/'manifest.json').exists():
        pinned=json.loads((HERE/'manifest.json').read_text())
        for item in pinned['source_inputs']+pinned['capture_evidence']+pinned.get('matched_frontier_evidence',[]):
            data=(HERE/item['path']).read_bytes()
            assert len(data)==item['bytes'] and hashlib.sha256(data).hexdigest()==item['sha256']
    cases=[]
    for name in CASES:
        arguments=['-ef',r'{bin}\c1.err','-f',name+'.rc','-g','result.i','-E']
        command=subprocess.list2cmdline([x.replace('{bin}',r'T:\bin') for x in arguments])
        spec=dict(name=name,fixture=HERE,arguments=command,extra_input=None)
        outputs=[]
        for number in range(2):
            label='capture-%d'%number
            events=run(label,original,command,spec)
            assert events=='exit 0\n',events
            work,stem=location(label,spec)
            streams={kind:Path(str(stem)+'.'+kind).read_bytes() for kind in ['stdout','stderr']}
            assert streams==dict(stdout=b'',stderr=b''),streams
            outputs.append((streams,(work/'result.i').read_bytes()))
            assert {p.name for p in work.iterdir() if p.is_file()}=={n+'.rc' for n in CASES}|{'result.i'}
            if pinned is None:
                destination=HERE/'evidence'/name/('original-%d'%number)
                destination.mkdir(parents=True,exist_ok=True)
                for kind in ['stdout','stderr','events']:
                    shutil.copyfile(Path(str(stem)+'.'+kind),destination/kind)
                shutil.copyfile(work/'result.i',destination/'result.i')
        assert outputs[0]==outputs[1]
        if pinned is None:
            oracle=HERE/'cases'/name;oracle.mkdir(parents=True,exist_ok=True)
            for kind,data in outputs[0][0].items():(oracle/(kind+'.bin')).write_bytes(data)
            (oracle/'result.i').write_bytes(outputs[0][1])
            cases.append(dict(id=name,arguments=arguments,expected_exit=0,
                              stdout=record(oracle/'stdout.bin'),stderr=record(oracle/'stderr.bin'),
                              outputs=[dict(record(oracle/'result.i'),output_name='result.i')]))
        else:
            oracle=HERE/'cases'/name
            assert outputs[0]==({kind:(oracle/(kind+'.bin')).read_bytes() for kind in ['stdout','stderr']},(oracle/'result.i').read_bytes())
        print('%s: two exact original successes, %d output bytes, empty streams.'%(name,len(outputs[0][1])),flush=True)
    if pinned is None:
        manifest=dict(toolchain='1.0-nt',target=cfg['target']['path'],target_sha256=cfg['target']['sha256'],
                      status='captured-before-acceptance',normalization='None: raw status, streams, exact output file set and bytes.',
                      original_capture_runs_per_case=2,source_inputs=[record(p) for p in sorted((HERE/'source').iterdir())],
                      toolchain_dependencies=[dict(path='toolchains/1.0-nt/bin/C1.ERR',sha256=hashlib.sha256((wt.TC/'bin/C1.ERR').read_bytes()).hexdigest())],
                      environment=dict(INCLUDE='{include}',WINEPATH='{bin}'),
                      unset_environment=['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS'],cases=cases,
                      capture_evidence=[record(p)for p in sorted((HERE/'evidence').rglob('*'))if p.is_file()])
        (HERE/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')


if __name__=='__main__':main()
