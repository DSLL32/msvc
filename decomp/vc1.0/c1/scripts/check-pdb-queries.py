#!/usr/bin/env python3
"""Native-only PDB interface replay against committed original observations."""
import argparse,hashlib,importlib.util,json,subprocess,sys,tempfile,time
from pathlib import Path
BINARY=Path(__file__).resolve().parents[1]
CASE=BINARY/'cases/pdb-record-interface'
def module(name):
    spec=importlib.util.spec_from_file_location(name.replace('-','_'),Path(__file__).with_name(name+'.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--directory',type=Path);parser.add_argument('--exe',type=Path,default=BINARY/'build/pdb-tests-trace/C13232.EXE');args=parser.parse_args()
    start=time.perf_counter();m=json.loads((CASE/'manifest.json').read_text());config=json.loads((BINARY/'functions.json').read_text());runner=module('pdb-native');contract=module('pdb-query-contract')
    directory=args.directory or Path(tempfile.mkdtemp(prefix='pdb-query-check-',dir=Path.home()/'.ai-tmp/msvc2/vc1.0/c1/validation'))
    if args.directory:directory.mkdir(parents=True,exist_ok=False)
    reports=[]
    for row in m['direct_oracles']:
        unit=row['unit'];plan=m['direct_plan'][unit-6];assert plan==row['input'];original=CASE/row['directory']
        assert{str(p.relative_to(original)):runner.sha(p)for p in original.rglob('*')if p.is_file()}==row['files'],'cached oracle files changed'
        assert hashlib.sha256(json.dumps(m['phases']['context'],sort_keys=True).encode()).hexdigest()==row['phase_sha256']
        expected=row['accepted_functions'];assert config['accepted'][row['accepted_from']:row['accepted_count']]==expected
        reference=json.loads((original/'receipt.json').read_text());trace=reference['first_entry_projection'];assert trace['observed']==trace['expected']==expected
        assert trace['seeded_prefix_sha256']==hashlib.sha256(json.dumps(config['accepted'][:row['accepted_from']]).encode()).hexdigest()
        assert reference['terminal_exit']==99 and reference['original_sha256']==config['target']['sha256']and reference['pdb_unit']['unit']==unit
        target=directory/str(unit);receipt=runner.run(unit,target,args.exe,row['accepted_from']);assert receipt['exit']==99
        check=time.perf_counter();comparison=contract.validate(original,target,plan);assert(target/'entries').read_text().splitlines()==[n[1:].split('@')[0]for n in expected]
        (target/'comparison.json').write_text(json.dumps(comparison,indent=2)+'\n');reports.append(dict(unit=unit,status='pass',setup_seconds=receipt['setup_seconds'],execution_seconds=receipt['execution_seconds'],checks_seconds=time.perf_counter()-check))
    report=dict(status='pass',units=reports,total_seconds=time.perf_counter()-start,public_reachability=False,scope='Native i386 instrumented external units; cached original states/entries/streams/pending buffers/owned files, no Wine/debugger/reference refresh.')
    (directory/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print('PDB record interface:',len(reports),'passed;',report['total_seconds'],'seconds')
if __name__=='__main__':main()
