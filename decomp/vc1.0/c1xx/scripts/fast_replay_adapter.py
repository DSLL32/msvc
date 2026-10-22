"""C1XX ordinary-case adapter for the shared continuous frontier walker.

This checks first encounters and ordinary final I/O. The existing comparator
retains its fourteen runtime observations, pending buffers and caller checks.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

import compare as comparison

ROOT = Path(__file__).resolve().parents[1]
SHARED = Path(__file__).resolve().parents[4] / 'tooling/replay'
sys.path.insert(0, str(SHARED))
import fast_replay as shared


def frozen_inputs():
    paths = []
    for folder in (ROOT / 'src', ROOT / 'scripts'):
        paths.extend(p for p in folder.rglob('*') if p.is_file() and
                     '__pycache__' not in p.parts and 'build' not in p.parts)
    paths.extend(p for p in SHARED.rglob('*') if p.is_file() and
                 '__pycache__' not in p.parts and 'build' not in p.parts and
                 (p.suffix in ('.py', '.c', '.h', '.inc', '.def', '.sh') or p.name == 'Makefile'))
    paths.extend(ROOT / p for p in (
        'Makefile', 'functions.json', 'runtime.json', 'build/c1xx.exe',
        'build/c1xx.map', 'build/runtime.json', 'build/debug-run.exe',
        'build/parser-grammar.c', 'build/lookahead-grammar.c',
        '../fixtures/cpp-basic/manifest.json', '../fixtures/cpp-basic/c1/cpp-basic.cpp'))
    config = json.loads((ROOT / 'functions.json').read_text())
    paths.extend(ROOT / case['terminal_evidence'] for case in config['cases'].values()
                 if case.get('terminal_evidence'))
    paths.extend((comparison.wt.TC / 'bin/C1XX3232.EXE',
                  comparison.wt.TC / 'bin/C1.ERR'))
    return paths


def segment(config, arguments, stop_at=None):
    case = next(c for c in config['cases'].values() if c['arguments'] == arguments)
    first = case.get('accepted_from', 0)
    last = case.get('accepted_count', len(config['accepted']))
    terminal = case['terminal']
    if stop_at is not None:
        if not first <= stop_at <= last:
            raise ValueError('stop index outside this case segment')
        if stop_at < last:
            terminal = config['accepted'][stop_at]
        last = stop_at
    return first, last, terminal


def prepare_pair(root, arguments='', stop_at=None):
    original, rebuilt, config, original_records, rebuilt_records = comparison.indexes()
    first, last, terminal = segment(config, arguments, stop_at)
    flags = comparison.CASE['environment']['MSC_CMD_FLAGS'].format(
        bin=comparison.wt.windows(comparison.wt.TC / 'bin'),
        include=comparison.wt.windows(comparison.wt.TC / 'include'))
    command = comparison.CASE['command_line_template'].format(
        bin=comparison.wt.windows(comparison.wt.TC / 'bin'))
    if arguments:
        command += ' ' + arguments
    sides = []
    for label, image, records in (
            ('original', original, original_records),
            ('reconstructed', rebuilt, rebuilt_records)):
        work = root / ('minimal-' + label)
        work.mkdir(parents=True)
        shutil.copyfile(comparison.FIXTURE / 'c1/cpp-basic.cpp', work / 'cpp-basic.cpp')
        sides.append(shared.prepare_side(
            directory=root / label, pe=image, records=records,
            accepted=config['accepted'], first=first, last=last, terminal=terminal,
            cwd=shared.windows(work), environment={'MSC_CMD_FLAGS': flags},
            wine_env=comparison.wt.environment(), mask_image=comparison.mask,
            command_line=command, stdout=root / (label + '.stdout'),
            stderr=root / (label + '.stderr'),
            frozen_paths=frozen_inputs() + [work / 'cpp-basic.cpp']))
    return sides, first, last, terminal


def validate_outputs(root, terminal):
    """Reuse the ordinary oracle, without live-state reads at fast stops."""
    left, right = [root / ('minimal-' + side) for side in ('original', 'reconstructed')]
    names = {p.name for p in left.iterdir()}
    if names != {p.name for p in right.iterdir()}:
        raise AssertionError('fast terminal file sets differ')
    hashes = {}
    for name in sorted(names):
        data = (left / name).read_bytes()
        if data != (right / name).read_bytes():
            raise AssertionError('fast terminal file differs: ' + name)
        hashes[name] = hashlib.sha256(data).hexdigest()
    for stream in ('stdout', 'stderr'):
        data = (root / ('original.' + stream)).read_bytes()
        if data != (root / ('reconstructed.' + stream)).read_bytes():
            raise AssertionError('fast terminal ' + stream + ' differs')
        hashes[stream] = hashlib.sha256(data).hexdigest()
        if terminal.startswith('exit:') and data != comparison.CASE['expected_' + stream].encode():
            raise AssertionError('captured frontend ' + stream + ' differs')
    if terminal.startswith('exit:'):
        if terminal != 'exit:' + str(comparison.CASE['expected_exit']):
            raise AssertionError('captured frontend exit differs')
        comparison.check_artifacts(['original', 'reconstructed'], root)
    return hashes


def fresh_stop(root, arguments, count):
    config = json.loads((ROOT / 'functions.json').read_text())
    _, _, expected = segment(config, arguments, count)
    actual = comparison.compare(count, arguments, baseline=False, workspace=root)
    if actual != expected:
        raise AssertionError('fresh stop differs: ' + actual + ' != ' + expected)
    return dict(method='fresh-stop', index=count, terminal=actual,
                full_state_and_buffers=True, directory=str(root))


def main():
    parser = argparse.ArgumentParser(description=__doc__, epilog=
        'Use replay-frontiers.py --fresh for the complete fresh full-oracle history.')
    parser.add_argument('--case', default='')
    parser.add_argument('--jobs', type=int, default=4, help='fresh verification parallelism')
    parser.add_argument('--verify', action='store_true', help='compare full fresh and continuous histories')
    parser.add_argument('--stop-at', type=int, help='continuous rerun ending at absolute frontier index')
    parser.add_argument('--fresh-stop-at', type=int, help='fresh full-oracle rerun at absolute index')
    parser.add_argument('--root', type=Path, help='new evidence directory, normally under private scratch')
    args = parser.parse_args()
    if args.verify and (args.stop_at is not None or args.fresh_stop_at is not None):
        parser.error('--verify needs the complete case segment')
    if args.stop_at is not None and args.fresh_stop_at is not None:
        parser.error('choose a continuous or fresh stop')
    os.chdir(ROOT)
    root = args.root or comparison.wt.PREFIX.parent / (
        'fast-cpp-basic-%d-%d' % (os.getpid(), time.time_ns()))
    root = root.resolve()
    root.mkdir(parents=True, exist_ok=False)
    try:
        execute(root, args)
    except Exception as error:
        (root / 'adapter-failure.json').write_text(json.dumps(dict(
            directory=str(root), error_type=type(error).__name__, error=str(error)),
            indent=2) + '\n')
        print('C1XX replay failure evidence: ' + str(root), file=sys.stderr)
        raise


def execute(root, args):
    wall_started = time.monotonic()
    timings = {}
    if args.fresh_stop_at is not None:
        receipt = fresh_stop(root / 'fresh-stop', args.case, args.fresh_stop_at)
        (root / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
        print('Fresh-stop receipt: ' + str(root / 'receipt.json'))
        return
    phase_started = time.monotonic()
    shared.build_helper()
    timings['helper_build_seconds'] = time.monotonic() - phase_started
    phase_started = time.monotonic()
    frozen = shared.freeze(frozen_inputs())
    timings['freeze_setup_seconds'] = time.monotonic() - phase_started
    fresh = None
    if args.verify:
        phase_started = time.monotonic()
        log = root / 'fresh-history.log'
        with log.open('wb') as output:
            subprocess.run([sys.executable, str(Path(__file__).with_name('replay-fresh.py')),
                            '--case', args.case, '--jobs', str(args.jobs)],
                           cwd=ROOT, stdin=subprocess.DEVNULL, stdout=output,
                           stderr=subprocess.STDOUT, check=True, timeout=600)
        match = re.search(r'Replay receipt: (.+)', log.read_text())
        if not match:
            raise AssertionError('fresh history omitted its receipt: ' + str(log))
        fresh = json.loads(Path(match[1]).read_text())
        shared.verify_frozen(frozen)
        timings['fresh_reference_wall_seconds'] = time.monotonic() - phase_started
        timings['fresh_reference_runner_seconds'] = fresh['seconds']
    phase_started = time.monotonic()
    sides, first, last, terminal = prepare_pair(root, args.case, args.stop_at)
    timings['case_and_plan_setup_seconds'] = time.monotonic() - phase_started
    phase_started = time.monotonic()
    report = shared.run_pair(*sides, receipt_path=root / 'walker-pair.json')
    timings['fast_pair_with_helper_check_seconds'] = time.monotonic() - phase_started
    timings['fast_pair_seconds'] = report['seconds']
    phase_started = time.monotonic()
    report['output_hashes'] = validate_outputs(root, terminal)
    timings['output_oracle_seconds'] = time.monotonic() - phase_started
    report['frozen'] = frozen
    report['case'] = 'cpp_basic'
    if fresh is not None:
        expected = json.loads((ROOT / 'functions.json').read_text())['accepted'][first:last]
        if ([row['logical'] for row in fresh['results']] != expected or
                (fresh['first'], fresh['last'], fresh['final']) != (first, last, terminal)):
            raise AssertionError('fresh and continuous transcript/prefix/terminal differ')
        report['fresh_equivalence'] = dict(
            first=first, last=last, terminal=terminal, transcript_equal=True,
            seconds=fresh['seconds'], full_comparator_at_every_frontier=True,
            receipt_sha256=hashlib.sha256(Path(match[1]).read_bytes()).hexdigest(),
            receipt_path=match[1])
    phase_started = time.monotonic()
    shared.verify_frozen(frozen)
    timings['final_freeze_check_seconds'] = time.monotonic() - phase_started
    timings['adapter_wall_before_receipt_seconds'] = time.monotonic() - wall_started
    timings['prefix_setup'] = 'unchanged warm private Wine prefix; setup not measured'
    timings['receipt_serialization_and_process_exit'] = 'excluded from internal wall; external measurement may include them'
    report['timings'] = timings
    (root / 'receipt.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Replay receipt: ' + str(root / 'receipt.json'), flush=True)
    print('All %d first encounters and terminal %s passed in %.3f seconds.' %
          (last - first, terminal, report['seconds']), flush=True)


if __name__ == '__main__':
    main()
