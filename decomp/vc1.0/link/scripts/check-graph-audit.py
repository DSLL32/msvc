#!/usr/bin/env python3
"""Read-only canonical graph checks and scratch mutation rejection tests.

Host tools only. No Wine/compiler, original or reconstructed executable is run.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import time
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from case_paths import ROOT,SCRATCH
from indexes import indexes

BINARY=Path(__file__).resolve().parents[1]
TOOL=ROOT/'tooling/graph-audit/target/release/graph-audit'
FLAGS=['--include-evidenced-runtime','--check-accepted-extents','--check-ret',
       '--check-references','--check-ownership','--check-extents']

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reuse-python-baseline',type=Path)
    options=parser.parse_args()
    directory=Path(tempfile.mkdtemp(prefix='verified-',dir=SCRATCH/'vc1.0/link/audit-adoption'))
    originals=[BINARY/p for p in ['functions.json','runtime.json','build/runtime.json','build/link.exe','build/link.map']]
    hashes=lambda:{str(p.relative_to(BINARY)):hashlib.sha256(p.read_bytes()).hexdigest() for p in originals}
    frozen=hashes()
    command=[str(TOOL),'--profile','link','--repo',str(ROOT),'--project',str(BINARY),*FLAGS]
    results=[]
    def run(label,arguments,accepted):
        start=time.perf_counter()
        result=subprocess.run(arguments,cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=60)
        elapsed=time.perf_counter()-start
        (directory/(label+'.stdout')).write_bytes(result.stdout)
        (directory/(label+'.stderr')).write_bytes(result.stderr)
        assert (result.returncode==0)==accepted,(label,result.returncode,result.stderr.decode(errors='replace'))
        record=dict(test=label,status='pass' if accepted else 'rejected',exit=result.returncode,
                    seconds=elapsed,command=arguments,stdout=str(directory/(label+'.stdout')),
                    stderr=str(directory/(label+'.stderr')))
        if accepted and label=='rust-positive':record['report']=json.loads(result.stdout)
        results.append(record)
        print(label,record['status'],round(elapsed,4),'seconds',flush=True)
    if options.reuse_python_baseline:
        baseline=json.loads(options.reuse_python_baseline.read_text())
        assert baseline['canonical_inputs']==frozen, 'Baseline canonical inputs changed'
        reference=next(t for t in baseline['tests'] if t['test']=='python-reference')
        results.append(dict(reference,reused_from=str(options.reuse_python_baseline)))
    else:
        run('python-reference',['python3','scripts/audit-graph.py'],True)
    run('rust-positive',command,True)
    original,rebuilt,config,old,new=indexes()
    decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    def wrong_call(name,filename):
        record=next(r for r in new if r['name']==name)
        call=next(i for i in decoder.disasm(rebuilt.read(record['start'],record['end']-record['start']),record['start']) if i.mnemonic=='call' and i.bytes[0]==0xe8)
        data=bytearray(rebuilt.data)
        struct.pack_into('<I',data,rebuilt.offset(call.address)+1,(0xdeadbeef-call.address-5)&0xffffffff)
        path=directory/filename
        path.write_bytes(data)
        return path
    path=wrong_call('_omf_record_common','wrong-driver-call.exe')
    run('wrong-driver-call',command+['--rebuilt',str(path)],False)
    path=wrong_call('_doexit','wrong-runtime-call.exe')
    run('wrong-runtime-call',command+['--rebuilt',str(path)],False)
    # This accepted stdcall helper has one actualRET4 and no inline table.
    # A linear search across arbitrary extents can mistake switch data for RET.
    return_record=next(r for r in new if r['name']=='_mapped_image_headers@4')
    assert return_record['name'] in config['accepted']
    candidates=[i for i in decoder.disasm(rebuilt.read(return_record['start'],return_record['end']-return_record['start']),return_record['start'])
                if i.mnemonic=='ret']
    assert len(candidates)==1 and candidates[0].size==3 and int(candidates[0].op_str,0)==4
    ret=candidates[0]
    data=bytearray(rebuilt.data)
    struct.pack_into('<H',data,rebuilt.offset(ret.address)+1,int(ret.op_str,0)+4)
    path=directory/'wrong-ret.exe'
    path.write_bytes(data)
    run('wrong-ret-cleanup',command+['--rebuilt',str(path)],False)
    results[-1]['mutation']=dict(function=return_record['name'],address=ret.address,original_cleanup=4,changed_cleanup=8,scope='Genuine soleRET4 in table-free accepted stdcall helper')
    runtime=json.loads((BINARY/'runtime.json').read_text())
    record=next(r for r in runtime if r['name']=='_mainCRTStartup')
    last=next(i for i in reversed(list(decoder.disasm(original.read(record['start'],record['end']-record['start']),record['start']))) if i.size>1)
    record['end']=last.address+1
    path=directory/'split-runtime-instruction.json'
    path.write_text(json.dumps(runtime,indent=2)+'\n')
    run('split-runtime-tail-instruction',command+['--runtime',str(path)],False)
    assert frozen==hashes(),'Canonical audit inputs changed'
    receipt=dict(status='pass',flags=FLAGS,tool_sha256=hashlib.sha256(TOOL.read_bytes()).hexdigest(),
                 canonical_inputs=frozen,tests=results,scope='Host-only original/rebuilt PE metadata audits; scratch mutations never execute',
                 native_regression='Cached native regression receipt is recorded in the current completed family validation.json; this host-only audit does not execute fixtures',
                 negative_design_note='First exploratory RETmutation targeted unaccepted control_handler and was outside audited scope; corrected to an accepted callee-cleanup routine.',
                 timings='Single samples include process startup, index loading and output; builds/native suite/original captures excluded')
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    # Compact checked result: full function/event reports stay in scratch.
    positive=next(t for t in results if t['test']=='rust-positive')
    count=positive.pop('report')
    positive['checked_functions']=count['checked_functions']
    positive['accepted_extent_boundaries']=count.get('checked_accepted_extents')
    receipt['raw_receipt']=str(directory/'receipt.json')
    (BINARY/'cases/graph-audit-validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(directory/'receipt.json')
if __name__=='__main__':main()
