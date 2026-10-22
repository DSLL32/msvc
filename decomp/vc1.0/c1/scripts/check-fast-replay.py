#!/usr/bin/env python3
"""Check C1 continuous stop, fresh fallback, and wrong-entry rejection."""
from importlib.machinery import SourceFileLoader
import json
from pathlib import Path
import shutil

adapter = SourceFileLoader('c1_fast_adapter', str(Path(__file__).with_name('fast-replay.py'))).load_module()
comparison, shared = adapter.comparison, adapter.shared


def main():
    config_before = shared.sha256('functions.json')
    comparison.select_case('minimal')
    original, _, config, records, _ = comparison.indexes()
    index = 100
    expected = config['accepted'][index]
    stopped = adapter.replay_cases('minimal', stop_before=index)
    assert stopped['receipts'][0]['terminal'] == expected
    # This is the full existing focused oracle, including pending buffers/files.
    fresh = comparison.compare(accepted_count=index, baseline=False)
    assert fresh == expected, 'fresh divergence fallback changed'
    directory = Path('build/fast-replay/wrong-entry')
    if directory.exists():
        shutil.rmtree(directory)
    label = 'reject-original'
    stem, work, wine_env, cwd, flags = comparison.setup_run(label)
    accepted = list(config['accepted'])
    accepted[index], accepted[index+1] = accepted[index+1], accepted[index]
    side = shared.prepare_side(
        directory=directory, pe=original, records=records, accepted=accepted,
        first=0, last=index+2, terminal=config['accepted'][index+2],
        cwd=cwd, environment={'MSC_CMD_FLAGS': flags}, wine_env=wine_env,
        mask_image=comparison.mask,
        frozen_paths=adapter.frozen_inputs() + list(work.iterdir()),
        stdout=Path(str(stem) + '.stdout'), stderr=Path(str(stem) + '.stderr'))
    try:
        shared.run_side(side)
    except RuntimeError:
        events = [json.loads(line) for line in side.receipt.read_text().splitlines()]
        failure = next(row for row in events if row['event'] == 'failure')
        assert 'wrong frontier' in failure['reason'], failure
        assert failure['index'] == index and failure['expected'] == accepted[index]
        assert 'registers' in failure and 'address' in failure, failure
        actual = comparison.logical('exception 80000003 rva %08x' %
                                    (failure['address'] - original.base), original, records)
        assert actual == expected, actual
    else:
        raise AssertionError('wrong expected function was accepted')
    assert shared.sha256('functions.json') == config_before, 'acceptance metadata changed'
    report = dict(continuous_stop=dict(index=index, terminal=expected, passed=True),
                  fresh_fallback=dict(index=index, terminal=fresh, passed=True),
                  wrong_entry=dict(rejected=True, index=index,
                                   expected=failure['expected'], actual=actual,
                                   evidence=str(side.receipt), context_retained=True),
                  acceptance_metadata_unchanged=True)
    Path('build/fast-replay/adapter-checks.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
