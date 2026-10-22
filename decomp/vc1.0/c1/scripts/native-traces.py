#!/usr/bin/env python3
"""Project frozen original acceptance histories onto native application entries.

Cache generation reads metadata and an existing original-toolchain symbol map;
it never executes originals or captures references. Runtime/CRT/import/shim
vertices are excluded on both sides. Unknown application definitions fail.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

BINARY = Path(__file__).resolve().parents[1]
CACHE = BINARY / 'cases/native-traces.json'
# CRT variants reconstructed outside runtime.c retain their CRT classification.
EXTRA_CRT = {'___strgtold12', '__unlink'}
CRT_NATIVE_ALIASES = {'recon_unlink':'__unlink'}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def native_name(logical):
    assert logical[0] in '_@', 'unknown C decoration: ' + logical
    name = re.sub(r'@\d+$', '', logical[1:])
    assert re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', name), logical
    return name


def application_symbols(directory):
    objects = sorted(directory.glob('*.o'))
    assert objects, 'native objects missing'
    objects = [p for p in objects if p.name != 'runtime.o']
    output = subprocess.check_output(['nm', '-A', '--defined-only', '--format=posix',
                                      *map(str, objects)], text=True)
    excluded = {native_name(n) for n in EXTRA_CRT} | set(CRT_NATIVE_ALIASES)
    definitions = {}
    for line in output.splitlines():
        row = line.split()
        if len(row) < 4 or row[2] not in ('T', 't', 'W', 'w'):
            continue
        name = row[1]
        if name in excluded:
            continue
        assert name not in definitions, 'duplicate native definition: ' + name
        definitions[name] = 'src/' + Path(row[0][:-1]).stem + '.c'
    return definitions


def project(directory):
    config_path, runtime_path = BINARY/'functions.json', BINARY/'runtime.json'
    config = json.loads(config_path.read_text())
    runtime = {r['name'] for r in json.loads(runtime_path.read_text())}
    owners = {}
    map_path = BINARY/'build/c1.map'
    for line in map_path.read_text().splitlines():
        match = re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if match:
            owner = line.split()[-1]
            if owner.endswith('.obj'):
                owners[match[1]] = 'src/' + Path(owner).stem + '.c'
    drivers = {r['name']:r for r in config['driver']}
    assert len(drivers) == len(config['driver'])
    assert all(n in owners for n in drivers), 'source-backed Win32 symbol missing'
    crt = {n for n in drivers if owners[n] == 'src/runtime.c'} | EXTRA_CRT
    app = {n:r for n,r in drivers.items() if n not in crt}
    native = {native_name(n):dict(logical=n, source=owners[n]) for n in app}
    assert len(native) == len(app), 'native decoration collision'
    actual = application_symbols(directory)
    assert actual == {n:r['source'] for n,r in native.items()}, (
        'unresolved native application definitions',
        sorted(actual.keys()-native.keys()), sorted(native.keys()-actual.keys()))
    classified = {}
    for name in config['accepted']:
        if name in app:
            classified[name] = 'application'
        elif name.startswith('import:'):
            classified[name] = 'import'
        elif name in crt or name in runtime:
            classified[name] = 'CRT'
        else:
            raise AssertionError('unclassified original entry: ' + name)
    accepted = config['accepted']
    projected = lambda rows: [native_name(n) for n in rows if classified[n] == 'application']
    cases = {}
    for name, case in config['cases'].items():
        first, last = case['accepted_from'], case['accepted_count']
        cases[name] = dict(first=first, last=last,
                           seen_count=len(projected(accepted[:first])),
                           expected=projected(accepted[first:last]),
                           terminal=dict(kind='exit', code=0))
    return dict(schema=1, description='Original-backed first-entry projection; no execution or recapture',
                metadata_sha256={'functions.json':sha(config_path), 'runtime.json':sha(runtime_path)},
                source_map_provenance=dict(path='build/c1.map', sha256=sha(map_path)),
                explicit_CRT_variants=sorted(EXTRA_CRT), native_CRT_aliases=CRT_NATIVE_ALIASES,
                application_definitions={n:native[n] for n in sorted(native)},
                accepted_application=projected(accepted),
                excluded_accepted={kind:[n for n in accepted if classified[n] == kind]
                                   for kind in ('CRT', 'import')}, cases=cases)


def load(directory, path=CACHE):
    cache = json.loads(path.read_text())
    for name, digest in cache['metadata_sha256'].items():
        assert sha(BINARY/name) == digest, 'cached original projection is stale: ' + name
    expected = {n:r['source'] for n,r in cache['application_definitions'].items()}
    assert application_symbols(directory) == expected, 'new/missing/unmapped native application definition'
    return cache


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--objects', type=Path, default=BINARY/'build/native-trace')
    parser.add_argument('--write', action='store_true')
    args = parser.parse_args()
    if args.write:
        cache = project(args.objects)
        CACHE.write_text(json.dumps(cache, indent=2) + '\n')
    else:
        cache = load(args.objects)
    print('application definitions:', len(cache['application_definitions']))
    print('original case segments:', len(cache['cases']))
