"""Replay accepted trap pairs in isolated per-frontier directories."""
import argparse
import concurrent.futures
import hashlib
from importlib.machinery import SourceFileLoader
import json
import os
from pathlib import Path
import shutil
import subprocess
import time
import compare as comparison


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=('minimal', 'hello'), default='minimal')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--method', choices=('fast', 'fresh'), default='fast')
    parser.add_argument('--stop-at', type=int, help='stop at this absolute acceptance index')
    parser.add_argument('--receipt', type=Path, help='write the summary receipt here')
    args = parser.parse_args()
    if args.method == 'fast':
        adapter = SourceFileLoader('c2_fast_replay', str(Path(__file__).with_name('fast-replay.py'))).load_module()
        adapter.run(args.profile, stop_at=args.stop_at, receipt_path=args.receipt)
        return
    assert 1 <= args.jobs <= 8, 'replay supports one to eight independent pairs'
    comparison.select_profile(args.profile)
    original, rebuilt, config, original_records, rebuilt_records = comparison.indexes()
    case = config['cases'][args.profile]
    first = case.get('accepted_from', 0)
    last = case.get('accepted_count', len(config['accepted']))
    segment_last = last
    if args.stop_at is not None:
        assert first <= args.stop_at <= last, 'stop-at is outside the case segment'
        last = args.stop_at
    root = comparison.wt.PREFIX.parent / ('replay-' + args.profile + '-' + str(os.getpid()))
    root.mkdir()
    frozen = {str(path): sha256(path) for path in
              ('functions.json', 'build/c2.exe', 'build/c2.map', 'build/runtime.json')}
    environment = comparison.wt.environment()
    environment['MSC_CMD_FLAGS'] = comparison.PHASE['environment']['MSC_CMD_FLAGS'].format(
        bin=comparison.wt.windows(comparison.wt.TC / 'bin'))
    inputs = []
    for source in comparison.PHASE['inputs']:
        path = comparison.FIXTURES / source['path']
        assert sha256(path) == source['sha256'], 'fixture changed: ' + str(path)
        inputs.append(path)

    def run_side(count, label, executable, records):
        directory = root / str(count) / label
        directory.mkdir(parents=True)
        masked = directory / 'masked.exe'
        comparison.mask(executable, records, config['accepted'][:count], masked)
        work = directory / 'work'
        work.mkdir()
        for path in inputs:
            shutil.copyfile(path, work / path.name)
        command = ['wine', 'build/debug-run.exe', comparison.wt.windows(masked),
                   case['arguments'], comparison.wt.windows(directory / 'stdout'),
                   comparison.wt.windows(directory / 'stderr'),
                   comparison.wt.windows(directory / 'events'), comparison.wt.windows(work)]
        subprocess.run(command, env=environment, check=True, timeout=90,
                       stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL)
        events = (directory / 'events').read_text()
        return comparison.logical(events, executable, records), events

    def replay(count):
        with concurrent.futures.ThreadPoolExecutor(2) as pair:
            left = pair.submit(run_side, count, 'original', original, original_records)
            right = pair.submit(run_side, count, 'reconstructed', rebuilt, rebuilt_records)
            left, right = left.result(), right.result()
        expected = config['accepted'][count]
        assert left[0] == right[0] == expected, (count, expected, left, right)
        return dict(count=count, logical=expected)

    started = time.monotonic()
    results = []
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        for result in pool.map(replay, range(first, last)):
            results.append(result)
            print('frontier %d/%d: %s' % (result['count'], last, result['logical']), flush=True)
    for path, digest in frozen.items():
        assert sha256(path) == digest, 'frozen build/config changed: ' + path
    # No parallel pair remains when the ordinary final-case comparator writes
    # its shared build paths. It retains baseline, stream and artifact checks.
    final = comparison.compare(last, case=case['arguments'])
    if args.stop_at is not None and last < segment_last:
        assert final == config['accepted'][last], 'fresh stop-at frontier differs'
    receipt = dict(profile=args.profile, first=first, last=last, frontiers=len(results),
                   jobs=args.jobs, seconds=time.monotonic()-started, frozen=frozen,
                   final=final, results=results)
    receipt.update(method='fresh', terminal=final)
    receipt_path = args.receipt or root / 'receipt.json'
    receipt_path.write_text(json.dumps(receipt, indent=2) + '\n')
    print('Replay receipt: ' + str(receipt_path), flush=True)


if __name__ == '__main__':
    main()
