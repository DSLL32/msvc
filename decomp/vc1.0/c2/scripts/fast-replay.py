"""Continuous historical replay adapter; fresh reconstruction stays in compare.py."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import time

import compare as comparison

sys.path.insert(0, str(Path(__file__).resolve().parents[4] / 'tooling/replay'))
import fast_replay as shared


def run(profile, *, stop_at=None, receipt_path=None):
    started = time.monotonic()
    comparison.select_profile(profile)
    original, rebuilt, config, original_records, rebuilt_records = comparison.indexes()
    case = config['cases'][profile]
    first = case.get('accepted_from', 0)
    last = case.get('accepted_count', len(config['accepted']))
    segment_last = last
    if stop_at is not None:
        if not first <= stop_at <= last:
            raise ValueError('stop-at must be within the case acceptance segment')
        last = stop_at

    root = comparison.wt.PREFIX.parent / ('fast-replay-' + profile + '-' + str(os.getpid()))
    root.mkdir()
    frozen_paths = [Path('functions.json'), Path('runtime.json'), Path('Makefile'),
                    Path('build/c2.exe'), Path('build/c2.map'), Path('build/runtime.json'),
                    Path('build/debug-run.exe'), comparison.wt.TC / 'bin/C23232.EXE',
                    Path(case['fixture_manifest'])]
    frozen_paths += list(Path('scripts').glob('*.py')) + list(Path('src').iterdir())
    inputs = []
    for source in comparison.PHASE['inputs']:
        path = comparison.FIXTURES / source['path']
        if hashlib.sha256(path.read_bytes()).hexdigest() != source['sha256']:
            raise RuntimeError('fixture changed: ' + str(path))
        inputs.append(path)
    frozen_paths += inputs
    frozen_paths += [comparison.FIXTURES / output['path']
                     for output in comparison.PHASE['outputs']]
    shared_paths = [shared.HERE / 'fast_replay.py', shared.HERE / 'src/walker.c',
                    shared.HERE / 'Makefile', shared.HERE / 'scripts/build.py']
    shared_frozen = shared.freeze(shared_paths)
    frozen_paths += shared_paths
    frozen = shared.freeze(frozen_paths)
    timings = {'initial_setup_seconds': time.monotonic() - started}
    phase_started = time.monotonic()

    # An independent fresh pair supplies the exact current terminal and keeps
    # the established untouched-original/artifact oracle for completed cases.
    # No acceptance metadata is inferred or updated from a continuous run.
    terminal = comparison.compare(last, case=case['arguments'])
    fresh_streams = {}
    for stream in ('stdout', 'stderr'):
        left = Path('build/original.' + stream).read_bytes()
        right = Path('build/reconstructed.' + stream).read_bytes()
        if left != right:
            raise RuntimeError('fresh terminal streams differ: ' + stream)
        fresh_streams[stream] = left
    if stop_at is not None and last < segment_last:
        if terminal != config['accepted'][last]:
            raise RuntimeError('fresh stop-at frontier differs from acceptance history')
    shared.verify_frozen(frozen)
    timings['fresh_final_oracle_seconds'] = time.monotonic() - phase_started
    phase_started = time.monotonic()
    child_environment = {'MSC_CMD_FLAGS': comparison.PHASE['environment']['MSC_CMD_FLAGS'].format(
        bin=comparison.wt.windows(comparison.wt.TC / 'bin'))}
    sides = []
    directories, streams = {}, {}
    input_copy_seconds = 0
    for label, executable, records in (('original', original, original_records),
                                       ('reconstructed', rebuilt, rebuilt_records)):
        directory = root / label
        work = directory / 'work'
        work.mkdir(parents=True)
        copy_started = time.monotonic()
        for path in inputs:
            shutil.copyfile(path, work / path.name)
        input_copy_seconds += time.monotonic() - copy_started
        side = shared.prepare_side(
            directory=directory, pe=executable, records=records,
            accepted=config['accepted'], first=first, last=last, terminal=terminal,
            cwd=comparison.wt.windows(work), environment=child_environment,
            wine_env=comparison.wt.environment(), mask_image=comparison.mask,
            frozen_paths=frozen_paths + list(work.iterdir()), timeout=90)
        sides.append(side)
        directories[label] = work
        streams[label] = dict(stdout=side.stdout, stderr=side.stderr)
    timings['plan_setup_seconds'] = time.monotonic() - phase_started
    timings['fixture_copy_seconds_within_plan_setup'] = input_copy_seconds
    phase_started = time.monotonic()
    report = shared.run_pair(*sides, receipt_path=root / 'pair.json')
    timings['helper_build_or_check_seconds'] = max(0, time.monotonic() - phase_started - report['seconds'])
    timings['fast_pair_seconds'] = report['seconds']
    phase_started = time.monotonic()
    for label in streams:
        for stream in ('stdout', 'stderr'):
            if streams[label][stream].read_bytes() != fresh_streams[stream]:
                raise RuntimeError('continuous stream differs from fresh terminal: ' + label + ' ' + stream)
    if terminal == 'exit:0':
        report['artifacts'] = comparison.check_artifacts(
            list(directories), directories=directories, streams=streams,
            receipt_path=root / 'artifacts.json')
    shared.verify_frozen(frozen)
    timings['output_validation_and_final_freeze_seconds'] = time.monotonic() - phase_started
    timings['target_build_seconds'] = None
    timings['prefix_initialization_seconds'] = None
    timings['notes'] = ('Existing target build and warm private prefix reused; target build and prefix '
                        'initialization not run. Fresh final oracle includes a fresh pair, untouched '
                        'original baseline and object checks when complete; that internal split is not measured. '
                        'Helper timing includes make freshness check and shared-call overhead. '
                        'Fixture copying is a subset of plan setup, not an additional phase. '
                        'Total excludes Python startup/imports and writing the final summary receipt.')
    shared_frozen.update(shared.freeze([shared.HERE / 'build/walker.exe']))
    shared_identity = hashlib.sha256(json.dumps(shared_frozen, sort_keys=True).encode()).hexdigest()
    report.update(profile=profile, fresh_terminal=terminal, frozen=frozen,
                  shared_hashes=shared_frozen, shared_identity_sha256=shared_identity,
                  timings=timings, total_seconds=time.monotonic()-started)
    path = Path(receipt_path) if receipt_path else root / 'receipt.json'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('Fast replay %s: %d frontiers, terminal %s, %.3fs (%.3fs including fresh oracle)' %
          (profile, report['frontiers'], terminal, report['seconds'], report['total_seconds']), flush=True)
    print('Replay receipt: ' + str(path), flush=True)
    return report
