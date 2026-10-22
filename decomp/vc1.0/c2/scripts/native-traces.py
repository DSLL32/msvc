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


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write', action='store_true')
    parser.add_argument('--objects', type=Path, default=BINARY/'build/native-trace')
    args = parser.parse_args()
    if args.write:
        result = project(args.objects)
        CACHE.write_text(json.dumps(result, indent=2)+'\n')
    else:
        result = load(args.objects)
    print(len(result['application_definitions']), 'application definitions;', len(result['cases']), 'cached cases')
