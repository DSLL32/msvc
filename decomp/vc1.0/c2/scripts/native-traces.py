"""Project captured original C2 histories onto audited native application names.

Writing the cache consumes existing original-backed replay receipts, never
executes or recaptures a Windows program. Ordinary fixture checks only load it.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

BINARY = Path(__file__).resolve().parents[1]
CACHE = BINARY / 'cases/native-traces.json'
CRT = {'_ftell', '_fseek', '__sopen', '__filbuf', '__freebuf'}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def native_name(name):
    assert name[0] in '_@', name
    result = re.sub(r'@\d+$', '', name[1:])
    assert re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', result), name
    return result


def definitions(directory):
    objects = sorted(p for p in directory.glob('*.o') if p.name != 'runtime.o')
    assert objects, 'missing native application objects'
    output = subprocess.check_output(['nm', '-A', '--defined-only', '--format=posix',
                                     *map(str, objects)], text=True)
    result = {}
    for line in output.splitlines():
        fields = line.split()
        if len(fields) < 4 or fields[2] not in ('T', 't', 'W', 'w'):
            continue
        name, obj = fields[1], Path(fields[0][:-1])
        assert name not in result, 'duplicate application definition: ' + name
        result[name] = 'src/' + obj.stem + '.c'
    return result


def project(directory):
    config = json.loads((BINARY/'functions.json').read_text())
    drivers = {r['name']: r for r in config['driver']}
    runtime = {r['name'] for r in json.loads((BINARY/'runtime.json').read_text())}
    application = set(drivers) - CRT
    native = {native_name(name): name for name in application}
    assert len(native) == len(application), 'decoration collision'
    actual = definitions(directory)
    assert actual.keys() == native.keys(), ('unresolved application definitions',
                                            actual.keys()-native.keys(), native.keys()-actual.keys())
    accepted = config['accepted']
    for name in accepted:
        assert name in application or name in CRT or name in runtime or name.startswith('import:'), name
    projected = lambda names: [native_name(n) for n in names if n in application]
    cases, provenance = {}, {}
    for profile, case in config['cases'].items():
        receipt_path = Path.home()/'.ai-tmp/vc1.0-c2'/('fast-adoption-continuous-'+profile+'.json')
        receipt = json.loads(receipt_path.read_text())
        first = case.get('accepted_from', 0)
        last = case.get('accepted_count', len(accepted))
        assert receipt['first'] == first and receipt['last'] == last
        original = receipt['sides'][0]
        assert original['expected'] == accepted[first:last]
        original_path = str((BINARY.parents[2]/'toolchains/1.0-nt/bin/C23232.EXE').resolve())
        assert original['frozen'][original_path] == config['target']['sha256']
        observed = [e['name'] for e in original['events'] if e['event'] == 'frontier']
        assert observed == accepted[first:last], 'cached original transcript differs'
        terminal = receipt['terminal']
        assert [e['name'] for e in original['events'] if e['event'] == 'terminal'] == [terminal]
        expected = projected(observed)
        if not terminal.startswith('exit:'):
            assert terminal in application and drivers[terminal].get('provisional'), terminal
            expected.append(native_name(terminal))
        cases[profile] = dict(first=first, last=last, seen=projected(accepted[:first]),
                              expected=expected, terminal=terminal,
                              returncode=int(terminal[5:]) if terminal.startswith('exit:') else -5)
        provenance[profile] = dict(path=str(receipt_path), sha256=sha(receipt_path),
                                   original_target_sha256=config['target']['sha256'],
                                   original_logical_entries=observed)
    return dict(schema=1, description='Cached original application-entry projection; no capture or Wine',
                metadata_sha256={n:sha(BINARY/n) for n in ('functions.json', 'runtime.json')},
                application_definitions={n:dict(logical=native[n], source=actual[n]) for n in sorted(native)},
                excluded_crt=sorted(CRT), provenance=provenance, cases=cases)


def load(directory):
    cache = json.loads(CACHE.read_text())
    for path, digest in cache['metadata_sha256'].items():
        assert sha(BINARY/path) == digest, 'stale original trace projection: ' + path
    assert definitions(directory) == {n:r['source'] for n,r in cache['application_definitions'].items()}, 'unmapped application definitions'
    return cache


def check_runtime_evidence(path, name, before, record):
    """Validate an explicit library seam observation; never execute a reference."""
    from binary import PE, COFF, archive
    evidence = json.loads(path.read_text())
    assert evidence['logical'] == name
    assert evidence['before_config_sha256'] == before['config_sha256']
    assert evidence['target_sha256'] == before['target_sha256']
    identity = evidence['original']['identity']
    assert identity['record'] == record
    library = BINARY.parents[2]/'toolchains/1.0-nt/lib'/record['library']
    assert sha(library) == identity['library_sha256']
    data = next(data for obj, data in archive(library) if obj == record['object'])
    assert hashlib.sha256(data).hexdigest() == identity['object_sha256']
    text = next(s for s in COFF(data).sections if s['flags'] & 0x20)
    assert not text['relocs'], 'this seam evidence requires a relocation-free CRT body'
    original = PE(BINARY.parents[2]/'toolchains/1.0-nt/bin/C23232.EXE')
    assert hashlib.sha256(original.data).hexdigest() == before['target_sha256']
    assert bytes(original.read(record['start'], record['end']-record['start'])) == text['data']
    assert hashlib.sha256(text['data']).hexdigest() == identity['text_sha256']
    assert len(text['data']) == identity['exact_text_bytes']
    assert name == '_strncmp', 'new CRT seams require their own observation validator'
    left, right = evidence['observed_original'], evidence['observed_native']
    assert {k:left[k] for k in ('left_hex','right_hex','count')} == {k:right[k] for k in ('left_hex','right_hex','count')}
    assert evidence['caller_result_use'] == 'zero/nonzero'
    assert (left['result'] == 0) == (right['result'] == 0), 'CRT caller predicate differs'
    return dict(path=str(path), sha256=sha(path), evidence=evidence)


def extend(directory, before_path, after_path, runtime_evidence=None):
    """Accept one original-only before/after capture pair, outside fixtures."""
    cache = json.loads(CACHE.read_text())
    before, after = (json.loads(path.read_text()) for path in (before_path, after_path))
    config = json.loads((BINARY/'functions.json').read_text())
    accepted = config['accepted']
    old_count = before['accepted']
    assert after['accepted'] == len(accepted) == old_count+1, 'extension must accept exactly one function'
    assert before['config_sha256'] == cache['metadata_sha256']['functions.json'], 'before capture is not the cached checkpoint'
    assert sha(BINARY/'runtime.json') == cache['metadata_sha256']['runtime.json'], 'runtime classification changed during one-function extension'
    assert after['config_sha256'] == sha(BINARY/'functions.json'), 'after capture is not current code metadata'
    name = accepted[old_count]
    # An untyped naked stub can acquire its recovered calling convention at
    # acceptance. Require the same C identity and exact original entry below;
    # only the decoration changes, never the function being accepted.
    assert native_name(before['logical']) == native_name(name), 'wrong original entry before acceptance'
    drivers = {r['name']:r for r in config['driver']}
    runtime = {r['name']:r for r in json.loads((BINARY/'runtime.json').read_text())}
    records = dict(runtime, **drivers)
    application_names = set(drivers)-CRT
    assert name in records, 'unknown accepted original function'
    assert before['original_va'] == hex(records[name]['start'])
    terminal = after['logical']
    completed = terminal == 'exit:0'
    if completed:
        assert after['original_va'] is None and after['events'] == 'exit 0\n'
    else:
        assert terminal in records, 'unknown next original entry'
        if terminal in application_names:
            assert drivers[terminal].get('provisional'), 'next application entry must remain a named stub'
        else:
            assert terminal in runtime, 'next nonapplication entry must have indexed CRT identity'
        assert after['original_va'] == hex(records[terminal]['start'])
        assert after['logical'] not in accepted
    case = cache['cases']['hello']
    assert case['last'] == old_count and case['terminal'] == before['logical']
    if name in application_names:
        assert case['expected'][-1] == native_name(name)
        assert runtime_evidence is None, 'runtime evidence supplied for application acceptance'
        library_evidence = None
    else:
        assert runtime_evidence is not None, 'CRT acceptance requires explicit library and call evidence'
        library_evidence = check_runtime_evidence(runtime_evidence, name, before, records[name])
    manifest = BINARY/config['cases']['hello']['fixture_manifest']
    for receipt in (before, after):
        assert receipt['target_sha256'] == config['target']['sha256']
        assert receipt['manifest_sha256'] == sha(manifest)
        phase = json.loads(manifest.read_text())['phases']['c2']
        assert receipt['inputs'] == {r['path']:r['sha256'] for r in phase['inputs']}
        assert receipt['environment'] == phase['environment']
        if receipt is after and completed:
            outputs = receipt['outputs']
            assert len(outputs) == len(phase['outputs'])
            for artifact, expected in zip(outputs, phase['outputs']):
                data = bytearray((BINARY/artifact['path']).read_bytes())
                reference = bytearray((manifest.parent/expected['path']).read_bytes())
                assert sha(manifest.parent/expected['path']) == expected['sha256']
                assert len(data) == artifact['bytes'] and hashlib.sha256(data).hexdigest() == artifact['sha256']
                assert artifact['ignored_timestamp'] == [4,4]
                fields = expected['ignored_fields']
                assert len(fields) == 1 and (fields[0]['offset'],fields[0]['bytes']) == (4,4)
                data[4:8] = reference[4:8] = b'\0'*4
                assert data == reference, 'completed original object differs beyond timestamp'
                assert hashlib.sha256(data).hexdigest() == artifact['normalized_sha256'] == expected['normalized_sha256']
        else:
            event = re.fullmatch(r'exception 80000003 rva ([0-9a-f]+) base ([0-9a-f]+) first 1\nexit 99\n', receipt['events'])
            assert event and int(event[1],16)+int(event[2],16) == int(receipt['original_va'],16), 'reference trap address differs from named entry'
    actual = definitions(directory)
    application = {native_name(n):n for n in drivers if n not in CRT}
    assert actual.keys() == application.keys(), 'unresolved native application identity'
    cache['application_definitions'] = {n:dict(logical=application[n], source=actual[n]) for n in sorted(actual)}
    case['last'] = len(accepted)
    case['terminal'] = after['logical']
    case['terminal_kind'] = 'exit' if completed else ('application' if terminal in application_names else 'runtime-library')
    case['returncode'] = 0 if completed else -5
    if terminal in application_names:
        case['expected'].append(native_name(terminal))
    cache['metadata_sha256'] = {n:sha(BINARY/n) for n in ('functions.json','runtime.json')}
    cache['provenance']['hello'].setdefault('extensions', []).append(dict(
        accepted_function=name, accepted_kind='application' if name in application_names else 'runtime-library',
        previous_logical=before['logical'],
        original_extent=[hex(records[name]['start']),hex(records[name]['end'])],
        before=dict(path=str(before_path), sha256=sha(before_path), capture=before),
        after=dict(path=str(after_path), sha256=sha(after_path), capture=after)))
    if library_evidence is not None:
        cache['provenance']['hello']['extensions'][-1]['library_evidence'] = library_evidence
    return cache


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write', action='store_true')
    parser.add_argument('--objects', type=Path, default=BINARY/'build/native-trace')
    parser.add_argument('--extend-before', type=Path, help='explicit original-only receipt before one-function acceptance')
    parser.add_argument('--extend-after', type=Path, help='explicit original-only receipt after one-function acceptance')
    parser.add_argument('--runtime-evidence', type=Path, help='explicit indexed CRT body/call observations for a library acceptance')
    args = parser.parse_args()
    if args.extend_before or args.extend_after:
        assert args.extend_before and args.extend_after and not args.write
        result = extend(args.objects, args.extend_before, args.extend_after, args.runtime_evidence)
        CACHE.write_text(json.dumps(result,indent=2)+'\n')
    elif args.write:
        result = project(args.objects)
        CACHE.write_text(json.dumps(result, indent=2)+'\n')
    else:
        result = load(args.objects)
    print(len(result['application_definitions']), 'application definitions;', len(result['cases']), 'cached cases')
