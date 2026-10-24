"""Focused negatives for the initialized declaration iterator forwarding caller."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import time
from declaration_thunk_owner import validate


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('proof', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    started = time.perf_counter()
    proof = json.loads(args.proof.read_text())
    directory = args.proof.parent
    original = json.loads((directory/'original-receipt.json').read_text())
    checkpoint = json.loads((directory/'native-checkpoint.json').read_text())
    config = json.loads((directory/'original-configuration.json').read_text())
    events = (directory/'original-events.txt').read_text()
    digest = hashlib.sha256(events.encode()).hexdigest()
    words = {int(i): int(v, 16) for i, v in re.findall(r'^argument (\d+) ([0-9a-f]+)$', events, re.M)}
    checks = []
    for name in ('control', 'wrong-iterator-offset', 'wrong-push', 'wrong-caller', 'missing-owner-entry', 'wrong-native-formal', 'wrong-native-value', 'wrong-actual-word'):
        formal = copy.deepcopy(next(row for row in proof['formals'] if row['name'] == 'function'))
        native, receipt = copy.deepcopy(checkpoint), copy.deepcopy(original)
        if name == 'wrong-iterator-offset': formal['binding']['value_offset'] = 12
        elif name == 'wrong-push': formal['binding']['push_instruction'] += 1
        elif name == 'wrong-caller': formal['binding']['original_caller_rva'] += 1
        elif name == 'missing-owner-entry': receipt['ownership_entry_projection']['observed'] = []
        elif name == 'wrong-native-formal': native['caller_locals']['caller_function']['fields'] = ['key']
        elif name == 'wrong-native-value': native['caller_locals']['caller_function']['value'] += 4
        elif name == 'wrong-actual-word': formal['original_raw'] += 4
        try: validate(proof['function'], formal, native, words, receipt, events, config, digest)
        except (AssertionError, KeyError) as error:
            assert name != 'control', str(error)
            checks.append(dict(test=name, result='held', reason=str(error)))
        else:
            assert name == 'control', name + ' unexpectedly passed'
            checks.append(dict(test=name, result='verified'))
    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    report = dict(schema=1, scope=__doc__, proof=str(args.proof), proof_sha256=sha(args.proof),
                  checker_sha256=sha(Path(__file__)), validator_sha256=sha(Path(__file__).with_name('declaration_thunk_owner.py')),
                  checks=checks, elapsed_seconds=time.perf_counter()-started)
    args.receipt.parent.mkdir(parents=True, exist_ok=True)
    args.receipt.write_text(json.dumps(report, indent=2)+'\n')
    print(args.receipt)


if __name__ == '__main__': main()
