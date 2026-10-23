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
ALIASES = {'help':'/?', 'dump-library':'library-dump-library',
           'dump-object':'library-dump-object', 'dump-dll':'library-dump-dll',
           'edit':'library-edit', 'dump-edited':'library-dump-edited'}


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


def original_references(cases):
    references={}
    config=json.loads((BINARY/'functions.json').read_text())
    app={r['name'] for r in config['driver'] if r.get('source') not in (None,'src/runtime.c')}
    projected=[native_name(n) for n in config['accepted'] if n in app]
    registry=json.loads((BINARY/'cases/reference-cases.json').read_text())
    for fixture,record in registry.items():
        directory=BINARY/record['directory']
        receipt_path=directory/'capture/receipt.json'
        receipt=json.loads(receipt_path.read_text())
        events_path=directory/'capture/events.jsonl'
        assert sha(events_path)==receipt['events']['sha256']
        assert receipt['status']=='pass' and receipt['expected']==receipt['observed']
        segment=cases[ALIASES.get(fixture,fixture)]
        assert receipt['expected']==segment['expected'], fixture+': fresh original entries differ'
        assert receipt['prefix_application']==projected[:segment['seen_count']], fixture+': original prefix differs'
        assert receipt['original_sha256']==config['target']['sha256']
        ceiling=receipt['accepted_count']
        assert segment['last']<=ceiling<=len(config['accepted'])
        assert receipt['accepted_sha256']==hashlib.sha256(json.dumps(config['accepted'][:ceiling]).encode()).hexdigest()
        events=[json.loads(line) for line in events_path.read_text().splitlines()]
        assert [event['name'] for event in events if event['event']=='entry']==receipt['observed']
        assert events[-1]==receipt['terminal']
        assert receipt['terminal']['root_exit']==segment['terminal']['code']
        assert receipt['terminal']['failed']==0
        references[fixture]={
            'receipt_path':str(receipt_path.relative_to(BINARY)),
            'receipt_sha256':sha(receipt_path),
            'manifest_path':str((directory/'manifest.json').relative_to(BINARY)),
            'manifest_sha256':sha(directory/'manifest.json'),
            'original_sha256':receipt['original_sha256']}
    return references


def project(object_directory):
    config_path, runtime_path = BINARY/'functions.json', BINARY/'runtime.json'
    config = json.loads(config_path.read_text())
    runtime = {r['name'] for r in json.loads(runtime_path.read_text())}
    drivers = {r['name']:r for r in config['driver']}
    assert len(drivers)==len(config['driver'])
    app = {n:r for n,r in drivers.items() if r.get('source') and r['source']!='src/runtime.c'}
    native = {native_name(n):r for n,r in app.items()}
    assert len(native)==len(app), 'application decoration collision'
    definitions = application_symbols(object_directory)
    assert definitions.keys()==native.keys(), ('unresolved application definitions',
        sorted(definitions.keys()-native.keys()), sorted(native.keys()-definitions.keys()))
    for n,source in definitions.items():
        assert native[n]['source']==source, (n,source,native[n]['source'])
    accepted, history = config['accepted'], config['frontier_cases']
    assert len(accepted)==len(history) and len(accepted)==len(set(accepted))
    classified = {}
    for name in accepted:
        if name in app:
            classified[name] = 'application'
        elif name.startswith('import:'):
            assert re.fullmatch(r'import:[A-Za-z_][A-Za-z_0-9]*',name)
            classified[name] = 'import'
        elif name in runtime or (name in drivers and
                (drivers[name].get('source')=='src/runtime.c' or drivers[name].get('library'))):
            classified[name] = 'CRT'
        else:
            raise AssertionError('unresolved accepted identity: '+name)
    registry = json.loads((BINARY/'replay-cases.json').read_text())['cases']
    cases = {}
    for record in registry:
        case = record['case']
        indexes = [i for i,c in enumerate(history) if c==case]
        first,last = (indexes[0],indexes[-1]+1) if indexes else (record['accepted_prefix'],record['accepted_prefix'])
        assert not indexes or indexes==list(range(first,last)), 'noncontiguous case segment'
        projected = lambda names:[native_name(n) for n in names if classified[n]=='application']
        cases[case] = dict(first=first,last=last,seen_count=len(projected(accepted[:first])),
                          expected=projected(accepted[first:last]),terminal=record['terminal'])
    return dict(schema=2,description='Application projection verified against fresh shared-prefix original captures',
                original_entry_references=original_references(cases),
                metadata_sha256={'functions.json':sha(config_path),'runtime.json':sha(runtime_path),
                                 'replay-cases.json':sha(BINARY/'replay-cases.json')},
                application_definitions={n:dict(source=native[n]['source'],
                    logical=next(k for k in app if native_name(k)==n),
                    implementation=native[n]['implementation']) for n in sorted(native)},
                accepted_application=[native_name(n) for n in accepted if classified[n]=='application'],
                excluded_accepted={kind:[n for n in accepted if classified[n]==kind]
                                   for kind in ('CRT','import')},cases=cases)


def load(object_directory, cache_path=CACHE):
    cache = json.loads(cache_path.read_text())
    for name,digest in cache['metadata_sha256'].items():
        assert sha(BINARY/name)==digest, 'native trace cache stale: '+name
    assert cache['original_entry_references']==original_references(cache['cases']), 'original reference binding differs'
    config=json.loads((BINARY/'functions.json').read_text())
    app={r['name'] for r in config['driver'] if r.get('source') not in (None,'src/runtime.c')}
    assert cache['accepted_application']==[native_name(n) for n in config['accepted'] if n in app], 'application prefix differs'
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
