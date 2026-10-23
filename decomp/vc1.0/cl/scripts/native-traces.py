#!/usr/bin/env python3
"""Project cached original first-entry segments onto native application names.

No execution or reference capture. Native object definitions must all resolve to
source-backed driver identities. CRT overrides and explicit runtime/import graph
vertices are classified separately on the reference side.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

BINARY = Path(__file__).resolve().parents[1]
CACHE = BINARY / 'cases/native-traces.json'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def native_name(logical):
    assert logical and logical[0] in '_@', 'unrecognized C decoration: '+logical
    result = re.sub(r'@\d+$', '', logical[1:])
    assert re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', result), logical
    return result


def application_symbols(object_directory):
    objects = sorted(object_directory.glob('*.o'))
    assert objects, 'native object files are missing'
    objects = [p for p in objects if p.name != 'runtime.o']
    output = subprocess.check_output(['nm', '-A', '--defined-only', '--format=posix',
                                      *map(str, objects)], text=True)
    definitions = {}
    for line in output.splitlines():
        parts = line.split()
        if len(parts)<4 or parts[2] not in ('T','t','W','w'):
            continue
        symbol, obj = parts[1], Path(parts[0][:-1])
        assert symbol not in definitions, 'duplicate application definition: '+symbol
        definitions[symbol] = 'src/'+obj.stem+'.c'
    return definitions


def project(object_directory,verify_capture=True):
    config_path, runtime_path = BINARY/'functions.json', BINARY/'runtime.json'
    config = json.loads(config_path.read_text())
    runtime = {r['name'] for r in json.loads(runtime_path.read_text())}
    drivers = {r['name']:r for r in config['driver']}
    assert len(drivers)==len(config['driver'])
    app = {n:r for n,r in drivers.items() if r['start'] < 0x1b081}
    native = {native_name(n):r for n,r in app.items()}
    assert len(native)==len(app), 'application decoration collision'
    definitions = application_symbols(object_directory)
    assert definitions.keys()==native.keys(), ('unresolved application definitions',
        sorted(definitions.keys()-native.keys()), sorted(native.keys()-definitions.keys()))
    for n,source in definitions.items():
        native[n]['source'] = source
    accepted = config['accepted']
    assert len(accepted)==len(set(accepted))
    classified = {}
    for name in accepted:
        if name in app:
            classified[name] = 'application'
        elif name.startswith('import:'):
            assert re.fullmatch(r'import:[A-Za-z_][A-Za-z_0-9]*',name)
            classified[name] = 'import'
        elif name in runtime or (name in drivers and
                drivers[name]['start'] >= 0x1b081):
            classified[name] = 'CRT'
        else:
            raise AssertionError('unresolved accepted identity: '+name)
    cases = {}
    for case,record in config['cases'].items():
        first,last = record.get('accepted_from',0),record['accepted_count']
        assert 0 <= first <= last <= len(accepted), case
        projected = lambda names:[native_name(n) for n in names if classified[n]=='application']
        case_id = record.get('catalog_id',case)
        cases[case_id] = dict(first=first,last=last,seen_count=len(projected(accepted[:first])),
                             expected=projected(accepted[first:last]),terminal=dict(kind='exit'))
    capture_hashes={}
    if verify_capture:
        for case_id,segment in cases.items():
            reference_path=BINARY/'cases/native-boundaries'/(case_id+'.json')
            reference=json.loads(reference_path.read_text())
            proof=reference['original_entry_proof']
            assert proof['seen_prefix']==projected(accepted[:segment['first']]), 'original entry prefix mismatch'
            assert proof['entries']==segment['expected'], 'original entry capture changed'
            capture_hashes['cases/native-boundaries/'+case_id+'.json']=sha(reference_path)
    return dict(schema=2,original_capture_sha256=capture_hashes,description='Projection of existing original-backed acceptance metadata; no recapture',
                metadata_sha256={'functions.json':sha(config_path),'runtime.json':sha(runtime_path)},
                application_definitions={n:dict(source=native[n]['source'],
                    logical=next(k for k in app if native_name(k)==n),
                    original_extent=[native[n]['start'],native[n]['end']]) for n in sorted(native)},
                accepted_application=[native_name(n) for n in accepted if classified[n]=='application'],
                excluded_accepted={kind:[n for n in accepted if classified[n]==kind]
                                   for kind in ('CRT','import')},cases=cases)


def load(object_directory, cache_path=CACHE):
    cache = json.loads(cache_path.read_text())
    for name,digest in cache['metadata_sha256'].items():
        assert sha(BINARY/name)==digest, 'native trace cache stale: '+name
    for name,digest in cache['original_capture_sha256'].items():
        assert sha(BINARY/name)==digest, 'original path/entry capture changed: '+name
    definitions = application_symbols(object_directory)
    expected = {n:r['source'] for n,r in cache['application_definitions'].items()}
    assert definitions==expected, 'unresolved/new/missing native application definition'
    return cache


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--objects',type=Path,default=BINARY/'build/native-trace')
    ap.add_argument('--write',action='store_true')
    options=ap.parse_args()
    if options.write:
        cache=project(options.objects)
        CACHE.write_text(json.dumps(cache,indent=2)+'\n')
    else:
        cache=load(options.objects)
    print('application definitions:',len(cache['application_definitions']))
    print('cached original segments:',len(cache['cases']))


if __name__=='__main__':
    main()
