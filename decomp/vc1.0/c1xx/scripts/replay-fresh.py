"""Replay every accepted frontier with the full comparator in isolated workers."""
import argparse
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import compare as comparison

ROOT = Path(__file__).resolve().parents[1]


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--case', default='')
    parser.add_argument('--worker', type=int, help=argparse.SUPPRESS)
    parser.add_argument('--root', type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    os.chdir(ROOT)
    config = json.loads(Path('functions.json').read_text())
    if args.worker is not None:
        assert args.root is not None
        count = args.worker
        expected = config['accepted'][count]
        started = time.monotonic()
        workspace = args.root / str(count)
        logical = comparison.compare(count, case=args.case, baseline=False, workspace=workspace)
        assert logical == expected, (count, logical, expected)
        receipt = dict(count=count, logical=logical, seconds=time.monotonic()-started)
        (workspace / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
        return

    assert 1 <= args.jobs <= 8, 'replay supports one to eight independent frontier pairs'
    case = next(case for case in config['cases'].values() if case['arguments'] == args.case)
    first = case.get('accepted_from', 0)
    last = case.get('accepted_count', len(config['accepted']))
    root = comparison.wt.PREFIX.parent / ('replay-cpp-basic-%d-%d' % (os.getpid(), time.time_ns()))
    (root / 'logs').mkdir(parents=True)
    frozen_paths = set()
    for folder in ('src', 'scripts'):
        for path in (ROOT / folder).rglob('*'):
            if path.is_file() and '__pycache__' not in path.parts:
                frozen_paths.add(path)
    frozen_paths.update(ROOT / path for path in (
        'Makefile', 'functions.json', 'runtime.json', 'build/c1xx.exe',
        'build/c1xx.map', 'build/runtime.json', 'build/debug-run.exe',
        '../fixtures/cpp-basic/manifest.json', '../fixtures/cpp-basic/c1/cpp-basic.cpp'))
    frozen_paths.add(comparison.wt.TC / 'bin/C1XX3232.EXE')
    frozen = {str(path.resolve()): sha256(path) for path in sorted(frozen_paths)}
    started = time.monotonic()
    print('Frozen replay: %d frontiers, %d isolated pairs; full files, streams, buffers, callers and %d states.' %
          (last-first, args.jobs, len(config.get('checkpoint_state', []))), flush=True)

    def replay(count):
        log = root / 'logs' / ('%04d.log' % count)
        with log.open('wb') as output:
            completed = subprocess.run(
                [sys.executable, str(Path(__file__).resolve()), '--worker', str(count),
                 '--root', str(root), '--case', args.case],
                cwd=ROOT, stdin=subprocess.DEVNULL, stdout=output, stderr=subprocess.STDOUT,
                timeout=120)
        assert completed.returncode == 0, 'frontier %d failed: %s\n%s' % (
            count, log, log.read_text(errors='replace')[-5000:])
        receipt = json.loads((root / str(count) / 'receipt.json').read_text())
        assert receipt['count'] == count and receipt['logical'] == config['accepted'][count]
        receipt['log_sha256'] = sha256(log)
        return receipt

    results = []
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        for result in pool.map(replay, range(first, last)):
            results.append(result)
            print('frontier %d/%d: %s' % (result['count'], last, result['logical']), flush=True)
    for path, digest in frozen.items():
        assert sha256(path) == digest, 'frozen source, build, fixture or harness changed: ' + path
    # Every private pair has finished before the ordinary comparator touches
    # its conventional build paths and, on completion, the captured oracle.
    final = comparison.compare(last, case=args.case)
    assert not final.startswith(('fault:', 'unknown:')), 'final frontier is not an accepted logical trap: ' + final
    for path, digest in frozen.items():
        assert sha256(path) == digest, 'frozen input changed during final comparison: ' + path
    receipt = dict(first=first, last=last, frontiers=len(results), jobs=args.jobs,
                   seconds=time.monotonic()-started, frozen=frozen, final=final,
                   checkpoint_fields=len(config.get('checkpoint_state', [])),
                   full_comparator_at_every_frontier=True, results=results)
    (root / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('Replay receipt: ' + str(root / 'receipt.json'), flush=True)
    print('All %d frontiers passed in %.3f seconds.' % (len(results), receipt['seconds']), flush=True)


if __name__ == '__main__':
    main()
