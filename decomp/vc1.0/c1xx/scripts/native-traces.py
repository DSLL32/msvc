"""Project frozen original C1XX entries onto every native application definition."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess

BINARY = Path(__file__).resolve().parents[1]
CACHE = BINARY / 'cases/native-traces.json'
REFERENCE = BINARY / 'cases/original-entry-references.json'
MAP = BINARY / 'cases/canonical-map.txt'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def native_name(name):
    assert name[0] in '_@', name
    result = re.sub(r'@\d+$', '', name[1:])
    assert re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', result), name
    return result


def application_symbols(directory):
    objects = [p for p in sorted(directory.glob('*.o')) if p.name != 'runtime.o']
    assert objects, 'native application objects missing'
    output = subprocess.check_output(['nm', '-A', '--defined-only', '--format=posix',
                                      *map(str, objects)], text=True)
    result = {}
    for line in output.splitlines():
        parts = line.split()
        if len(parts) < 4 or parts[2] not in ('T', 't', 'W', 'w'):
            continue
        name = parts[1]
        assert name not in result, 'duplicate native application symbol: ' + name
        result[name] = 'src/' + Path(parts[0][:-1]).stem + '.c'
    return result


def runtime_evidence(config):
    """Retain newly observed runtime seams before projecting them away."""
    from runtime_boundary_loader import load_boundary
    boundary=load_boundary();files=[];verified={};covered=set()
    drivers={r['name'] for r in config['driver']}
    for case in config['cases'].values():
        for relative in case.get('frontier_extensions',[]):
            extension_path=BINARY/relative
            extension=json.loads(extension_path.read_text())
            if 'runtime_evidence' not in extension:continue
            path=BINARY/extension['runtime_evidence']
            assert sha(path)==extension['runtime_evidence_sha256'], 'runtime acceptance evidence changed'
            if path not in verified:
                verified[path]=boundary.check(path)
                files.extend([path,*[BINARY/item['path'] for item in verified[path]['evidence_files']]])
            proof=verified[path]
            if extension['accepted'] not in drivers:
                assert extension['accepted'] in proof['runtime_order'], 'runtime seam lacks observed boundary'
                covered.add(extension['accepted'])
            files.append(extension_path)
    if '_strchr' in config['accepted']:
        assert '_strchr' in covered, 'strchr cannot be filtered without its original/native boundary proof'
    return list(dict.fromkeys(files))


def project():
    config = json.loads((BINARY / 'functions.json').read_text())
    sources = {}
    for line in MAP.read_text().splitlines():
        match = re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+[0-9a-f]{8}\s+f\s+(\S+)', line)
        if match and ':' not in match[2]:
            sources[match[1]] = 'src/' + Path(match[2]).stem + '.c'
    drivers = {r['name']: r for r in config['driver']}
    assert drivers.keys() <= sources.keys(), 'unclassified driver source'
    application = {name: source for name, source in sources.items()
                   if name in drivers and source != 'src/runtime.c'}
    runtime_records = json.loads((BINARY / 'runtime.json').read_text())
    runtime = {r['name'] for r in runtime_records}
    from binary import PE
    from win32_indexes import imports
    image=PE(BINARY.parents[2]/'toolchains/1.0-nt/bin/C1XX3232.EXE')
    assert hashlib.sha256(image.data).hexdigest()==config['target']['sha256']
    records = {r['name']:r for r in runtime_records + imports(image) + config['driver']}
    classified = {}
    for name in config['accepted']:
        if name in application:
            classified[name] = 'application'
        elif name.startswith('import:'):
            classified[name] = 'import'
        elif name in runtime or sources.get(name) == 'src/runtime.c':
            classified[name] = 'CRT'
        else:
            raise AssertionError('unclassified original entry: ' + name)
    project_names=lambda names:[native_name(n) for n in names if n in application]
    definitions={native_name(n):dict(logical=n,source=source) for n,source in application.items()}
    assert len(definitions)==len(application),'native decoration collision'
    reference=json.loads(REFERENCE.read_text())
    extension_files=runtime_evidence(config)
    cases={}
    transcript=config['accepted']
    for case_name,case in config['cases'].items():
        first=case.get('accepted_from',0)
        bundle=reference[case_name]
        receipt_path=BINARY/bundle['receipt']
        events_path=BINARY/bundle['events']
        plan_path=BINARY/bundle['plan']
        receipt=json.loads(receipt_path.read_text())
        for field,path in [('receipt',receipt_path),('events',events_path),('plan',plan_path)]:
            assert sha(path)==bundle[field+'_sha256'],'original entry reference changed'
        projection=receipt['first_entry_projection']
        last=projection['accepted_last']
        assert projection['accepted_first']==first
        assert projection['accepted_prefix_sha256']==hashlib.sha256(json.dumps(transcript[:last],separators=(',',':')).encode()).hexdigest()
        expected_logical=[n for n in transcript[first:last] if n in application]
        plan_rows=[line.split() for line in plan_path.read_text().splitlines()]
        assert [row[2] for row in plan_rows]==expected_logical,'original entry plan identity differs'
        for rva,byte,name in plan_rows:
            assert int(rva,16)+image.base==records[name]['start'],'original entry plan address differs'
            assert int(byte,16)==image.read(records[name]['start'],1)[0],'original entry restore byte differs'
        observed_rows=re.findall(r'^entry (\S+) rva ([0-9a-f]+) byte ([0-9a-f]+)$',events_path.read_text(),re.M)
        assert [(name,int(rva,16),int(byte,16)) for name,rva,byte in observed_rows]==[(name,int(rva,16),int(byte,16)) for rva,byte,name in plan_rows], 'original debugger entry evidence differs'
        observed=re.findall(r'^entry (\S+) rva [0-9a-f]+ byte [0-9a-f]+$',events_path.read_text(),re.M)
        assert observed==expected_logical==projection['observed']==projection['expected'],'original first entries differ'
        assert receipt['original_sha256']==config['target']['sha256']
        terminal=case['terminal']
        assert receipt['terminal']==terminal
        raw=events_path.read_text()
        if terminal.startswith('exit:'):
            assert not re.search(r'^exception ',raw,re.M)
            assert re.search(r'^exit '+terminal.split(':')[1]+r'$',raw,re.M)
        else:
            event=re.search(r'^exception 80000003 rva ([0-9a-f]+) base ([0-9a-f]+) first 1$',raw,re.M)
            assert event and int(event[1],16)+int(event[2],16)==records[terminal]['start']
        complete=terminal.startswith('exit:')
        assert complete or terminal in application,'partial terminal must be an application stub'
        cases[case_name]=dict(first=first,last=last,seen=project_names(transcript[:first]),
            expected=project_names(transcript[first:last])+([] if complete else [native_name(terminal)]),
            terminal=(dict(kind='exit',logical=terminal,status=int(terminal.split(':')[1])) if complete else
                      dict(kind='frontier',logical=terminal,native=native_name(terminal),signal=5)))
        extension_files.extend([receipt_path,events_path,plan_path])
    assert max(c['last'] for c in cases.values())==len(transcript)
    return dict(schema=1,scope='cached original application first entries; no capture',
        metadata_sha256={str(p.relative_to(BINARY)):sha(p) for p in
            [BINARY/'functions.json',BINARY/'runtime.json',REFERENCE,MAP]+extension_files},
        application_definitions=dict(sorted(definitions.items())),accepted_application=project_names(transcript),
        excluded_accepted={kind:[n for n in transcript if classified[n]==kind] for kind in ('CRT','import')},cases=cases)


def load(directory, path=CACHE):
    cache = json.loads(path.read_text())
    runtime_evidence(json.loads((BINARY/'functions.json').read_text()))
    for name, digest in cache['metadata_sha256'].items():
        assert sha(BINARY / name) == digest, 'native cache stale: ' + name
    definitions = application_symbols(directory)
    assert definitions == {n: r['source'] for n, r in cache['application_definitions'].items()}, (
        'unresolved native application definition',
        sorted(definitions.keys() - cache['application_definitions'].keys()),
        sorted(cache['application_definitions'].keys() - definitions.keys()))
    return cache


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write', action='store_true')
    parser.add_argument('--objects', type=Path, default=BINARY / 'build/native-trace')
    args = parser.parse_args()
    if args.write:
        CACHE.write_text(json.dumps(project(), indent=2) + '\n')
    else:
        cache = load(args.objects)
        print('Native application definitions:', len(cache['application_definitions']))
        print('Expected cpp_basic application entries:', len(cache['cases']['cpp_basic']['expected']))
