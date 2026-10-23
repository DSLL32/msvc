#!/usr/bin/env python3
"""Cached external expression units; no Wine, debugger or public-path claim."""
import argparse
import importlib.util
import json
from pathlib import Path
import tempfile
import time

BINARY = Path(__file__).resolve().parents[1]
CASE = BINARY/'cases/expression-templates'


def module(name):
    spec = importlib.util.spec_from_file_location(name.replace('-', '_'),
        Path(__file__).with_name(name+'.py'))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def validate_reference(manifest, runner, contract):
    summaries = []
    for unit, oracle in enumerate(manifest['direct_oracles']):
        assert oracle['unit'] == unit and len(oracle['baselines']) == 2
        for reference in oracle['baselines']:
            directory = CASE/reference['directory']
            actual = {str(p.relative_to(directory)):runner.sha(p)
                for p in directory.rglob('*') if p.is_file()}
            assert actual == reference['files'], 'cached direct original file set/hash differs'
            row = contract.record((directory/'events').read_text())
            assert row['unit'] == unit
            assert contract.returned(row) == oracle['contract'], 'cached original contract differs'
        summaries.append(oracle['contract'])
    return summaries


def validate_actual(directory, receipt, phase, expected, contract, artifacts):
    artifacts.validate_result('expression-direct',receipt['exit'],
        (directory/'stdout').read_bytes(),(directory/'stderr').read_bytes(),phase)
    work = Path(receipt['work'])
    for item in phase['inputs']:
        assert (work/Path(item['path']).name).read_bytes() == (CASE/item['path']).read_bytes(), 'context input modified'
    outputs = artifacts.validate_artifacts(work,CASE,phase)
    observed = contract.returned(contract.record((directory/'report').read_text()))
    assert observed == expected, 'direct result differs from twice-original contract'
    return dict(contract=observed,outputs=outputs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory',type=Path)
    parser.add_argument('--exe',type=Path,default=BINARY/'build/expression-tests/C13232.EXE')
    options = parser.parse_args()
    started = time.perf_counter()
    runner,contract,artifacts = (module(n) for n in
        ['expression-native','expression-contract','check-native'])
    manifest = json.loads((CASE/'manifest.json').read_text())
    executable = options.exe.resolve()
    header = executable.read_bytes()[:20]
    assert header[:5] == b'\x7fELF\x01' and header[18:20] == b'\x03\x00', 'native i386 ELF required'
    expected = validate_reference(manifest,runner,contract)
    reference_seconds = time.perf_counter()-started
    root = Path.home()/'.ai-tmp/msvc2/vc1.0/c1/validation'
    directory = options.directory or Path(tempfile.mkdtemp(prefix='expression-check-',dir=root))
    if options.directory: directory.mkdir(parents=True,exist_ok=False)
    reports = []
    for unit,oracle in enumerate(manifest['direct_oracles']):
        target = directory/('%02d'%unit)
        receipt = runner.run(unit,target,executable)
        began = time.perf_counter()
        result = validate_actual(target,receipt,manifest['phases']['context'],
            expected[unit],contract,artifacts)
        reports.append(dict(unit=unit,name=oracle['name'],status='pass',
            setup_seconds=receipt['setup_seconds'],execution_seconds=receipt['execution_seconds'],
            oracle_seconds=time.perf_counter()-began,**result))
    receipt = dict(status='pass',public_reachability=False,direct_units=reports,
        cached_reference_seconds=reference_seconds,elapsed_seconds=time.perf_counter()-started,
        executable_sha256=runner.sha(executable),manifest_sha256=runner.sha(CASE/'manifest.json'),
        execution_policy='Native external guarded-stack harness; cached originals only. No Wine, debugger or reference refresh.',
        directory=str(directory))
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('Direct expression support: %d passed %.6fs; no public source reachability claim' % (len(reports),receipt['elapsed_seconds']))


if __name__ == '__main__':
    main()
