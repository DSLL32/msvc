#!/usr/bin/env python3
"""Cached native PDB method regression; never invokes Wine or a debugger."""
import argparse
import importlib.util
import json
from pathlib import Path
import tempfile
import time

BINARY=Path(__file__).resolve().parents[1]
CASE=BINARY/'cases/legacy-pdb-object'


def module(name):
    spec=importlib.util.spec_from_file_location(name.replace('-','_'),Path(__file__).with_name(name+'.py'))
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result)
    return result


def canonical(value):
    return json.loads(json.dumps(value))


def validate_reference(manifest,runner,contract):
    expected=[]
    for unit,oracle in enumerate(manifest['direct_method_oracles']):
        assert oracle['unit']==unit and len(oracle['baselines'])==2
        for reference in oracle['baselines']:
            directory=CASE/reference['directory']
            actual={str(path.relative_to(directory)):runner.sha(path)for path in directory.rglob('*')if path.is_file()}
            assert actual==reference['files'],'original cache file set/hash changed'
            assert canonical(contract.record((directory/'events').read_text(),unit))==oracle['contract']
            receipt=json.loads((directory/'receipt.json').read_text())
            assert receipt['terminal_exit']==99 and receipt['files']==oracle['files']
            assert (directory/'stdout').read_bytes()==bytes.fromhex(oracle['expected_stdout'])
            assert (directory/'stderr').read_bytes()==bytes.fromhex(oracle['expected_stderr'])
        expected.append(oracle['contract'])
    return expected


def validate_actual(directory,receipt,oracle,contract,runner):
    assert receipt['exit']==oracle['expected_exit']==99,'native external unit failed'
    assert (directory/'stdout').read_bytes()==bytes.fromhex(oracle['expected_stdout'])
    assert (directory/'stderr').read_bytes()==bytes.fromhex(oracle['expected_stderr'])
    work=Path(receipt['work'])
    actual=[dict(name=path.name,bytes=path.stat().st_size,sha256=runner.sha(path))
            for path in sorted(work.iterdir())if path.is_file()]
    assert all(path.is_file()and not path.is_symlink()for path in work.iterdir())
    assert actual==oracle['files'],'native output file set/bytes differ'
    result=canonical(contract.record((directory/'report').read_text(),oracle['unit'],True))
    assert result==oracle['contract'],'native method state differs'
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory',type=Path)
    parser.add_argument('--exe',type=Path,default=BINARY/'build/pdb-tests/C13232.EXE')
    options=parser.parse_args();start=time.perf_counter()
    runner,contract=module('pdb-native'),module('pdb-object-contract')
    manifest=json.loads((CASE/'manifest.json').read_text())
    validate_reference(manifest,runner,contract);reference_seconds=time.perf_counter()-start
    scratch=Path.home()/'.ai-tmp/msvc2/vc1.0/c1/validation'
    directory=options.directory or Path(tempfile.mkdtemp(prefix='pdb-object-check-',dir=scratch))
    if options.directory:directory.mkdir(parents=True,exist_ok=False)
    reports=[]
    for oracle in manifest['direct_method_oracles']:
        target=directory/str(oracle['unit'])
        receipt=runner.run(oracle['unit'],target,options.exe)
        began=time.perf_counter();validate_actual(target,receipt,oracle,contract,runner)
        reports.append(dict(unit=oracle['unit'],name=oracle['name'],status='pass',
            setup_seconds=receipt['setup_seconds'],execution_seconds=receipt['execution_seconds'],
            checks_seconds=time.perf_counter()-began))
    report=dict(status='pass',units=reports,reference_seconds=reference_seconds,
        total_seconds=time.perf_counter()-start,executable_sha256=runner.sha(options.exe),
        scope='Six direct external method units; no established public trigger. Native i386 only; no Wine, debugger, or cache refresh.')
    (directory/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':
    main()
