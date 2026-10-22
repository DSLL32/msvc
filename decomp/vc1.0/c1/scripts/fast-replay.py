#!/usr/bin/env python3
"""Continuous historical C1 replay; fresh acceptance stays in compare.py."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[4] / 'tooling/replay'))
import fast_replay as shared
import compare as comparison

CASE_ORDER = ('minimal', 'hello-win32', 'hello-dll', 'numeric-varargs',
              'pch-create', 'pch-use')
LABELS = ('fast-original', 'fast-reconstructed')


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def frozen_inputs():
    paths = [Path('Makefile'), Path('functions.json'), Path('runtime.json'),
             Path('build/c1.exe'), Path('build/c1.map'),
             Path('build/runtime.json'), Path('build/debug-run.exe'),
             comparison.wt.TC / 'bin/C13232.EXE',
             comparison.wt.TC / 'bin/C1.ERR',
             comparison.FIXTURE / 'manifest.json']
    paths.extend(Path('src').glob('*'))
    paths.extend(Path('scripts').glob('*.py'))
    for row in comparison.CASE['inputs'] + comparison.CASE['outputs']:
        path = comparison.FIXTURE / row['path']
        assert sha256(path) == row['sha256'], 'fixture changed: ' + str(path)
        paths.append(path)
    for row in comparison.CASE.get('toolchain_dependencies', []):
        path = comparison.wt.ROOT / row['path']
        assert sha256(path) == row['sha256'], 'toolchain dependency changed: ' + str(path)
        paths.append(path)
    assert all(p.is_file() for p in paths), 'missing frozen replay input'
    return paths


def replay_case(name, *, dependencies=None, stop_before=None):
    started = time.monotonic()
    comparison.select_case(name)
    original, reconstructed, config, original_records, reconstructed_records = comparison.indexes()
    case = config['cases'][name]
    assert case['arguments'] == '' and comparison.CASE['arguments'] == []
    first, last = case['accepted_from'], case['accepted_count']
    terminal = case.get('terminal', 'exit:' + str(comparison.CASE['expected_exit']))
    if stop_before is not None:
        assert first <= stop_before < last, 'stop index is outside this case segment'
        last = stop_before
        terminal = config['accepted'][last]
    root = Path('build/fast-replay') / name
    if root.exists():
        shutil.rmtree(root)
    root.mkdir(parents=True)
    (root / 'adapter.json').write_text(json.dumps(dict(case=name, first=first, last=last, terminal=terminal, fresh_fallback='python3 scripts/compare.py --fixture %s --frontier INDEX' % name, continuous_fallback='python3 scripts/fast-replay.py --fixture %s --stop-before INDEX' % name), indent=2) + '\n')
    frozen = frozen_inputs()
    sides = []
    for label, pe, records in ((LABELS[0], original, original_records),
                               (LABELS[1], reconstructed, reconstructed_records)):
        stem, work, environment, cwd, flags = comparison.setup_run(label, dependencies)
        extra = list(work.iterdir())
        for path in (dependencies or {}).get(label, {}).values():
            extra.append(Path(path))
        side = shared.prepare_side(
            directory=root / label, pe=pe, records=records,
            accepted=config['accepted'], first=first, last=last,
            terminal=terminal, cwd=cwd,
            environment={'MSC_CMD_FLAGS': flags}, wine_env=environment,
            mask_image=comparison.mask, frozen_paths=frozen + extra,
            stdout=Path(str(stem) + '.stdout'),
            stderr=Path(str(stem) + '.stderr'), timeout=90)
        sides.append(side)
    setup_seconds = time.monotonic() - started
    pair_started = time.monotonic()
    receipt = shared.run_pair(*sides, receipt_path=root / 'paired-receipt.json')
    pair_call_seconds = time.monotonic() - pair_started
    validation_started = time.monotonic()
    if terminal.startswith('exit:'):
        for side in sides:
            for stream in ('stdout', 'stderr'):
                data = getattr(side, stream).read_bytes()
                expected = comparison.CASE.get('expected_' + stream, '').encode('utf-8')
                assert data == expected, name + ' captured ' + stream + ' differs'
        comparison.check_artifacts(LABELS)
    validation_seconds = time.monotonic() - validation_started
    receipt.update(case=name, first=first, last=last, terminal=terminal,
                   setup_seconds=setup_seconds, pair_call_seconds=pair_call_seconds,
                   walker_pair_seconds=receipt['seconds'],
                   output_validation_seconds=validation_seconds,
                   timing_scope='Setup includes fixture validation/copy, namespaces, indexes, masked images and plans. Pair call includes shared helper build/cache checks and paired execution. Case wall time excludes final adapter receipt serialization and printing.',
                   end_to_end=terminal.startswith('exit:'),
                   setup_and_run_seconds=time.monotonic() - started,
                   generated_dependency=bool(dependencies),
                   fresh_fallback='python3 scripts/compare.py --fixture %s --frontier INDEX' % name,
                   continuous_fallback='python3 scripts/fast-replay.py --fixture %s --stop-before INDEX' % name)
    (root / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('%s: %d frontiers, %s, %.3fs' %
          (name, last-first, terminal, receipt['setup_and_run_seconds']), flush=True)
    return receipt


def replay_cases(name, *, stop_before=None):
    if name == 'all':
        assert stop_before is None, '--stop-before requires a single fixture'
        names = CASE_ORDER
    elif name == 'pch-use':
        names = ('pch-create', 'pch-use')
    else:
        names = (name,)
    receipts = []
    for current in names:
        dependencies = None
        if current == 'pch-use':
            assert receipts and receipts[-1]['case'] == 'pch-create'
            dependencies = {label: {'state.pch': Path('build') /
                                   ('pch-create-' + label) / 'state.pch'}
                            for label in LABELS}
        receipts.append(replay_case(current, dependencies=dependencies,
                                   stop_before=stop_before if current == name else None))
    report = dict(method='continuous', cases=[r['case'] for r in receipts],
                  frontiers=sum(r['last']-r['first'] for r in receipts),
                  seconds=sum(r['setup_and_run_seconds'] for r in receipts),
                  receipts=receipts)
    Path('build/fast-replay/receipt.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', choices=(*CASE_ORDER, 'all'), default='all')
    parser.add_argument('--stop-before', type=int,
                        help='stop at the still-masked global frontier index')
    args = parser.parse_args()
    replay_cases(args.fixture, stop_before=args.stop_before)
