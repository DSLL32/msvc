#!/usr/bin/env python3
"""Affected numeric/public/continuation controls; immutable observations only."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import time

from integer_state import check


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original', type=Path)
    parser.add_argument('native', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    started = time.perf_counter()
    spec = importlib.util.spec_from_file_location('integer_control_decoder', Path(__file__).with_name('check-callback.py'))
    decoder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(decoder)
    original, native = args.original.resolve(), args.native.resolve()
    trial = json.loads((native / 'trial.json').read_text())
    state = json.loads((native / 'state.json').read_text())
    text = (original / 'events.txt').read_text()
    positive = check(decoder.s, decoder.d, original, native, trial, text, state, {})
    cases = ['numeric-argument', 'numeric-formal', 'numeric-caller', 'numeric-result',
             'numeric-return-stack', 'numeric-register', 'numeric-original-string',
             'API-argument', 'API-provider-PC', 'API-return-stack', 'API-register-EDI',
             'API-return-slot', 'API-original-mask', 'API-native-predicate', 'API-atol-result',
             'API-MB-domain', 'API-owner', 'API-FP', 'API-errors', 'API-unknown-original-data',
             'API-raw-provider-body', 'API-private-vector', 'numeric-whole-BSS',
             'warning-store', 'later-CRC-result', 'later-CRC-third-slot', 'later-CRC-seed',
             'fourth-iterator-owner', 'fourth-iterator-advance', 'fourth-matcher-callback',
             'suffix-pattern-owner', 'suffix-action', 'suffix-formal', 'suffix-caller',
             'suffix-pending-PC', 'suffix-original-stack', 'suffix-final-neighbor',
             'suffix-final-warning', 'whole-range']
    results = []
    with tempfile.TemporaryDirectory(prefix='integer-controls-', dir=original.parent) as temporary:
        root = Path(temporary)
        old, new = root / 'original', root / 'native'
        old.mkdir();new.mkdir()
        for destination, source in ((old, original), (new, native)):
            for item in source.iterdir():
                (destination / item.name).symlink_to(item, target_is_directory=item.is_dir())
        for case in cases:
            changed = []
            config, final, events = copy.deepcopy(trial), copy.deepcopy(state), text

            def write(directory, name, value):
                p = directory / name;p.unlink();p.write_bytes(value)
                changed.append((p, (original if directory == old else native) / name))

            def row(name, callback, index=0):
                values = json.loads((native / (name + '.json')).read_text())
                callback(values[index]);write(new, name + '.json', json.dumps(values).encode())

            def flip(directory, name, offset=0):
                values = bytearray(((original if directory == old else native) / name).read_bytes())
                values[offset] ^= 1;write(directory, name, values)

            if case == 'numeric-argument': row('integer-entries', lambda r:r['args'].__setitem__(0, r['args'][0] + 1))
            elif case == 'numeric-formal': row('integer-entries', lambda r:r['formal_inventory'][0].update(type='int'))
            elif case == 'numeric-caller': row('integer-entries', lambda r:r.update(caller='match_option'))
            elif case == 'numeric-result': row('integer-returns', lambda r:r.update(eax=2))
            elif case == 'numeric-return-stack': row('integer-returns', lambda r:r.update(esp=r['esp'] + 4))
            elif case == 'numeric-register': row('integer-returns', lambda r:r.update(edi=r['edi'] ^ 1))
            elif case == 'numeric-original-string': flip(old, 'integer-0-entry-input.bin')
            elif case == 'API-argument': row('integer-api-entries', lambda r:r['args'].__setitem__(0, 50))
            elif case == 'API-provider-PC':
                row('integer-api-entries', lambda r:r.update(pc=r['pc'] + 1))
                v=json.loads((native/'integer-native-providers.json').read_text());v['isspace']+=1;write(new,'integer-native-providers.json',json.dumps(v).encode())
            elif case == 'API-return-stack': row('integer-api-returns', lambda r:r.update(esp=r['esp'] + 4))
            elif case == 'API-register-EDI': row('integer-api-returns', lambda r:r.update(edi=r['edi'] ^ 1))
            elif case == 'API-return-slot': row('integer-api-returns', lambda r:r.update(argument_after=50))
            elif case == 'API-original-mask': flip(old, 'integer-0-api-isdigit-return-registers.bin', 12)
            elif case == 'API-native-predicate': row('integer-api-returns', lambda r:r.update(eax=0), 1)
            elif case == 'API-atol-result': row('integer-api-returns', lambda r:r.update(eax=2), 2)
            elif case == 'API-MB-domain': row('integer-api-entries', lambda r:r['locale'].update(mb_cur_max=2))
            elif case == 'API-owner': row('integer-api-entries', lambda r:r['locale'].update(pctype=r['locale']['pctype'] + 2))
            elif case == 'API-FP': row('integer-api-returns', lambda r:r['fp'].update(sw=r['fp']['sw'] ^ 1))
            elif case == 'API-errors': row('integer-api-returns', lambda r:r.update(last_error=0))
            elif case == 'API-unknown-original-data': flip(old, 'frontend-data-locale-integer-0-api-isspace-return.bin', 8000)
            elif case == 'API-raw-provider-body': flip(old, 'integer-provider-isspace.bin')
            elif case == 'API-private-vector': flip(new, 'locale-native-integer-0-api-isspace-return-CRT-argv.bin', -1)
            elif case == 'numeric-whole-BSS': row('integer-returns', lambda r:r['guard'].update(bss='01' + r['guard']['bss'][2:]))
            elif case == 'warning-store': row('actions-returns', lambda r:r['guard'].update(data='01' + r['guard']['data'][2:]), 2)
            elif case.startswith('later-CRC'):
                index=2 if trial.get('test_seed') else 3
                if case == 'later-CRC-result': row('actions-signature-returns', lambda r:r.update(eax=r['eax'] ^ 1), index)
                elif case == 'later-CRC-third-slot': row('actions-signature-returns', lambda r:r['argument_slots_after'].__setitem__(2, r['eax'] ^ 1), index)
                else:row('actions-signatures', lambda r:r['args'].__setitem__(2,r['args'][2] ^ 1), index)
            elif case == 'fourth-iterator-owner': row('actions-outer-returns', lambda r:r.update(eax=r['eax'] + 1), 3)
            elif case == 'fourth-iterator-advance': row('actions-outer-returns', lambda r:r['guard'].update(bss='01'+r['guard']['bss'][2:]),3)
            elif case == 'fourth-matcher-callback': row('actions-matches', lambda r:r['args'].__setitem__(2,r['args'][2] + 1),3)
            elif case == 'suffix-pattern-owner':
                row('integer-suffix-entries', lambda r:r['args'].__setitem__(0, r['args'][0] + 1));final['args']['pattern']+=1
            elif case == 'suffix-action': row('integer-suffix-entries', lambda r:r['args'].__setitem__(2, 36))
            elif case == 'suffix-formal': row('integer-suffix-entries', lambda r:r['formal_inventory'][2].update(type='void *'))
            elif case == 'suffix-caller': row('integer-suffix-entries', lambda r:r.update(caller='apply_option_action'))
            elif case == 'suffix-pending-PC': final['pc']+=128
            elif case == 'suffix-original-stack': events=events.replace('stack=104394cf,','stack=104394d0,')
            elif case == 'suffix-final-neighbor': final['frontend']['pending_group_head']^=1
            elif case == 'suffix-final-warning': final['frontend']['options_data']['warning_level']=2
            elif case == 'whole-range': config['ranges'][-1]['end_rva']+=1
            else: raise AssertionError(case)
            try:
                check(decoder.s, decoder.d, old, new, config, events, final, {})
            except (ValueError, AssertionError) as error:
                results.append(dict(case=case, rejection=str(error)))
            else:
                raise ValueError('numeric mutation accepted: '+case)
            for p, target in changed:
                p.unlink();p.symlink_to(target, target_is_directory=target.is_dir())
    result=dict(kind='fresh affected semantic decoder controls; no original/native execution',
                original=str(original),native=str(native),original_receipt_sha256=sha(original/'receipt.json'),native_receipt_sha256=sha(native/'receipt.json'),
                checker_sha256=sha(Path(__file__).with_name('integer_state.py')),control_sha256=sha(Path(__file__)),positive=positive,
                count=len(results),cases=results,seconds=time.perf_counter()-started)
    if args.output.exists():raise ValueError('refuse overwrite')
    args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=len(results),seconds=result['seconds'])))


if __name__=='__main__':main()
