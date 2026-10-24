#!/usr/bin/env python3
"""Focused option-loop/API controls against immutable data, no execution."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import time

from option_action_state import check


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original', type=Path)
    parser.add_argument('native', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    started = time.perf_counter()
    spec = importlib.util.spec_from_file_location('action_control', Path(__file__).with_name('check-callback.py'))
    decoder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(decoder)
    original, native = args.original.resolve(), args.native.resolve()
    trial = json.loads((native / 'trial.json').read_text())
    state = json.loads((native / 'state.json').read_text())
    text = (original / 'events.txt').read_text()
    positive = check(decoder.s, decoder.d, original, native, trial, text, state, {})
    cases = [
        'outer-count', 'outer-formal', 'outer-caller', 'outer-return-value',
        'outer-return-stack', 'outer-return-register', 'outer-return-string',
        'match-rule', 'match-argument', 'match-callback', 'match-copy', 'match-formal',
        'match-result', 'match-return-stack', 'inner-return-value', 'inner-return-string',
        'inner-return-register', 'inner-caller', 'action-rule', 'action-value',
        'action-copy', 'action-formal', 'action-caller', 'action-input-string',
        'action-return-data', 'action-return-BSS', 'action-return-owner',
        'action-original-unknown-data', 'action-original-snapshot',
        'action-original-borrowed-value', 'later-phase-owner', 'later-phase-FP',
        'later-private-vector', 'integer-value', 'integer-formal', 'integer-caller',
        'integer-PC', 'integer-original-input', 'final-source-file', 'final-il-prefix',
        'final-neighbor', 'table-range', 'table-entry', 'table-byte',
        'API-seed', 'API-input-count', 'API-input-owner', 'API-result',
        'API-third-slot', 'API-first-slot', 'API-return-stack', 'API-return-register',
        'API-owner', 'API-FP', 'API-body', 'API-table', 'API-input-byte',
        'API-original-third-slot', 'API-committed-neighbor', 'API-native-vector',
    ]
    if not trial.get('test_seed'):
        cases += ['initial-CPP-index-bridge', 'initial-CPP-body']
    results = []
    with tempfile.TemporaryDirectory(prefix='option-action-controls-', dir=Path.home() / '.ai-tmp/msvc2/vc5.10/c1xx') as temporary:
        root = Path(temporary)
        old, new = root / 'original', root / 'native'
        old.mkdir()
        new.mkdir()
        for destination, source in ((old, original), (new, native)):
            for item in source.iterdir():
                (destination / item.name).symlink_to(item, target_is_directory=item.is_dir())
        for case in cases:
            changed = []
            config, final = copy.deepcopy(trial), copy.deepcopy(state)

            def write(directory, name, data):
                path = directory / name
                path.unlink()
                path.write_bytes(data)
                changed.append((path, (original if directory == old else native) / name))

            def js(name, fn, index=0):
                value = json.loads((native / name).read_text())
                fn(value[index])
                write(new, name, json.dumps(value).encode())

            def flip(directory, name, at=0):
                data = bytearray(((original if directory == old else native) / name).read_bytes())
                data[at] ^= 1
                write(directory, name, data)

            def owner(row):
                row['locale']['pctype'] += 2

            if case == 'outer-count':
                value = json.loads((native / 'actions-outer-entries.json').read_text())[:-1]
                write(new, 'actions-outer-entries.json', json.dumps(value).encode())
            elif case == 'outer-formal': js('actions-outer-entries.json', lambda r: r.update(formal_inventory=[{'name': 'hidden', 'type': 'int'}]), 1)
            elif case == 'outer-caller': js('actions-outer-entries.json', lambda r: r.update(caller='match_option'), 1)
            elif case == 'outer-return-value': js('actions-outer-returns.json', lambda r: r.update(eax=r['eax'] + 1), 1)
            elif case == 'outer-return-stack': js('actions-outer-returns.json', lambda r: r.update(esp=r['esp'] + 4), 1)
            elif case == 'outer-return-register': js('actions-outer-returns.json', lambda r: r.update(ebp=r['ebp'] ^ 1), 1)
            elif case == 'outer-return-string': js('actions-outer-returns.json', lambda r: r.update(value='00'), 1)
            elif case.startswith('match-') and case[6:] in ('rule', 'argument', 'callback', 'copy'):
                field = ['rule', 'argument', 'callback', 'copy'].index(case[6:])
                js('actions-matches.json', lambda r: r['args'].__setitem__(field, r['args'][field] ^ 1), 1)
            elif case == 'match-formal': js('actions-matches.json', lambda r: r['formal_inventory'][2].update(type='void *'), 1)
            elif case == 'match-result': js('actions-match-returns.json', lambda r: r.update(eax=0), 1)
            elif case == 'match-return-stack': js('actions-match-returns.json', lambda r: r.update(esp=r['esp'] + 4), 1)
            elif case == 'inner-return-value': js('actions-inner-returns.json', lambda r: r.update(eax=r['eax'] + 1), 1)
            elif case == 'inner-return-string': js('actions-inner-returns.json', lambda r: r.update(value='00'), 1)
            elif case == 'inner-return-register': js('actions-inner-returns.json', lambda r: r.update(esi=r['esi'] ^ 1), 1)
            elif case == 'inner-caller': js('actions-inner-entries.json', lambda r: r.update(caller='process_compiler_options'), 1)
            elif case.startswith('action-') and case[7:] in ('rule', 'value', 'copy'):
                field = ['rule', 'value', 'copy'].index(case[7:])
                js('actions-entries.json', lambda r: r['args'].__setitem__(field, r['args'][field] ^ 1), 1)
            elif case == 'action-formal': js('actions-entries.json', lambda r: r['formal_inventory'][2].update(type='void *'), 1)
            elif case == 'action-caller': js('actions-entries.json', lambda r: r.update(caller='process_compiler_options'), 1)
            elif case == 'action-input-string': js('actions-entries.json', lambda r: r.update(value='00'), 1)
            elif case in ('action-return-data', 'action-return-BSS'):
                kind = 'data' if case.endswith('data') else 'bss'
                js('actions-returns.json', lambda r: r['guard'].update({kind: '01' + r['guard'][kind][2:]}), 1)
            elif case == 'action-return-owner': js('actions-returns.json', owner, 1)
            elif case == 'action-original-unknown-data': flip(old, 'frontend-data-locale-actions-action-return-1.bin', 8000)
            elif case == 'action-original-snapshot': flip(old, 'frontend-snapshot-locale-actions-action-return-1.bin', 8000)
            elif case == 'action-original-borrowed-value': flip(old, 'actions-inner-1-value.bin')
            elif case == 'later-phase-owner': js('actions-matches.json', owner, 2)
            elif case == 'later-phase-FP': js('actions-matches.json', lambda r: r['fp'].update(sw=r['fp']['sw'] ^ 1), 2)
            elif case == 'later-private-vector': flip(new, 'locale-native-actions-match-entry-2-CRT-argv.bin', -1)
            elif case == 'integer-value': js('actions-integer-entries.json', lambda r: r['args'].__setitem__(0, r['args'][0] + 1))
            elif case == 'integer-formal': js('actions-integer-entries.json', lambda r: r['formal_inventory'][0].update(type='int'))
            elif case == 'integer-caller': js('actions-integer-entries.json', lambda r: r.update(caller='match_option'))
            elif case == 'integer-PC': js('actions-integer-entries.json', lambda r: r.update(pc=r['pc'] + 1))
            elif case == 'integer-original-input':
                text_case = re_sub_integer(text)
            elif case == 'final-source-file': final['frontend']['options_bss']['source_file'] += 4
            elif case == 'final-il-prefix': final['frontend']['options_data']['il_prefix'] += 4
            elif case == 'final-neighbor': final['frontend']['known_sizes']['bss'] += 4
            elif case == 'table-range': config['ranges'][-1]['end_rva'] += 1
            elif case == 'table-entry': config['ranges'][-1]['entry'] = 1
            elif case == 'table-byte': flip(old, 'relocated-text.bin', 0x39710 - 0x1000)
            elif case.startswith('API-'):
                index = len(json.loads((native / 'actions-signatures.json').read_text())) - 1
                field = case[4:]
                if field in ('seed', 'input-count', 'input-owner'):
                    slot = {'seed': 2, 'input-count': 1, 'input-owner': 0}[field]
                    js('actions-signatures.json', lambda r: r['args'].__setitem__(slot, r['args'][slot] ^ 1), index)
                elif field == 'result': js('actions-signature-returns.json', lambda r: r.update(eax=r['eax'] ^ 1), index)
                elif field in ('third-slot', 'first-slot'):
                    slot = 2 if field == 'third-slot' else 0
                    js('actions-signature-returns.json', lambda r: r['argument_slots_after'].__setitem__(slot, r['argument_slots_after'][slot] ^ 1), index)
                elif field == 'return-stack': js('actions-signature-returns.json', lambda r: r.update(esp=r['esp'] + 4), index)
                elif field == 'return-register': js('actions-signature-returns.json', lambda r: r.update(edi=r['edi'] ^ 1), index)
                elif field == 'owner': js('actions-signature-returns.json', owner, index)
                elif field == 'FP': js('actions-signature-returns.json', lambda r: r['fp'].update(tag=0), index)
                elif field == 'body': flip(old, 'actions-signature-%d-provider-body.bin' % index, 1)
                elif field == 'table': flip(old, 'actions-signature-%d-return-table.bin' % index, 100)
                elif field == 'input-byte': flip(old, 'actions-signature-%d-entry-input.bin' % index)
                elif field == 'original-third-slot': flip(old, 'actions-signature-%d-return-registers.bin' % index, 14 * 4)
                elif field == 'committed-neighbor': flip(old, 'actions-signature-%d-return-vector.bin' % index, -1)
                elif field == 'native-vector': js('actions-signature-returns.json', lambda r: r.update(vector='01' + r['vector'][2:]), index)
                else: raise ValueError(field)
            elif case == 'initial-CPP-index-bridge': js('actions-signatures.json', lambda r: r.update(esp=r['esp'] + 4))
            elif case == 'initial-CPP-body': flip(old, 'actions-signature-0-provider-body.bin', 1)
            else: raise ValueError(case)
            try:
                check(decoder.s, decoder.d, old, new, config,
                      text_case if case == 'integer-original-input' else text, final, {})
            except (ValueError, AssertionError, KeyError) as error:
                results.append(dict(case=case, rejection=str(error)))
            else:
                raise ValueError('mutation accepted: ' + case)
            finally:
                for path, source in changed:
                    path.unlink()
                    path.symlink_to(source)
    result = dict(kind='focused action-loop/public-call semantic controls; no binary execution',
                  original=str(original), native=str(native), count=len(results), cases=results,
                  original_receipt_sha256=decoder.s.sha(original / 'receipt.json'),
                  native_receipt_sha256=decoder.s.sha(native / 'receipt.json'),
                  checker_sha256=decoder.s.sha(Path(__file__).with_name('option_action_state.py')),
                  control_sha256=decoder.s.sha(Path(__file__)), positive=positive,
                  seconds=time.perf_counter() - started)
    if args.output.exists(): raise ValueError('refuse overwrite')
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(count=len(results), seconds=result['seconds'])))


def re_sub_integer(text):
    import re
    pattern = r'(^trap seq=\d+ rva=00039779 .*stack=[0-9a-f]+,)([0-9a-f]+)'
    result, count = re.subn(pattern, lambda m: m[1] + '%08x' % (int(m[2], 16) ^ 1), text, flags=re.M)
    if count != 1: raise ValueError('integer raw trap absent')
    return result


if __name__ == '__main__':
    main()
