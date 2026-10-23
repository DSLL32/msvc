#!/usr/bin/env python3
"""Reject rank-ready reset/storage drift and a false genuine helper frontier."""
import importlib.util
import json
from pathlib import Path
import re
import shutil
import struct
import tempfile
import time


def load(name, file):
    spec = importlib.util.spec_from_file_location(name, Path(__file__).with_name(file))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    d = load('rank_decoder', 'check-callback.py')
    h = load('rank_negative', 'check-projection-negative.py')
    history = load('rank_history', 'check-history.py')
    packet = json.loads((d.s.PROJECT / 'cases/minimal/rank-wrapper-trials.json').read_text())
    started = time.perf_counter()
    passed = []
    for row in packet['trials']:
        original, native = Path(row['original']), Path(row['native'])
        audit = history.configure(d, original, native, saved_record=row)
        d.check(original, native, audit_path=audit)
        cases = ('original-default', 'native-width', 'native-alias', 'false-helper-PC', 'unexpected-helper-argument', 'missing-helper-frontier') if row['seed'] is None else ('original-prestate', 'native-prestate', 'original-clear', 'native-clear') if row['seed'] == 'restore-nonzero' else ('original-helper-state', 'native-helper-state')
        for case in cases:
            with tempfile.TemporaryDirectory(prefix='rank-wrapper-negative-', dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as tmp:
                o, n = Path(tmp)/'original', Path(tmp)/'native'
                shutil.copytree(original, o)
                shutil.copytree(native, n)
                if case.startswith('original-'):
                    suffix = '000' if case == 'original-default' else '006' if case == 'original-prestate' else 'phase' if case == 'original-clear' else '%03d' % int(re.findall(r'^trap seq=(\d+) rva=', (o/'events.txt').read_text(), re.M)[-1])
                    name = 'type-ranks-ready-'+suffix+'.bin'
                    (o/name).write_bytes(struct.pack('<I', 0 if case == 'original-prestate' else 1))
                    h.rehash(o, name)
                elif case == 'missing-helper-frontier':
                    f = o/'events.txt'
                    f.write_text(re.sub(r'^frontier rva=\w+\n', '', f.read_text(), flags=re.M))
                    h.rehash(o, f.name)
                else:
                    name = 'pre.json' if case == 'native-prestate' else 'phase.json' if case == 'native-clear' else 'state.json'
                    def change(j):
                        f = j['frontend']
                        if case == 'native-width': f['storage']['type_ranks_initialized'][1] = 8
                        elif case == 'native-alias': f['storage']['type_ranks_initialized'][0] = f['storage']['source_signature'][0]
                        elif case == 'false-helper-PC': j['pc'] += 1
                        elif case == 'unexpected-helper-argument': j['args']['invented'] = 1
                        elif case == 'native-prestate': f['type_ranks_initialized'] = 0
                        else: f['type_ranks_initialized'] = 1
                    h.edit(n/name, change)
                    h.rehash(n, name)
                h.rejected(case, lambda: d.check(o, n, audit_path=audit))
                passed.append(case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed}, indent=2))


if __name__ == '__main__':
    main()
