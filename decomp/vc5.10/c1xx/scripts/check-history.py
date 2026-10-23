#!/usr/bin/env python3
"""Read-only revalidation of frozen completion executions, never promotion."""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import time

PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]


def module(name,file):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(file))
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result


def configure(decoder,original,native,saved_record=None):
    # Anchor the untouched baseline before semantic-negative tests make copies.
    saved=saved_record or json.loads((PROJECT/'cases/lifecycle/validation.json').read_text())
    for directory,key in ((original,'original'),(native,'native')):
        decoder.s.require(decoder.s.sha(directory/'receipt.json')==saved[key+'_receipt_sha256'],
                          'not the recorded immutable completion proof')
    decoder.s.HISTORICAL_SNAPSHOT=native
    config=json.loads((native/'trial.json').read_text())
    graph=ROOT/'tooling/graph-audit/target/release/graph-audit'
    digest=decoder.s.sha(graph)
    started=time.perf_counter()
    run=subprocess.run([str(graph),'--repo',str(ROOT),'--project',str(PROJECT),'--profile','c1xx',
        '--discovery','cfg','--original',str(ROOT/config['target']['path']),
        '--rebuilt',str(native/'c1n32_l.dll'),'--map',str(native/'c1xx.map'),
        '--config',str(native/'trial.json'),'--runtime',str(native/'runtime.json'),
        '--rebuilt-runtime',str(native/'rebuilt-runtime.json'),'--metadata',str(native/'graph-metadata.json'),
        '--check-ret','--check-imports','--check-references','--check-ownership','--check-extents',
        '--include-evidenced-runtime','--check-accepted-extents'],check=True,stdout=subprocess.PIPE,text=True)
    report=json.loads(run.stdout)
    decoder.s.require(decoder.s.sha(graph)==digest,'auditor changed during historical validation')
    module('historical_import_check','audit.py').verify_initializer_thunk(
        report,config,native/'c1n32_l.dll',native/'rebuilt-runtime.json')
    proof={'schema':1,'kind':'read-only historical static validation; not current execution or acceptance',
           'trial':config['trial'],'trial_sha256':decoder.s.sha(native/'trial.json'),
           'auditor_sha256':digest,'seconds':time.perf_counter()-started,'report':report}
    path=PROJECT/'build/historical-audit-proof.json'
    payload=(json.dumps(proof,indent=2)+'\n').encode()
    history=PROJECT/'build/audits';history.mkdir(exist_ok=True)
    immutable=history/(__import__('hashlib').sha256(payload).hexdigest()+'.json')
    if not immutable.exists():immutable.write_bytes(payload)
    path.write_bytes(payload)
    return path


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original',type=Path);parser.add_argument('native',type=Path)
    args=parser.parse_args()
    decoder=module('completion_history','check-completion.py')
    audit=configure(decoder,args.original,args.native)
    result=decoder.check(args.original,args.native,audit_path=audit)
    print(json.dumps({'historical_validation':result,'fresh_audit':str(audit)},indent=2))


if __name__=='__main__':main()
