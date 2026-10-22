#!/usr/bin/env python3
"""Replay LINK's ordinary historical segments with frozen independent walkers."""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import shutil
import sys
import time
import compare as comparison

SHARED = Path(__file__).resolve().parents[4] / 'tooling/replay'
sys.path.insert(0, str(SHARED))
import fast_replay as fast

ROOT = Path(__file__).resolve().parents[1]
REGISTRY = ROOT / 'replay-cases.json'


def registry_segments(config):
    registry = json.loads(REGISTRY.read_text())
    entries = {entry['case']: dict(entry) for entry in registry['cases']}
    assert len(entries) == len(registry['cases']), 'duplicate replay case'
    history = config.get('frontier_cases', ['/?'] * len(config['accepted']))
    assert len(history) == len(config['accepted'])
    position = 0
    seen = set()
    for name, group in itertools.groupby(history):
        count = len(list(group))
        assert name in entries, 'unregistered historical case: ' + name
        assert name not in seen, 'noncontiguous acceptance segment: ' + name
        seen.add(name)
        entries[name].update(first=position, last=position + count)
        position += count
    for entry in entries.values():
        # Cases introducing no functions retain their concrete terminal/oracle
        # check, without pretending to have introduced a historical sequence.
        entry.setdefault('first', len(config['accepted']))
        entry.setdefault('last', len(config['accepted']))
        terminal = entry['terminal']
        entry['terminal_site'] = ('exit:%d' % terminal['code'] if terminal['kind'] == 'exit'
                                  else terminal['name'])
    return list(entries.values())


def frozen_paths(case):
    paths = set(ROOT.glob('src/*')) | set(ROOT.glob('scripts/*.py'))
    paths.update(ROOT / name for name in ('Makefile', 'functions.json', 'runtime.json',
                 'data-symbols.json', 'replay-cases.json', 'build/link.exe',
                 'build/link.map', 'build/runtime.json', 'build/debug-run.exe'))
    paths.update(comparison.wt.TC.glob('lib/*.LIB'))
    paths.update(comparison.wt.TC.glob('bin/LINK*'))
    paths.update(comparison.wt.TC.glob('bin/CVPACK*'))
    item = comparison.fixture(case)
    if item:
        fixture_root, phase = item
        paths.add(fixture_root / 'manifest.json')
        paths.update(fixture_root.glob('*.py'))
        paths.update(fixture_root / spec['path'] for spec in phase['inputs'] + phase['outputs'])
        for helper in ('../cl/src/gui-check.c', '../fixtures/hello-dll/dll-host.c',
                       '../fixtures/link-debug-dll/dll-host.c'):
            path = ROOT / helper
            if path.exists():
                paths.add(path)
    return sorted(path.resolve() for path in paths if path.is_file())


def final_oracles(case, labels, terminal, directory):
    assert terminal.startswith('exit:')
    code = int(terminal.split(':', 1)[1])
    for label in labels:
        Path('build/' + label + '.events').write_text('exit %d\n' % code)
    for stream in ('stdout', 'stderr'):
        left, right = [Path('build/' + label + '.' + stream).read_bytes() for label in labels]
        assert left == right, case + ' fast ' + stream + ' differs'
    # Retain an independent untouched-original check once at segment completion.
    events = comparison.run('fast-baseline', comparison.wt.TC / 'bin/LINK.EXE', case)
    assert events == 'exit %d\n' % code, case + ' untouched-original status differs'
    for stream in ('stdout', 'stderr'):
        expected = Path('build/fast-baseline.' + stream).read_bytes()
        assert expected == Path('build/' + labels[0] + '.' + stream).read_bytes(), case + ' baseline ' + stream + ' differs'
    comparison.check_artifacts(case, labels + ['fast-baseline'])
    for label in labels + ['fast-baseline']:
        for suffix in ('stdout', 'stderr', 'events'):
            shutil.copyfile(Path('build/' + label + '.' + suffix), directory / (label + '.' + suffix))


def continuous_segment(segment, directory, original, rebuilt, config, original_records, rebuilt_records):
    started = time.monotonic()
    case = segment['case']
    paths = frozen_paths(case)
    hashes = fast.freeze(paths)
    labels = []
    sides = []
    for label, image, records in [('original', original, original_records),
                                  ('reconstructed', rebuilt, rebuilt_records)]:
        side_directory = directory / label
        side_directory.mkdir(parents=True)
        seed = side_directory / 'seed.exe'
        comparison.mask(image, records, config['accepted'][:segment['first']], seed)
        work_label = 'fast-' + label + '-' + directory.name
        labels.append(work_label)
        setup = comparison.prepare_case_run(work_label, seed, case, namespace_label='fast-' + label)
        prefix = Path(setup['environment']['WINEPREFIX'])
        canonical = prefix.parent / 'root' / comparison.wt.TC.relative_to('/').as_posix() / 'bin/LINK.EXE'
        # setup copied this seed into its isolated canonical DOS namespace.
        # prepare_side writes the audited initial mask to that actual image.
        application = comparison.wt.windows(setup['executable'])
        command = '"' + application + '"'
        if setup['arguments']:
            command += ' ' + setup['arguments']
        side = fast.prepare_side(
            directory=side_directory, pe=image, records=records, accepted=config['accepted'],
            first=segment['first'], last=segment['last'], terminal=segment['terminal_site'],
            cwd=comparison.wt.windows(setup['cwd'] or ROOT), environment=None,
            wine_env=setup['environment'], mask_image=comparison.mask,
            image_path=canonical, application=application, command_line=command,
            stdout=ROOT / ('build/' + work_label + '.stdout'),
            stderr=ROOT / ('build/' + work_label + '.stderr'), frozen_paths=paths, timeout=90)
        env_hash = hashlib.sha256(json.dumps(setup['environment'], sort_keys=True).encode()).hexdigest()
        (side_directory / 'launch.json').write_text(json.dumps(dict(
            case=case, environment_sha256=env_hash, environment_mode='inherit prepared Wine launch environment',
            prefix=str(prefix), command_line=command, cwd=str(setup['cwd'] or ROOT)), indent=2) + '\n')
        sides.append(side)
    setup_seconds = time.monotonic() - started
    pair_started = time.monotonic()
    result = fast.run_pair(*sides, receipt_path=directory / 'pair.json')
    pair_call_seconds = time.monotonic() - pair_started
    oracle_started = time.monotonic()
    if segment['terminal_site'].startswith('exit:'):
        final_oracles(case, labels, segment['terminal_site'], directory)
    result['output_oracles_seconds'] = time.monotonic() - oracle_started
    fast.verify_frozen(hashes)
    result.update(setup_seconds=setup_seconds, fast_pair_seconds=result['seconds'],
                  pair_call_seconds=pair_call_seconds, adapter_seconds=time.monotonic() - started,
                  unsplit_pair_overhead_seconds=pair_call_seconds - result['seconds'])
    (directory / 'pair.json').write_text(json.dumps(result, indent=2) + '\n')
    return result


def fresh_segment(segment, directory, config):
    directory.mkdir(parents=True, exist_ok=True)
    hashes = fast.freeze(frozen_paths(segment['case']))
    started = time.monotonic()
    sites = []
    for count in range(segment['first'], segment['last']):
        expected = config['accepted'][count]
        actual = comparison.compare(count, segment['case'], baseline=False)
        assert actual == expected, (count, expected, actual)
        sites.append(actual)
    history_seconds = time.monotonic() - started
    terminal_started = time.monotonic()
    terminal = comparison.compare(segment['last'], segment['case'])
    assert terminal == segment['terminal_site'], (segment['case'], segment['terminal_site'], terminal)
    fast.verify_frozen(hashes)
    result = dict(method='fresh', case=segment['case'], first=segment['first'],
                  last=segment['last'], expected=sites, terminal=terminal,
                  seconds=time.monotonic() - started, history_seconds=history_seconds,
                  terminal_and_oracles_seconds=time.monotonic() - terminal_started, frozen=hashes)
    (directory / 'receipt.json').write_text(json.dumps(result, indent=2) + '\n')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', help='one registered case; otherwise all ordinary cases')
    parser.add_argument('--fresh', action='store_true', help='explicit full fresh historical fallback')
    parser.add_argument('--include-specialized', action='store_true', help='batch deferred process-tree histories at a checkpoint')
    parser.add_argument('--verify-fresh', action='store_true', help='compare every migrated segment with its full fresh transcript')
    parser.add_argument('--stop-at', help='continuous/fresh rerun leaving this logical entry masked')
    parser.add_argument('--fresh-frontier', type=int, help='focused fresh rerun at one absolute acceptance prefix')
    parser.add_argument('--final-case', help='retain the legacy full-history final comparator')
    parser.add_argument('--require-exit', action='store_true')
    parser.add_argument('--root', type=Path, help='new evidence directory')
    args = parser.parse_args()
    os.chdir(ROOT)
    original, rebuilt, config, original_records, rebuilt_records = comparison.indexes()
    segments = registry_segments(config)
    if args.case:
        segments = [segment for segment in segments if segment['case'] == args.case]
        assert segments, 'unregistered replay case: ' + args.case
    if args.case and segments[0]['mode'] != 'fast':
        args.include_specialized = True  # An explicit special-case request runs its existing runner.
    if args.fresh_frontier is not None:
        assert args.case and len(segments) == 1
        segment = segments[0]
        assert segment['first'] <= args.fresh_frontier <= segment['last']
        actual = comparison.compare(args.fresh_frontier, args.case, baseline=False)
        expected = (config['accepted'][args.fresh_frontier] if args.fresh_frontier < segment['last']
                    else segment['terminal_site'])
        assert actual == expected, (args.fresh_frontier, expected, actual)
        return
    if args.stop_at:
        assert args.case and len(segments) == 1, '--stop-at requires one case'
        segment = segments[0]
        if args.stop_at != segment['terminal_site']:
            position = config['accepted'].index(args.stop_at)
            assert segment['first'] <= position < segment['last'], 'stop is outside case segment'
            segment['last'] = position
            segment['terminal_site'] = args.stop_at
    directory = args.root or comparison.wt.PREFIX.parent / ('fast-history-%d-%d' % (os.getpid(), time.time_ns()))
    directory = directory.resolve()
    directory.mkdir(parents=True, exist_ok=False)
    paths = set()
    for segment in segments:
        paths.update(frozen_paths(segment['case']))
    paths.update([SHARED / 'fast_replay.py', SHARED / 'src/walker.c', SHARED / 'Makefile',
                  SHARED / 'scripts/build.py'])
    build_started = time.monotonic()
    if not args.fresh:
        paths.add(fast.build_helper())
    build_seconds = time.monotonic() - build_started
    freeze_started = time.monotonic()
    batch_frozen = fast.freeze(paths)
    freeze_seconds = time.monotonic() - freeze_started
    report = dict(root=str(directory), accepted=len(config['accepted']), segments=[], deferred=[], frozen=batch_frozen,
                  shared_build_check_seconds=build_seconds, batch_freeze_seconds=freeze_seconds,
                  timing_notes='Wall clock with other workers active. Shared build check may be up to date. '
                               'Fast pair excludes external setup and output validators. Unsplit pair overhead includes '
                               'helper build check, receipt serialization and host wrapper overhead. Fresh terminal call '
                               'includes paired completion, untouched-original baseline and final validators; not further split.')
    started = time.monotonic()
    for index, segment in enumerate(segments):
        fast.verify_frozen(batch_frozen)
        if segment['mode'] != 'fast' and not (args.fresh or args.include_specialized):
            report['deferred'].append(segment)
            print('Deferred specialized case: ' + segment['case'], flush=True)
            continue
        child = directory / ('%02d-' % index + segment['case'].replace('/?', 'help'))
        child.mkdir()
        fixture_started = time.monotonic()
        print('Replay %s [%d,%d) %s' % (segment['case'], segment['first'], segment['last'], segment['mode']), flush=True)
        try:
            if args.fresh or segment['mode'] != 'fast':
                result = fresh_segment(segment, child / 'fresh', config)
            else:
                result = continuous_segment(segment, child / 'continuous', original, rebuilt, config,
                                            original_records, rebuilt_records)
                if args.verify_fresh:
                    fresh = fresh_segment(segment, child / 'fresh', config)
                    assert result['sides'][0]['expected'] == result['sides'][1]['expected'] == fresh['expected']
                    assert result['terminal'] == fresh['terminal']
                    result['fresh_seconds'] = fresh['seconds']
                    result['fresh'] = fresh
        except Exception as error:
            failures = []
            for path in child.glob('continuous/*/events.jsonl'):
                events = [json.loads(line) for line in path.read_text().splitlines()]
                for event in events:
                    if event.get('event') != 'failure':
                        continue
                    if 'address' in event and 'base' in event:
                        image, records = ((original, original_records) if path.parent.name == 'original'
                                          else (rebuilt, rebuilt_records))
                        event['logical_site'] = comparison.logical(
                            'exception %08x rva %08x' % (event.get('exception', 0), event['address'] - event['base']),
                            image, records)
                    failures.append(event)
            offset = failures[0].get('local_index', failures[0].get('index', 0)) if failures else 0
            count = min(segment['first'] + offset, segment['last'])
            expected = config['accepted'][count] if count < segment['last'] else segment['terminal_site']
            command = 'python3 scripts/replay-frontiers.py --case ' + repr(segment['case'])
            failure = dict(case=segment['case'], message=str(error), frontier=count,
                           expected=expected, evidence=str(child), native_failures=failures,
                           fresh_rerun=command + ' --fresh-frontier ' + str(count),
                           continuous_rerun=command + ('' if expected.startswith('exit:') else ' --stop-at ' + repr(expected)))
            (directory / 'failure.json').write_text(json.dumps(failure, indent=2) + '\n')
            print('Failure evidence: ' + str(directory / 'failure.json'), flush=True)
            raise
        fast.verify_frozen(batch_frozen)
        report['segments'].append(dict(case=segment['case'], first=segment['first'], last=segment['last'],
                                       wall_seconds=time.monotonic() - fixture_started, result=result))
    if args.final_case:
        report['final_site'] = comparison.compare(case=args.final_case)
        if args.require_exit:
            assert report['final_site'].startswith('exit:'), 'case incomplete: ' + report['final_site']
    fast.verify_frozen(batch_frozen)
    report['seconds'] = time.monotonic() - started
    (directory / 'receipt.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Replay receipt: ' + str(directory / 'receipt.json'), flush=True)


if __name__ == '__main__':
    main()
