#!/usr/bin/env python3
"""Native direct legacy helpers with frozen original inputs/outputs and trace."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
BINARY=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--directory',type=Path);parser.add_argument('--exe',type=Path,default=BINARY/'build/native-trace/C13232.EXE');args=parser.parse_args()
    start=time.perf_counter();case=BINARY/'cases/legacy-type-support';manifest=json.loads((case/'manifest.json').read_text());config=json.loads((BINARY/'functions.json').read_text())
    spec=importlib.util.spec_from_file_location('legacy_support_contract',BINARY/'scripts/support-contract.py');contract=importlib.util.module_from_spec(spec);spec.loader.exec_module(contract)
    root=Path.home()/'.ai-tmp/msvc2/vc1.0/c1/validation';directory=args.directory or Path(tempfile.mkdtemp(prefix='support-check-',dir=root))
    if args.directory:directory.mkdir(parents=True,exist_ok=False)
    reports=[]
    for unit,row in enumerate(manifest['direct_oracles']):
        original=case/row['directory'];assert {str(p.relative_to(original)):hashlib.sha256(p.read_bytes()).hexdigest()for p in original.rglob('*')if p.is_file()}==row['files'],'cached support oracle changed'
        plan=manifest['direct_plan'][unit];assert plan==row['input'] and config['accepted'][row['accepted_from']:row['accepted_count']]==[plan.get('accepts',plan['function'])]
        if 'phase_sha256'in row:
            phase=manifest['phases'][plan['phase']]
            assert hashlib.sha256(json.dumps(phase,sort_keys=True).encode()).hexdigest()==row['phase_sha256'],'cached literal invocation changed'
        receipt=json.loads((original/'receipt.json').read_text());assert receipt['original_sha256']==config['target']['sha256']and receipt['terminal_exit']==99 and receipt['support_unit']==unit and receipt['direct_call']['function']==plan['function'];trace=receipt['first_entry_projection'];assert trace['observed']==trace['expected']==[plan.get('accepts',plan['function'])]
        if 'phase_sha256'in row:
            from case_paths import windows
            assert receipt['phase']==plan['phase']and receipt['child_environment']['MSC_CMD_FLAGS']==phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(BINARY.parents[2]/'toolchains/1.0-nt/bin'),include=windows(BINARY.parents[2]/'toolchains/1.0-nt/include')),'cached literal invocation differs'
        assert(trace['accepted_from'],trace['accepted_count'])==(row['accepted_from'],row['accepted_count'])
        assert trace['seeded_prefix_sha256']==hashlib.sha256(json.dumps(config['accepted'][:row['accepted_from']]).encode()).hexdigest()
        target=directory/str(unit);result=subprocess.run([sys.executable,str(BINARY/'scripts/direct-native.py'),'--support-unit',str(unit),'--directory',str(target),'--exe',str(args.exe.resolve()),'--body','--seen-count',str(row['accepted_from'])],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=35)
        (directory/f'{unit}.host.log').write_bytes(result.stdout);assert result.returncode==0,result.stdout.decode('latin1')
        check=time.perf_counter();comparison=contract.validate(original,target,plan,config);assert (target/'entries').read_text().splitlines()==[plan.get('accepts',plan['function'])[1:].split('@')[0]],'direct first-entry order differs'
        (target/'comparison.json').write_text(json.dumps(comparison,indent=2)+'\n');timing=json.loads((target/'receipt.json').read_text());reports.append(dict(unit=unit,status='pass',setup_seconds=timing['setup_seconds'],execution_seconds=timing['execution_seconds'],checks_seconds=time.perf_counter()-check))
    receipt=dict(status='pass',public_reachability=False,units=reports,elapsed_seconds=time.perf_counter()-start,policy='Native i386/GDB only; cached originals, real parsed owners, no target global seeding')
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print('Direct legacy support:',len(reports),'passed;',receipt['elapsed_seconds'],'seconds')
if __name__=='__main__':main()
