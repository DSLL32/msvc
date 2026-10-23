#!/usr/bin/env python3
"""Reject altered allocator ABI, ownership, memory and continuation observations."""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import shutil
import tempfile
import time


def load(name, file):
    spec = importlib.util.spec_from_file_location(name, Path(__file__).with_name(file))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--storage-only',action='store_true',help='Validate all three positive pairs and the two added original storage guards only')
    options=parser.parse_args()
    d = load('allocation_decoder', 'check-callback.py')
    h = load('allocation_negative', 'check-projection-negative.py')
    history = load('allocation_history', 'check-history.py')
    packet = json.loads((d.s.PROJECT/'cases/minimal/allocation-trials.json').read_text())
    started = time.perf_counter()
    passed = []
    for row in packet['trials']:
        original, native = Path(row['original']), Path(row['native'])
        audit = history.configure(d, original, native, saved_record=row)
        d.check(original, native, audit_path=audit)
        cases = ['provider-hFile', 'provider-IAT', 'original-api-args',
                 'original-api-RET16', 'original-api-EDI', 'original-error',
                 'original-region', 'original-helper-args', 'original-helper-RET0',
                 'original-helper-ESI', 'original-zero-memory', 'original-BSS-neighbor',
                 'original-data-neighbor', 'original-snapshot-neighbor',
                 'native-api-args', 'native-api-formal', 'native-api-PC',
                 'native-api-caller', 'native-api-RET16', 'native-api-EDI',
                 'native-api-result', 'native-error', 'native-FPU',
                 'native-reserve-map', 'native-commit-map', 'native-zero-memory',
                 'native-helper-formal', 'native-helper-args', 'native-helper-RET0',
                 'native-helper-EBX', 'native-helper-NULL', 'native-helper-alias',
                 'native-before-store', 'native-limit', 'native-cleanup-count',
                 'native-cleanup-callback', 'native-next-argv'] if row['seed'] is None else [
                 'native-restore-neighbor', 'native-limit', 'original-BSS-neighbor']
        if options.storage_only:
            cases=['original-data-neighbor','original-snapshot-neighbor'] if row['seed'] is None else []
        for case in cases:
            with tempfile.TemporaryDirectory(prefix='allocation-negative-', dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx') as temporary:
                o, n = Path(temporary)/'original', Path(temporary)/'native'
                shutil.copytree(original, o)
                shutil.copytree(native, n)
                if case.startswith('original-') or case.startswith('provider-'):
                    final='%03d'%int(re.findall(r'^trap seq=(\d+) ',(o/'events.txt').read_text(),re.M)[-1])
                    fname = 'loaded-kernel32.bin' if case == 'provider-hFile' else 'work-committed-01.bin' if case == 'original-zero-memory' else 'allocation-final-bss.bin' if case == 'original-BSS-neighbor' else 'frontend-data-'+final+'.bin' if case=='original-data-neighbor' else 'frontend-snapshot-'+final+'.bin' if case=='original-snapshot-neighbor' else 'events.txt'
                    f = o/fname
                    if fname.endswith('.bin'):
                        blob = bytearray(f.read_bytes()); blob[-1] ^= 1; f.write_bytes(blob)
                    else:
                        text = f.read_text()
                        pattern, field = {
                            'provider-IAT': (r'^(allocation-provider .*resolved=)([0-9a-f]+)$', 0),
                            'original-api-args': (r'^(allocation-before .*args=)([0-9a-f,]+)', 1),
                            'original-api-RET16': (r'^(allocation-after .*esp=)([0-9a-f]+)', 0),
                            'original-api-EDI': (r'^(allocation-after .*edi=)([0-9a-f]+)', 0),
                            'original-error': (r'^(allocation-after .*error=)([0-9]+)', 0),
                            'original-region': (r'^(allocation-region .*bytes=)([0-9a-f]+)', 0),
                            'original-helper-args': (r'^(allocation-helper-entry .*reserve=)([0-9a-f]+)', 0),
                            'original-helper-RET0': (r'^(allocation-helper-return .*esp=)([0-9a-f]+)', 0),
                            'original-helper-ESI': (r'^(allocation-helper-return .*esi=)([0-9a-f]+)', 0),
                        }[case]
                        def change(match):
                            words = match[2].split(',')
                            words[field] = str(int(words[field])+1) if case == 'original-error' else '%08x' % (int(words[field],16)+1)
                            return match[1]+','.join(words)
                        text, count = re.subn(pattern, change, text, count=1, flags=re.M)
                        if count != 1: raise ValueError('mutation did not match '+case)
                        f.write_text(text)
                    h.rehash(o, fname)
                elif case == 'native-zero-memory':
                    f = n/'work-committed-02.bin'; blob = bytearray(f.read_bytes()); blob[-1] = 1; f.write_bytes(blob); h.rehash(n, f.name)
                else:
                    fname = 'allocation-api-entries.json' if case in ('native-api-args','native-api-formal','native-api-PC','native-api-caller') else 'allocation-api-returns.json' if case.startswith('native-api-') or case in ('native-error','native-FPU','native-reserve-map','native-commit-map') else 'allocation-helper-entries.json' if case in ('native-helper-formal','native-helper-args') else 'allocation-helper-returns.json' if case.startswith('native-helper-') else 'reserve-return-01.json' if case == 'native-before-store' else 'state.json'
                    def change(j):
                        if case == 'native-api-args': j[0]['args'][1] += 1
                        elif case in ('native-api-formal','native-helper-formal'): j[0]['formal_inventory'].pop()
                        elif case == 'native-api-PC': j[0]['pc'] += 1
                        elif case == 'native-api-caller': j[0]['return_pc'] += 1
                        elif case in ('native-api-RET16','native-helper-RET0'): j[0]['esp'] += 4
                        elif case == 'native-api-EDI': j[0]['edi'] += 1
                        elif case == 'native-api-result': j[0]['eax'] += 4
                        elif case == 'native-error': j[0]['last_error'] = 0
                        elif case == 'native-FPU': j[1]['fpu'] = 0x37f
                        elif case == 'native-reserve-map': j[0]['maps'] = j[0]['maps'].replace('---p','rw-p')
                        elif case == 'native-commit-map': j[1]['maps'] = j[1]['maps'].replace('rw-p','---p')
                        elif case == 'native-helper-args': j[1]['args'][1] += 1
                        elif case == 'native-helper-EBX': j[0]['ebx'] += 1
                        elif case == 'native-helper-NULL': j[0]['eax'] = 0
                        elif case == 'native-helper-alias': j[1]['eax'] = j[0]['eax']
                        elif case == 'native-before-store': j['frontend']['work_buffers'][0][0] = 1
                        elif case == 'native-limit': j['frontend']['work_buffers'][1][1] += 1
                        elif case == 'native-cleanup-count': j['frontend']['cleanup_count'] -= 1
                        elif case == 'native-cleanup-callback': j['frontend']['cleanup_callbacks'][12] = 0
                        elif case == 'native-next-argv': j['args']['argv'] += 4
                        elif case == 'native-restore-neighbor': j['frontend']['source_signature'] ^= 1
                        else: raise ValueError(case)
                    h.edit(n/fname, change); h.rehash(n, fname)
                h.rejected(case, lambda: d.check(o, n, audit_path=audit))
                passed.append({'seed':row['seed'], 'rejected':case})
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'rejected':passed},indent=2))


if __name__ == '__main__': main()
