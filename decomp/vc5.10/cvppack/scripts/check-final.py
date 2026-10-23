#!/usr/bin/env python3
"""Native-only final pair/audit/negative checks against the cached original."""
import argparse
import fcntl
import json
from pathlib import Path
import shutil
import subprocess
import sys
import time
from state import BINARY,sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--content-store',type=Path,default=BINARY/'build/source-content')
    args=parser.parse_args()
    output=BINARY/'build/final-native-validation'
    if output.exists():shutil.rmtree(output)
    output.mkdir(parents=True)
    case=json.loads((BINARY/'cases/help/case.json').read_text())
    reference=BINARY/case['reference']
    assert case['terminal']=='exit' and sha(reference/'receipt.json')==case['reference_receipt_sha256']
    original=output/'original';shutil.copytree(reference,original)
    native=output/'native';audit=output/'current-audit/audit.json'
    elapsed={}
    def run(name,command):
        begin=time.perf_counter()
        result=subprocess.run(command,cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30)
        elapsed[name]=time.perf_counter()-begin
        (output/(name+'.log')).write_bytes(result.stdout)
        assert result.returncode==0,(name,result.stdout.decode('latin1'))
    run('native_capture',[sys.executable,'scripts/native-debug.py','--original',str(original),
        '--config',str(original/'configuration.json'),'--content-store',str(args.content_store.resolve()),'--output',str(native)])
    run('candidate_audit',[sys.executable,'scripts/audit.py','--config',str(original/'configuration.json'),
        '--candidate','message_text','--reuse-original',str(original),'--reuse-native',str(native),'--output',str(audit)])
    run('pair_check',[sys.executable,'scripts/accept.py','--config',str(original/'configuration.json'),
        '--candidate','message_text','--original',str(original),'--native',str(native),'--audit',str(audit),'--check-only'])
    run('negative_checks',[sys.executable,'scripts/check-rejections.py','--config',str(original/'configuration.json'),
        '--pair',str(output),'--audit',str(audit)])
    report=dict(schema=1,native_only=True,reused_original_receipt_sha256=case['reference_receipt_sha256'],
        native_sha256=sha(BINARY/'build/native/CVPACK'),rebuilt_sha256=sha(BINARY/'build/CVPACK.EXE'),
        result=json.loads((output/'pair_check.log').read_text()),
        audit=json.loads(audit.read_text()),negatives=json.loads((BINARY/'build/rejections.json').read_text()),seconds=elapsed)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(native_only=True,result=report['result'],negative_checks=report['negatives']['count'],seconds=elapsed),indent=2))


if __name__=='__main__':
    # Independent reviews share this disposable directory; do not erase another
    # invocation's raw native capture while it is still running.
    (BINARY/'build').mkdir(exist_ok=True)
    with (BINARY/'build/final-native-validation.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        main()
