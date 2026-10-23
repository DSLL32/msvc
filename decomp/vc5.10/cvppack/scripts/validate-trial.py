#!/usr/bin/env python3
"""Explicit new-function validation, including original capture; never a fixture."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import time
from state import BINARY, sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate',required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--promote',action='store_true')
    parser.add_argument('--case',choices=['help','hello-debug-dll'],default='help')
    args=parser.parse_args()
    output=args.output.resolve()
    assert not output.exists()
    config=json.loads((BINARY/'functions.json').read_text())
    assert [r['name'] for r in config['functions'] if r['status']=='candidate']==[args.candidate]
    elapsed={}
    from provenance import fingerprints,snapshot
    before_build=fingerprints()
    def run(name,command):
        begin=time.perf_counter()
        result=subprocess.run(command,cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        elapsed[name]=time.perf_counter()-begin
        output.mkdir(parents=True,exist_ok=True)
        (output/(name+'.log')).write_bytes(result.stdout)
        (output/'timings.json').write_text(json.dumps(elapsed,indent=2)+'\n')
        assert result.returncode==0,(name,result.stdout.decode('latin1'))
    run('original_build',['make','-j4','all','build/debug-run.exe'])
    run('native_build',['make','-j4','native'])
    assert fingerprints()==before_build, 'build inputs changed during compilation; retry before capturing'
    snapshot(output/'build-source-inputs.json')
    run('graph_tool_build',['make','graph-audit-tool'])
    original,native=output/'original',output/'native'
    run('original_capture',[sys.executable,'scripts/capture-original.py','--case',args.case,'--candidate',args.candidate,'--output',str(original)])
    run('native_capture',[sys.executable,'scripts/native-debug.py','--original',str(original),'--output',str(native)])
    run('pair_check',[sys.executable,'scripts/accept.py','--candidate',args.candidate,'--original',str(original),'--native',str(native),'--check-only'])
    result=json.loads((output/'pair_check.log').read_text())
    if args.promote:
        run('promotion',[sys.executable,'scripts/accept.py','--candidate',args.candidate,'--original',str(original),'--native',str(native)])
        from caseio import definition
        case=dict(schema=1,name=args.case,argv=definition(args.case)['argv'],reference=str(original.relative_to(BINARY)),
            reference_receipt_sha256=sha(original/'receipt.json'),terminal=result['terminal'],
            application_trace=(native/'entry-trace').read_text().splitlines())
        if result['terminal']=='exit':case['exit_status']=result['exit_status']
        (BINARY/'cases'/args.case/'case.json').write_text(json.dumps(case,indent=2)+'\n')
        run('native_fixture',['make','-j4','check'])
    (output/'validation.json').write_text(json.dumps(dict(schema=1,candidate=args.candidate,promoted=args.promote,result=result,
        original=json.loads((original/'receipt.json').read_text())['seconds'],
        native=json.loads((native/'receipt.json').read_text())['seconds'],seconds=elapsed),indent=2)+'\n')
    print(json.dumps(dict(candidate=args.candidate,promoted=args.promote,result=result,seconds=elapsed),indent=2))


if __name__=='__main__':main()
