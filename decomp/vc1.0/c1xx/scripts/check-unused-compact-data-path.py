"""Focused checks for the one whole-body unused compact-data path formal."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import time
from unused_compact_data_path import validate


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('proof', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    started = time.perf_counter()
    proof = json.loads(args.proof.read_text())
    directory = args.proof.parent
    checkpoint = json.loads((directory/'native-checkpoint.json').read_text())
    configuration = json.loads((directory/'original-configuration.json').read_text())
    events = (directory/'original-events.txt').read_text()
    words = {int(i): int(value, 16) for i, value in re.findall(r'^argument (\d+) ([0-9a-f]+)$', events, re.M)}
    checks = []
    for name in ('control', 'wrong-function', 'wrong-formal-index', 'wrong-body', 'wrong-table-slot', 'wrong-table-alias', 'wrong-owner', 'wrong-model', 'wrong-raw-word'):
        formal = copy.deepcopy(next(row for row in proof['formals'] if row['name'] == 'path'))
        native, config = copy.deepcopy(checkpoint), copy.deepcopy(configuration)
        function = proof['function']
        if name == 'wrong-function': function = '_convert_compact_method_pointer@24' if proof['function'] == '_convert_compact_data_member_pointer@24' else '_convert_compact_data_member_pointer@24'
        elif name == 'wrong-formal-index': formal['stack_index'] = 3
        elif name == 'wrong-body': formal['binding']['body_extent'][1] += 1
        elif name == 'wrong-table-slot': formal['binding']['callback_slots'][0] = 31
        elif name == 'wrong-table-alias':
            table = next(row for row in config['tables'] if row.get('name') == '_member_pointer_conversions')
            table['functions'][16] = '_convert_compact_method_pointer@24'
        elif name == 'wrong-owner': native['caller_locals']['caller_source_model']['function'] = 'other'
        elif name == 'wrong-model': native['caller_locals']['caller_source_model']['value'] = 3
        elif name == 'wrong-raw-word': formal['original_raw'] += 4
        try:
            result = validate(function, formal, native, words, config)
            assert result == 'uncredited-unused'
        except (AssertionError, KeyError) as error:
            assert name != 'control', str(error)
            checks.append(dict(test=name, result='held', reason=str(error)))
        else:
            assert name == 'control', name + ' unexpectedly passed'
            checks.append(dict(test=name, result=result))
    report = dict(schema=1, scope=__doc__, proof=str(args.proof), proof_sha256=sha(args.proof),
                  checker_sha256=sha(Path(__file__)), validator_sha256=sha(Path(__file__).with_name('unused_compact_data_path.py')),
                  checks=checks, elapsed_seconds=time.perf_counter()-started)
    args.receipt.parent.mkdir(parents=True, exist_ok=True)
    args.receipt.write_text(json.dumps(report, indent=2)+'\n')
    print(args.receipt)


if __name__ == '__main__': main()
