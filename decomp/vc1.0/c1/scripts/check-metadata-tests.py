#!/usr/bin/env python3
"""Native-only replay of ten frozen PCH metadata interface units."""
import argparse,hashlib,importlib.util,json,tempfile,time
from pathlib import Path
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/pch-metadata-interface'
def module(name):
    spec=importlib.util.spec_from_file_location(name.replace('-','_'),Path(__file__).with_name(name+'.py'));value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value);return value
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path);p.add_argument('--exe',type=Path,default=BINARY/'build/metadata-tests-trace/C13232.EXE');args=p.parse_args();start=time.perf_counter();runner=module('metadata-unit-native');contract=module('metadata-contract');m=json.loads((CASE/'manifest.json').read_text());config=json.loads((BINARY/'functions.json').read_text());reports=[]
    directory=args.directory or Path(tempfile.mkdtemp(prefix='metadata-check-',dir=Path.home()/'.ai-tmp/msvc2/vc1.0/c1/validation'))
    if args.directory:directory.mkdir(parents=True,exist_ok=False)
    assert [row['unit']for row in m['direct_oracles']]==list(range(10))
    assert len(m['direct_plan'])==10 and config['direct_cases']['pch-metadata-interface']['units']==10
    for row in m['direct_oracles']:
        unit=row['unit'];plan=m['direct_plan'][unit];original=CASE/row['directory'];assert row['input']==plan
        assert{str(f.relative_to(original)):runner.context.sha(f)for f in original.rglob('*')if f.is_file()}==row['files'],'cached oracle changed'
        phase=m['phases'][plan['phase']];assert hashlib.sha256(json.dumps(phase,sort_keys=True).encode()).hexdigest()==row['phase_sha256']
        expected=row['accepted_functions'];assert config['accepted'][row['accepted_from']:row['accepted_count']]==expected
        reference=json.loads((original/'receipt.json').read_text());trace=reference['first_entry_projection'];assert trace['expected']==trace['observed']==expected
        assert trace['seeded_prefix_sha256']==hashlib.sha256(json.dumps(config['accepted'][:row['accepted_from']]).encode()).hexdigest()
        assert reference['original_sha256']==config['target']['sha256']and reference['terminal_exit']==phase['expected_exit']and reference['metadata_unit']==unit
        assert reference['child_arguments']==[]and reference['child_working_directory']==phase['canonical_working_directory']and reference['child_stdio']==phase['stdio']
        expected_flags=phase['environment']['MSC_CMD_FLAGS'].format(bin=runner.context.windows(BINARY.parents[2]/'toolchains/1.0-nt/bin'),include=runner.context.windows(BINARY.parents[2]/'toolchains/1.0-nt/include'))
        assert reference['child_environment']=={'MSC_CMD_FLAGS':expected_flags}and reference['direct_call']['function']==plan['function']and reference['direct_call']['arguments']==plan['request_arguments']
        target=directory/str(unit);receipt=runner.run(unit,target,args.exe,seen_count=row['accepted_from']);assert receipt['exit']==(phase['expected_exit']&255)
        began=time.perf_counter();comparison=contract.validate(original,target,plan);assert(target/'entries').read_text().splitlines()==[n[1:].split('@')[0]for n in expected]
        (target/'comparison.json').write_text(json.dumps(comparison,indent=2)+'\n');reports.append(dict(unit=unit,status='pass',setup_seconds=receipt['setup_seconds'],execution_seconds=receipt['execution_seconds'],checks_seconds=time.perf_counter()-began))
    result=dict(status='pass',units=reports,total_seconds=time.perf_counter()-start,public_reachability=False,scope='Ten fixed native i386 units over genuine parsed argument/symbol/vector/browser owners; cached original ABI/entries/states/streams/files/full terminal status; no Wine/debugger/original refresh.')
    (directory/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');print('Direct metadata support:',len(reports),'passed;',result['total_seconds'],'seconds')
if __name__=='__main__':main()
