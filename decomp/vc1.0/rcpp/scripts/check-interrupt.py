#!/usr/bin/env python3
"""Stress the assigned real Ctrl+C case and retain its final raw replay proof."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
from compare import compare, HERE, SCRATCH, case_spec

NAME='console-interrupt'
FIXTURE=HERE.parent/'fixtures/rcpp-interrupt'


def digest(data):
    return dict(bytes=len(data),sha256=hashlib.sha256(data).hexdigest())


def collect(label,destination):
    source=SCRATCH/'cases'/NAME/label
    work=SCRATCH/'runs'/NAME/label
    proof_data=Path(str(source)+'.proof').read_bytes()
    proof=proof_data.decode('ascii')
    assert Path(str(source)+'.events').read_bytes()==b'exit 1\n'
    assert 'cleanup pipe_disconnected=1 output_exclusive_open=1' in proof
    assert 'reached_handler=1 controller_breaks=0 controller_interrupts=1' in proof
    callback=re.search(r'callback pid=(\d+) tid=(\d+) argument=(\d+) handler_rva=([0-9a-f]+)',proof)
    primary=int(re.search(r'primary_tid=(\d+)',proof)[1])
    assert callback and int(callback[2])!=primary and int(callback[3])==0
    assert 'control_exception=40010005' in proof and 'disposition=not_handled' in proof
    if destination.exists():shutil.rmtree(destination)
    destination.mkdir(parents=True)
    for kind in ['proof','events','stdout','stderr','launcher']:
        shutil.copyfile(Path(str(source)+'.'+kind),destination/kind)
    shutil.copytree(Path(str(source)+'.before'),destination/'before')
    shutil.copytree(work,destination/'after')
    for stream in ['stdout','stderr']:
        assert (destination/stream).read_bytes()==(FIXTURE/'oracle'/(stream+'.bin')).read_bytes()
    assert (destination/'before/result.i').read_bytes()==b''
    output=(destination/'after/result.i').read_bytes()
    assert output==(FIXTURE/'oracle/result.i').read_bytes()
    return dict(pid=int(callback[1]),primary_tid=primary,callback_tid=int(callback[2]),
                argument=int(callback[3]),handler_rva=int(callback[4],16),exit=1,
                proof=digest(proof_data),output=digest(output),
                pipe_disconnected=True,output_exclusive_open=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--count',type=int,default=10)
    parser.add_argument('--record-only',action='store_true')
    args=parser.parse_args()
    path=HERE/'interrupt-validation.json'
    if args.record_only:
        report=json.loads(path.read_text())
        assert report['stress_repeats']>=10
        report['stress_reconstructed_sha256']=report['reconstructed_sha256']
        report['final_replay_reconstructed_sha256']=hashlib.sha256((HERE/'build/rcpp.exe').read_bytes()).hexdigest()
        report['final_replay']={label:collect(label,HERE/'evidence/console-interrupt'/label)
                                for label in ['original','reconstructed','baseline']}
    else:
        assert args.count>=1
        case_spec(NAME)
        report=dict(case=NAME,target_sha256=hashlib.sha256((HERE/'../../../toolchains/1.0-nt/bin/RCPP.EXE').resolve().read_bytes()).hexdigest(),
                    reconstructed_sha256=hashlib.sha256((HERE/'build/rcpp.exe').read_bytes()).hexdigest(),
                    stress_repeats=args.count,runs=[])
        for number in range(args.count):
            assert compare(case_name=NAME)=='exit:1'
            destination=SCRATCH/'interrupt/stress'/('%02d'%number)
            report['runs'].append({label:collect(label,destination/label)
                                   for label in ['original','reconstructed','baseline']})
            print('Real Ctrl+C repetition %d/%d passed.'%(number+1,args.count),flush=True)
    path.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':main()
