"""Exercise C1XX transcript stopping, divergence evidence and fresh fallback.

The shared walker's own negative suite covers bad writes, access violations,
unsupported threads/processes, early exits and timeouts. These checks exercise
the target adapter's actual fixture and exact file/stream oracle.
"""
import copy
import json
import os
from pathlib import Path
import time

import fast_replay_adapter as adapter


def expect_failure(action, text):
    try:
        action()
    except (RuntimeError, AssertionError) as error:
        if text not in str(error):
            raise AssertionError('wrong rejection: ' + str(error)) from error
        return str(error)
    raise AssertionError('negative replay unexpectedly passed')


def main():
    os.chdir(adapter.ROOT)
    root = adapter.comparison.wt.PREFIX.parent / ('fast-adapter-check-%d' % time.time_ns())
    root.mkdir()
    adapter.shared.build_helper()
    frozen = adapter.shared.freeze(adapter.frozen_inputs())
    results = {}

    stopped = root / 'continuous-stop-100'
    sides, first, last, terminal = adapter.prepare_pair(stopped, stop_at=100)
    receipt = adapter.shared.run_pair(*sides, receipt_path=stopped / 'receipt.json')
    adapter.validate_outputs(stopped, terminal)
    assert (first, last) == (0, 100)
    assert all(side['events'][-1]['event'] == 'pass' for side in receipt['sides'])
    results['continuous_stop'] = dict(first=first, last=last, terminal=terminal, passed=True)

    results['fresh_fallback'] = adapter.fresh_stop(root / 'fresh-stop-100', '', 100)
    assert results['fresh_fallback']['terminal'] == terminal
    (stopped / 'reconstructed.stdout').write_bytes(b'injected output difference')
    results['output_oracle_rejects'] = expect_failure(
        lambda: adapter.validate_outputs(stopped, terminal), 'stdout differs')

    changed = root / 'changed-input'
    sides, _, _, _ = adapter.prepare_pair(changed, stop_at=1)
    (changed / 'minimal-original/cpp-basic.cpp').write_bytes(b'changed input')
    results['input_freeze_rejects'] = expect_failure(
        lambda: adapter.shared.run_pair(*sides), 'frozen replay input changed')

    # Alter only a scratch plan's expected entry. The native entry stays INT3,
    # so this must fail immediately instead of accepting an interior address.
    real_indexes = adapter.comparison.indexes
    original, rebuilt, config, old, new = real_indexes()
    damaged = copy.deepcopy(old)
    next(record for record in damaged if record['name'] == config['accepted'][0])['start'] += 1
    adapter.comparison.indexes = lambda: (original, rebuilt, config, damaged, new)
    wrong = root / 'wrong-entry'
    try:
        sides, _, _, _ = adapter.prepare_pair(wrong, stop_at=2)
    finally:
        adapter.comparison.indexes = real_indexes
    results['wrong_entry_rejects'] = expect_failure(
        lambda: adapter.shared.run_pair(*sides), 'fast replay divergence')
    events = [json.loads(line) for line in sides[0].receipt.read_text().splitlines()]
    failure = next(event for event in events if event['event'] == 'failure')
    assert failure['index'] == 0
    assert all(field in failure for field in (
        'pid', 'base', 'expected', 'thread', 'exception', 'address', 'registers'))
    results['failure_context_retained'] = failure
    adapter.shared.verify_frozen(frozen)
    report = dict(directory=str(root), results=results, frozen=frozen,
                  no_acceptance_changes=True, no_reconstruction_functions_added=True)
    (root / 'receipt.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Adapter checks passed: ' + str(root / 'receipt.json'))


if __name__ == '__main__':
    main()
