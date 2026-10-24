"""Focused missing/read/offset/value checks for the actual thunk-key tail words."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import time
from tail_word_pair_inputs import validate


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('proof', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    started = time.perf_counter()
    proof = json.loads(args.proof.read_text())
    directory = args.proof.parent
    receipt = json.loads((directory/'original-receipt.json').read_text())
    checkpoint = json.loads((directory/'native-checkpoint.json').read_text())
    words = {int(i): int(v, 16) for i, v in re.findall(r'^argument (\d+) ([0-9a-f]+)$', (directory/'original-events.txt').read_text(), re.M)}
    checks = []
    for name in ('control', 'missing-word', 'short-read', 'unreadable-word', 'wrong-offset', 'wrong-native-value', 'wrong-function'):
        anchors = {row['name']: copy.deepcopy(row) for row in receipt['diagnostic_observations']}
        native = copy.deepcopy(checkpoint)
        function = proof['function']
        if name == 'missing-word': del anchors['right_kind']
        elif name == 'short-read': anchors['left_displacement']['read_bytes'] = 2
        elif name == 'unreadable-word': anchors['right_displacement']['read_succeeded'] = False
        elif name == 'wrong-offset': anchors['left_kind']['original'] += 4
        elif name == 'wrong-native-value': native['right_kind'] ^= 1
        elif name == 'wrong-function': function = '_equal_first_words'
        try: validate(function, anchors, native, words)
        except (AssertionError, KeyError) as error:
            assert name != 'control', str(error)
            checks.append(dict(test=name, result='held', reason=str(error)))
        else:
            assert name == 'control', name + ' unexpectedly passed'
            checks.append(dict(test=name, result='verified'))
    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    report = dict(schema=1, scope=__doc__, proof=str(args.proof), proof_sha256=sha(args.proof),
                  checker_sha256=sha(Path(__file__)), validator_sha256=sha(Path(__file__).with_name('tail_word_pair_inputs.py')),
                  checks=checks, elapsed_seconds=time.perf_counter()-started)
    args.receipt.parent.mkdir(parents=True, exist_ok=True)
    args.receipt.write_text(json.dumps(report, indent=2)+'\n')
    print(args.receipt)


if __name__ == '__main__': main()
