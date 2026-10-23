"""Focused corruptions of the reached two-load parameter-record ownership proof."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import tempfile
import time
from case_paths import BINARY, CAPTURE_ROOT
from member_parameter_owner import validate


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('proof', type=Path)
    args = parser.parse_args()
    started = time.perf_counter()
    proof = json.loads(args.proof.read_text())
    directory = Path(tempfile.mkdtemp(prefix='member-parameter-owner-negatives-', dir=CAPTURE_ROOT))
    raw = args.proof.parent
    configuration = json.loads((raw/'original-configuration.json').read_text())
    receipt = json.loads((raw/'original-receipt.json').read_text())
    native = json.loads((raw/'native-checkpoint.json').read_text())
    events = (raw/'original-events.txt').read_text()
    formal = next(r for r in proof['formals'] if r['name'] == 'target')
    anchors = {r['name']: r for r in receipt['diagnostic_observations']}
    words = {r['stack_index']: r['original_raw'] for r in proof['formals']}
    base = dict(formal=formal, anchors=anchors, native=native, receipt=receipt, words=words)

    def check(data):
        validate(proof['function'], data['formal'], data['anchors'], data['native'], data['words'], data['receipt'], events, configuration, hashlib.sha256(events.encode()).hexdigest())

    check(base)
    tests = [
        ('missing-root', lambda d: d['receipt'].__setitem__('ownership_register_observations', [])),
        ('wrong-root', lambda d: d['receipt']['ownership_register_observations'][1].__setitem__('value', 0)),
        ('wrong-first-link', lambda d: d['anchors']['target_signature'].__setitem__('original', anchors['target_signature']['original']+4)),
        ('wrong-second-link', lambda d: d['anchors']['target_parameters'].__setitem__('original', anchors['target_parameters']['original']+4)),
        ('wrong-offset', lambda d: d['formal']['binding'].__setitem__('offsets', [4, 4])),
        ('missing-link', lambda d: d['anchors'].pop('target_signature')),
        ('unreadable-link', lambda d: d['anchors']['target_signature'].__setitem__('read_succeeded', False)),
        ('short-read', lambda d: d['anchors']['target_parameters'].__setitem__('read_bytes', 2)),
        ('NULL-link', lambda d: d['anchors']['target_signature'].__setitem__('value', 0)),
        ('native-NULL-root', lambda d: d['native']['caller_locals']['caller_target'].__setitem__('root', 0)),
        ('native-wrong-link', lambda d: d['native']['caller_locals']['caller_target']['pointer_path'][1].__setitem__('address', 1)),
        ('native-wrong-owner', lambda d: d['native']['caller_locals']['caller_target'].__setitem__('local', 'source_shape')),
        ('wrong-formal', lambda d: d['words'].__setitem__(1, words[1]+4)),
    ]
    checks = [dict(test='actual-current-control', status='pass')]
    for name, change in tests:
        data = copy.deepcopy(base)
        change(data)
        try:
            check(data)
        except (AssertionError, KeyError) as error:
            checks.append(dict(test=name, status='held', reason=str(error)))
        else:
            raise AssertionError('corrupted ownership accepted: '+name)
    result = dict(status='pass', scope='native-only/read-only focused ownership metadata corruptions, no original capture/promotion', proof=str(args.proof), proof_sha256=hashlib.sha256(args.proof.read_bytes()).hexdigest(), checks=checks, seconds=time.perf_counter()-started)
    (directory/'receipt.json').write_text(json.dumps(result, indent=2)+'\n')
    print(directory/'receipt.json')


if __name__ == '__main__':
    main()
