"""Focused checks that an unwritten component-zero slot stays address-only."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import time
from member_component_output import validate


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('proof', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    started = time.perf_counter()
    proof = json.loads(args.proof.read_text())
    native_original = json.loads((args.proof.parent/'native-checkpoint.json').read_text())
    raw = (args.proof.parent/'original-events.txt').read_text()
    original_words = {int(i): int(v, 16) for i, v in re.findall(r'^argument (\d+) ([0-9a-f]+)$', raw, re.M)}
    checks = []
    for name in ('control', 'nonzero-component', 'invented-prestate', 'wrong-output-local', 'wrong-output-address', 'missing-output-role'):
        formals, native, words = copy.deepcopy(proof['formals']), copy.deepcopy(native_original), dict(original_words)
        saved = next(row for row in formals if row['name'] == 'saved')
        owner = native['caller_locals']['caller_saved']
        if name == 'nonzero-component': words[1] = native['raw_component'] = 1
        elif name == 'invented-prestate': owner['value'] = 0
        elif name == 'wrong-output-local': owner['local'] = 'original'
        elif name == 'wrong-output-address': owner['address'] += 4
        elif name == 'missing-output-role': saved['binding']['output_only'] = False
        try: validate(proof['function'], formals, native, words)
        except (AssertionError, KeyError) as error:
            assert name != 'control', str(error)
            checks.append(dict(test=name, result='held', reason=str(error)))
        else:
            assert name == 'control', name + ' unexpectedly passed'
            checks.append(dict(test=name, result='verified'))
    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    report = dict(schema=1, scope=__doc__, proof=str(args.proof), proof_sha256=sha(args.proof),
                  checker_sha256=sha(Path(__file__)), validator_sha256=sha(Path(__file__).with_name('member_component_output.py')),
                  checks=checks, elapsed_seconds=time.perf_counter()-started)
    args.receipt.parent.mkdir(parents=True, exist_ok=True)
    args.receipt.write_text(json.dumps(report, indent=2)+'\n')
    print(args.receipt)


if __name__ == '__main__': main()
