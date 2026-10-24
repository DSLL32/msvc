#!/usr/bin/env python3
"""Reject any delta beyond the three exact numeric export-cache entries."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('numeric_locale_transport', Path(__file__).with_name('check-isolated-locale.py'))
    checker = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(checker)
    original = args.original.resolve()
    started, results = time.perf_counter(), []
    with tempfile.TemporaryDirectory(prefix='integer-locale-source-controls-', dir=original.parent) as temporary:
        directory = Path(temporary)
        for item in original.iterdir():
            (directory / item.name).symlink_to(item, target_is_directory=item.is_dir())
        for case in ('extra-source-byte', 'wrong-new-RVA', 'missing-new-export', 'fourth-export', 'wrong-candidate'):
            name = 'trial.json' if case == 'wrong-candidate' else 'locale-observer.inc'
            data = (original / name).read_bytes()
            if case == 'extra-source-byte': data += b'\n'
            elif case == 'wrong-new-RVA': data = data.replace(b'"isspace",0x1c0be', b'"isspace",0x1c0bf')
            elif case == 'missing-new-export': data = data.replace(b',{"isdigit",0x1c065,0}', b'')
            elif case == 'fourth-export': data = data.replace(b'{"atol",0x5d30,0}', b'{"atol",0x5d30,0},{"other",0x1234,0}')
            else:
                trial = json.loads(data)
                trial['trial'] = '_apply_option_action'
                data = json.dumps(trial).encode()
            (directory / name).unlink()
            (directory / name).write_bytes(data)
            receipt = json.loads((original / 'receipt.json').read_text())
            receipt['artifacts'][name] = dict(sha256=hashlib.sha256(data).hexdigest(), size=len(data))
            (directory / 'receipt.json').unlink()
            (directory / 'receipt.json').write_text(json.dumps(receipt))
            try:
                checker.check(directory)
            except ValueError as error:
                results.append(dict(case=case, rejection=str(error)))
            else:
                raise ValueError('mutation accepted: ' + case)
            for item in (name, 'receipt.json'):
                (directory / item).unlink()
                (directory / item).symlink_to(original / item)
    packet = dict(kind='numeric-only exact export-cache refinement controls; no binary invocation',
                  original=str(original), original_receipt_sha256=checker.sha(original / 'receipt.json'),
                  checker_sha256=checker.sha(Path(__file__).with_name('check-isolated-locale.py')),
                  control_sha256=checker.sha(Path(__file__)), count=len(results), cases=results,
                  seconds=time.perf_counter() - started)
    if args.output.exists(): raise ValueError('refuse overwrite')
    args.output.write_text(json.dumps(packet, indent=2) + '\n')
    print(json.dumps(dict(count=len(results), seconds=packet['seconds'])))


if __name__ == '__main__':
    main()
