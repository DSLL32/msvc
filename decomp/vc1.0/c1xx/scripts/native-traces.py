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
REFERENCE = BINARY / 'evidence/fast-replay-verification-receipt.json'
MAP = BINARY / 'cases/native-cpp-basic/canonical-map.txt'


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
    records = {r['name']:r for r in runtime_records + imports(PE(BINARY.parents[2]/'toolchains/1.0-nt/bin/C1XX3232.EXE')) + config['driver']}
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
    reference = json.loads(REFERENCE.read_text())
    original = reference['sides'][0]
    transcript = [event['name'] for event in original['events'] if event['event'] == 'frontier']
    original_terminal = original['terminal']
    extension_files = []
    # Fresh, explicit original-only captures extend the immutable historical
    # transcript one accepted function at a time. Never infer a terminal from
    # the native run that will consume this cache.
    for filename in config['cases']['cpp_basic'].get('frontier_extensions', []):
        path = BINARY / filename
        extension = json.loads(path.read_text())
        capture_path = BINARY / extension['original_capture']
        events_path = BINARY / extension['original_events']
        assert sha(capture_path) == extension['original_capture_sha256'], 'original extension capture changed'
        assert sha(events_path) == extension['original_events_sha256'], 'original extension events changed'
        capture = json.loads(capture_path.read_text())
        assert extension['accepted_before'] == len(transcript) == capture['accepted_count']
        assert extension['accepted'] == original_terminal == capture['candidate']
        assert capture['original_sha256'] == config['target']['sha256']
        assert extension['terminal'] == capture['terminal']
        event = re.search(r'^exception 80000003 rva ([0-9a-f]+) base ([0-9a-f]+) first 1$',
                          events_path.read_text(), re.M)
        if extension['terminal'].startswith('exit:'):
            status=int(extension['terminal'].split(':')[1])
            assert not event and not re.search(r'^exception ',events_path.read_text(),re.M),'exit extension contains an exception'
            assert re.search(r'^exit '+str(status)+r'$',events_path.read_text(),re.M),'original terminal status differs'
        else:
            assert event, 'original extension did not stop at a first-chance INT3'
            assert int(event[1], 16) + int(event[2], 16) == records[extension['terminal']]['start']
        if extension['accepted'] not in application:
            evidence_path = BINARY / extension['runtime_evidence']
            spec = importlib.util.spec_from_file_location('c1xx_runtime_boundary', BINARY/'scripts/runtime-boundary.py')
            checker = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(checker)
            proof = checker.check(evidence_path)
            position = proof['runtime_order'].index(extension['accepted'])
            assert extension['accepted_before'] == proof['accepted_before'] + 1 + position
            assert sha(evidence_path) == extension['runtime_evidence_sha256']
            extension_files += [evidence_path] + [BINARY/item['path'] for item in proof['evidence_files']]
        if 'original_configuration' in extension:
            configuration_path = BINARY/extension['original_configuration']
            assert sha(configuration_path) == capture['configuration_sha256']
            captured_config = json.loads(configuration_path.read_text())
            assert captured_config['accepted'] + capture.get('pending_accepted', []) == transcript
            extension_files += [configuration_path]
        transcript.append(extension['accepted'])
        original_terminal = extension['terminal']
        extension_files += [path, capture_path, events_path]
    assert transcript == config['accepted'], 'original reference acceptance identity changed'
    terminal = config['cases']['cpp_basic']['terminal']
    assert original_terminal == terminal
    complete=terminal.startswith('exit:')
    assert complete or terminal in application, 'partial terminal must be an application stub'
    first = config['cases']['cpp_basic'].get('accepted_from', 0)
    project_names = lambda names: [native_name(n) for n in names if n in application]
    definitions = {native_name(n): dict(logical=n, source=source)
                   for n, source in application.items()}
    assert len(definitions) == len(application), 'native decoration collision'
    return dict(schema=1, scope='cached original application first entries; no capture',
        metadata_sha256={str(p.relative_to(BINARY)): sha(p) for p in
            [BINARY / 'functions.json', BINARY / 'runtime.json', REFERENCE, MAP] + extension_files},
        application_definitions=dict(sorted(definitions.items())),
        accepted_application=project_names(transcript),
        excluded_accepted={kind: [n for n in transcript if classified[n] == kind]
                           for kind in ('CRT', 'import')},
        cases={'cpp_basic': dict(first=first, last=len(transcript),
            seen=project_names(transcript[:first]),
            expected=project_names(transcript[first:]) + ([] if complete else [native_name(terminal)]),
            terminal=(dict(kind='exit',logical=terminal,status=int(terminal.split(':')[1])) if complete else
                      dict(kind='frontier',logical=terminal,native=native_name(terminal),signal=5)))})


def load(directory, path=CACHE):
    cache = json.loads(path.read_text())
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
