#!/usr/bin/env python3
"""Fresh affected checksum/ABI/native-locale controls; no original execution."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import re
import tempfile
import time

from checksum_state import check as check_checksum


def load(name, filename):
    spec = importlib.util.spec_from_file_location(name, Path(__file__).with_name(filename))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original', type=Path)
    parser.add_argument('native', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    original, native = args.original.resolve(), args.native.resolve()
    decoder, history = load('checksum_decoder', 'check-callback.py'), load('checksum_history', 'check-history.py')
    bindings = {name + '_receipt_sha256': decoder.s.sha(path / 'receipt.json') for name, path in (('original', original), ('native', native))}
    audit = history.configure(decoder, original, native, saved_record=bindings)
    decoder.check(original, native, audit_path=audit)
    trial = json.loads((native / 'trial.json').read_text())
    state = json.loads((native / 'state.json').read_text())
    pre, phase = (json.loads((native / (name + '.json')).read_text()) for name in ('pre', 'phase'))
    text = (original / 'events.txt').read_text()
    cases = ('entry-PC', 'entry-formal', 'entry-table', 'output-table', 'output-neighbor', 'RET0', 'nonvolatile', 'FP', 'BSS-reset', 'final-table', 'next-formal', 'locale-category', 'MB-gate', 'LC-codepage', 'old-table', 'result-pointer', 'selection-count', 'binding-reference', 'bind-time-mutation', 'private-vector', 'layout', 'original-RET')
    results = []
    started = time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='checksum-controls-', dir=Path.home() / '.ai-tmp/msvc2/vc5.10/c1xx') as temporary:
        work = Path(temporary)
        for path in native.iterdir():
            (work / path.name).symlink_to(path, target_is_directory=path.is_dir())
        for case in cases:
            current_state, current_pre, current_phase, current_text = copy.deepcopy(state), copy.deepcopy(pre), copy.deepcopy(phase), text
            changed = []
            def write(name, value):
                path = work / name
                path.unlink()
                path.write_bytes(value)
                changed.append(path)
            def change(name, function):
                value = json.loads((work / ('checksum-' + name + '.json')).read_text())
                function(value)
                write('checksum-' + name + '.json', json.dumps(value).encode())
            def flip_hex(row, key, offset):
                raw = bytearray.fromhex(row[key])
                raw[offset] ^= 1
                row[key] = raw.hex()
            if case == 'entry-PC': change('entries', lambda v: v[0].update(pc=v[0]['pc'] + 1))
            elif case == 'entry-formal': change('entries', lambda v: v[0]['formal_inventory'].append(dict(name='invented', type='int')))
            elif case == 'entry-table': change('entries', lambda v: flip_hex(v[0], 'tables', 767))
            elif case == 'output-table': change('returns', lambda v: flip_hex(v[0], 'tables', 767))
            elif case == 'output-neighbor': change('returns', lambda v: flip_hex(v[0]['guard'], 'bss', 0))
            elif case == 'RET0': change('returns', lambda v: v[0].update(esp=v[0]['esp'] + 4))
            elif case == 'nonvolatile': change('returns', lambda v: v[0].update(esi=v[0]['esi'] ^ 4))
            elif case == 'FP': change('returns', lambda v: v[0]['fp'].update(cw=0x37f))
            elif case == 'BSS-reset': current_phase['frontend']['checksum_tables']['low'][255] ^= 1
            elif case == 'final-table': current_state['frontend']['checksum_tables']['update'][255] ^= 1
            elif case == 'next-formal': change('next-entries', lambda v: v[0]['formal_inventory'].append(dict(name='invented', type='int')))
            elif case == 'locale-category': change('locale-entries', lambda v: v[0]['args'].__setitem__(0, 3))
            elif case == 'MB-gate': change('locale-returns', lambda v: v[0]['locale'].update(mb_gate=1))
            elif case == 'LC-codepage': change('locale-returns', lambda v: v[0]['locale'].update(lc_codepage=1251))
            elif case == 'old-table': change('locale-returns', lambda v: flip_hex(v[0]['locale'], 'old_ctype', 513))
            elif case == 'result-pointer': change('locale-returns', lambda v: v[0]['locale'].update(result_pointer=v[0]['locale']['result_pointer'] + 1))
            elif case == 'selection-count': change('locale-returns', lambda v: v[0]['locale']['selection'].update(calls=2))
            elif case == 'binding-reference': change('binding-entries', lambda v: v[0]['args'].__setitem__(1, '0' * 64))
            elif case == 'bind-time-mutation': change('binding-returns', lambda v: v[0]['locale'].update(lc_codepage=1252))
            elif case == 'private-vector':
                name = 'locale-native-after-CRT-environment.bin'
                blob = bytearray((work / name).read_bytes());blob[-2] ^= 1;write(name, blob)
            elif case == 'layout': current_state['frontend']['checksum_native_layout']['size'] += 4
            elif case == 'original-RET': current_text = re.sub(r'(checksum-return pc=\w+ esp=)(\w+)', lambda m: m[1] + '%08x' % (int(m[2], 16) + 4), text)
            traps = re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$', current_text, re.M)
            try:
                check_checksum(decoder.s, decoder.d, original, work, trial, current_text, current_pre, current_phase, current_state, trial.get('test_seed'), '%03d' % int(traps[-1][0]), traps)
            except (ValueError, AssertionError, KeyError) as error:
                results.append(dict(case=case, rejection=str(error)))
            else:
                raise RuntimeError('mutant accepted: ' + case)
            finally:
                for path in changed:
                    path.unlink()
                    path.symlink_to(native / path.name)
    assert not args.output.exists()
    result = dict(kind='fresh affected checksum/native API semantic denials after full strict positive; no original execution', bindings=bindings, original_directory=str(original), native_directory=str(native), cases=results, count=len(results), seconds=time.perf_counter() - started, checker_sha256=decoder.s.sha(Path(__file__)), semantic_checker_sha256=decoder.s.sha(Path(__file__).with_name('checksum_state.py')))
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(count=len(results), seconds=result['seconds'])))


if __name__ == '__main__':
    main()
