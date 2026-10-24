#!/usr/bin/env python3
"""Bounded semantic denials, with mutated copies rehashed past transport checks."""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import tempfile
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source = Path(__file__).with_name('check-isolated-locale.py')
    spec = importlib.util.spec_from_file_location('isolated_locale_controls', source)
    checker = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(checker)
    original = args.original.resolve()
    started = time.perf_counter()
    checker.check(original)
    mutations = {
        'internal-MB-flag': ('locale-after-words.bin', 9 * 4),
        'MB-table': ('locale-after-mbctype.bin', 256),
        'static-ctype-EOF': ('locale-before-ctype.bin', 0),
        'retained-old-table': ('locale-after-old-ctype.bin', 513),
        'result-lifetime': ('locale-next-result.bin', 0),
        'FP-control': ('locale-after-fp.bin', 0),
        'target-unknown-BSS': ('locale-after-target-BSS.bin', 63131),
        'private-vector': ('locale-after-CRT-environment.bin', -2),
        'mapped-region-state': ('locale-after-ctype-region.bin', 16),
        'checksum-table': ('checksum-tables-checksum-return.bin', 767),
        'saved-snapshot': ('frontend-snapshot-locale-after.bin', 8519),
        'public-category': ('events.txt', None),
        'public-return-stack': ('events.txt', None),
    }
    results = []
    with tempfile.TemporaryDirectory(prefix='locale-controls-', dir=Path.home() / '.ai-tmp/msvc2/vc5.10/c1xx') as temporary:
        work = Path(temporary)
        for path in original.iterdir():
            (work / path.name).symlink_to(path, target_is_directory=path.is_dir())
        (work / 'receipt.json').unlink()
        for name, (filename, offset) in mutations.items():
            path = work / filename
            path.unlink()
            blob = bytearray((original / filename).read_bytes())
            if offset is not None:
                blob[offset] ^= 1
            else:
                text = blob.decode()
                if name == 'public-category':
                    text = text.replace('category=2 argument=', 'category=3 argument=')
                else:
                    text = re.sub(r'(locale-return seq=\d+ pc=\w+ esp=)(\w+)', lambda m: m[1] + '%08x' % (int(m[2], 16) + 4), text)
                blob = text.encode()
            path.write_bytes(blob)
            receipt = json.loads((original / 'receipt.json').read_text())
            receipt['artifacts'][filename] = dict(sha256=checker.sha(path), size=len(blob))
            (work / 'receipt.json').write_text(json.dumps(receipt))
            try:
                checker.check(work)
            except (ValueError, AssertionError) as error:
                results.append(dict(case=name, rejection=str(error), transport_hash_rebound=True))
            else:
                raise RuntimeError('mutant accepted: ' + name)
            path.unlink()
            path.symlink_to(original / filename)
    require = checker.require
    require(not args.output.exists(), 'refuse control receipt overwrite')
    result = dict(kind='fresh isolated API semantic denials; originals unchanged; no native equivalence', original_directory=str(original), original_receipt_sha256=checker.sha(original / 'receipt.json'), checker_sha256=checker.sha(source), control_checker_sha256=checker.sha(Path(__file__)), count=len(results), cases=results, seconds=time.perf_counter() - started)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(count=len(results), seconds=result['seconds'])))


if __name__ == '__main__':
    main()
