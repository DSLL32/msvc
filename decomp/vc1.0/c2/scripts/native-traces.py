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
from artifacts import compare

BINARY = Path(__file__).resolve().parents[1]
CACHE = BINARY / 'cases/native-traces.json'
CRT = {'_ftell', '_fseek', '__sopen', '__filbuf', '__freebuf'}


def runtime_records():
    from binary import PE
    from pe_indexes import imports
    original=PE(BINARY.parents[2]/'toolchains/1.0-nt/bin/C23232.EXE')
    return json.loads((BINARY/'runtime.json').read_text())+imports(original)


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
    """Project only durable fresh shared-prefix original first-entry captures."""
    from binary import PE
    from case_paths import PREFIX, C2_SCRATCH, canonical_parts
    config = json.loads((BINARY/'functions.json').read_text())
    drivers = {r['name']:r for r in config['driver']}
    application = set(drivers)-CRT
    native = {native_name(name):name for name in application}
    actual = definitions(directory)
    assert actual.keys() == native.keys(), 'unresolved application definitions'
    original = PE(BINARY.parents[2]/'toolchains/1.0-nt/bin/C23232.EXE')
    assert hashlib.sha256(original.data).hexdigest() == config['target']['sha256']
    accepted = set(config['accepted'])
    cases, provenance = {}, {}
    for profile, case in config['cases'].items():
        receipt_path = BINARY/'cases/shared-prefix-evidence'/(profile+'.json')
        receipt = json.loads(receipt_path.read_text())
        assert receipt['profile'] == profile and receipt['accepted'] == len(accepted)
        assert receipt['target_sha256'] == config['target']['sha256']
        assert receipt['config_sha256'] == sha(BINARY/'functions.json')
        assert receipt['wine_prefix'] == str(PREFIX)
        canonical_parts(receipt['canonical_working_directory'])
        manifest = BINARY/case['fixture_manifest']
        assert receipt['manifest_sha256'] == sha(manifest) == case['fixture_manifest_sha256']
        phase = json.loads(manifest.read_text())['phases']['c2']
        assert receipt['environment'] == phase['environment']
        assert receipt['inputs'] == {r['path']:r['sha256'] for r in phase['inputs']}
        assert receipt['canonical_working_directory'] == phase['canonical_working_directory']
        for item in phase['inputs']:
            assert sha(manifest.parent/item['path']) == item['sha256']
        projection = receipt['first_entry_projection']
        assert projection['expected'] == projection['observed']
        names = projection['observed']
        assert len(names) == len(set(names)) and all(n in application & accepted for n in names)
        seen = [native_name(n) for n in config['accepted'][:case.get('accepted_from',0)] if n in application]
        assert projection['seeded'] == seen
        events_path = receipt_path.with_suffix('.events')
        events = events_path.read_text()
        assert [line.split()[1] for line in events.splitlines() if line.startswith('entry ')] == names
        plan_path = receipt_path.with_suffix('.entry-plan.txt')
        assert sha(plan_path) == projection['entry_plan_sha256']
        planned = ''.join('%x %02x %s\n' % (drivers[name]['start']-original.base,
            original.read(drivers[name]['start'],1)[0],name) for name in names)
        assert plan_path.read_text() == planned
        terminal = receipt['logical']
        expected = [native_name(n) for n in names]
        if terminal == 'exit:0':
            assert receipt['events'] == 'exit 0\n'
            for item, artifact in zip(phase['outputs'],receipt['outputs']):
                data = bytearray((BINARY/artifact['path']).read_bytes())
                target = bytearray((manifest.parent/item['path']).read_bytes())
                assert hashlib.sha256(data).hexdigest() == artifact['sha256']
                checked=compare(data,target,item)
                assert artifact['ignored_timestamp']==checked['ignored_timestamp']
                assert checked['normalized_sha256']==artifact['normalized_sha256']
        else:
            assert terminal in application and terminal not in accepted
            assert drivers[terminal].get('provisional')
            assert receipt['original_va'] == hex(drivers[terminal]['start'])
            event = re.fullmatch(r'exception 80000003 rva ([0-9a-f]+) base ([0-9a-f]+) first 1\nexit 99\n',receipt['events'])
            assert event and int(event[1],16)+int(event[2],16) == drivers[terminal]['start']
            expected.append(native_name(terminal))
        assert events == ''.join('entry %s rva %08x byte %02x\n' %
            (name,drivers[name]['start']-original.base,original.read(drivers[name]['start'],1)[0]) for name in names)+receipt['events']
        cases[profile] = dict(first=case.get('accepted_from',0),last=len(config['accepted']),
            seen=seen,expected=expected,terminal=terminal,terminal_kind='exit' if terminal.startswith('exit:') else 'application',
            returncode=0 if terminal == 'exit:0' else -5)
        provenance[profile] = dict(path=str(receipt_path.relative_to(BINARY)),sha256=sha(receipt_path),
            events_sha256=sha(events_path),entry_plan_sha256=sha(plan_path),capture=receipt,
            source='Fresh original shared-prefix first-entry capture; accepted bodies/shared text restored, entry byte restore/EIP rewind')
    return dict(schema=2,description='Fresh shared-prefix original entry projection; no capture or Wine in fixtures',
        metadata_sha256={n:sha(BINARY/n) for n in ('functions.json','runtime.json')},
        application_definitions={n:dict(logical=native[n],source=actual[n]) for n in sorted(native)},
        excluded_crt=sorted(CRT),provenance=provenance,cases=cases)


def load(directory):
    cache = json.loads(CACHE.read_text())
    for path, digest in cache['metadata_sha256'].items():
        assert sha(BINARY/path) == digest, 'stale original trace projection: ' + path
    assert definitions(directory) == {n:r['source'] for n,r in cache['application_definitions'].items()}, 'unmapped application definitions'
    return cache


def check_runtime_evidence(path, name, before, record):
    """Validate an explicit library seam observation; never execute a reference."""
    from binary import PE, COFF, archive, u32
    evidence = json.loads(path.read_text())
    assert evidence['logical'] == name
    assert evidence['before_config_sha256'] == before['config_sha256']
    assert evidence['target_sha256'] == before['target_sha256']
    if name.startswith('import:'):
        from pe_indexes import imports
        original=PE(BINARY.parents[2]/'toolchains/1.0-nt/bin/C23232.EXE')
        assert hashlib.sha256(original.data).hexdigest()==before['target_sha256']
        assert name=='import:SetStdHandle' and record in imports(original)
        identity=evidence['original']['identity']
        assert identity['record']==record
        thunk=original.read(record['start'],record['end']-record['start'])
        assert len(thunk)==6 and thunk[:2]==b'\xff\x25'
        assert thunk.hex()==identity['thunk_hex'] and int.from_bytes(thunk[2:],'little')==identity['iat_address']
        left,right=evidence['observed_original'],evidence['observed_native']
        assert left==right==dict(std_handle=0xfffffff5,handle=0,result=True,before_nonnull=True,after_std=0)
        assert evidence['abi']=='BOOL WINAPI SetStdHandle(DWORD,HANDLE); stdcall8; original import is a tail thunk'
        assert evidence['caller_result_use']=='API return ignored by original free-os-handle path; StdOutput transition independently checked'
        return dict(path=str(path),sha256=sha(path),evidence=evidence)
    from crt_stdio import CONTRACTS,validate as validate_stdio
    if name in CONTRACTS:
        validate_stdio(evidence,record,before)
        return dict(path=str(path),sha256=sha(path),evidence=evidence)
    identity = evidence['original']['identity']
    assert identity['record'] == record
    library = BINARY.parents[2]/'toolchains/1.0-nt/lib'/record['library']
    assert sha(library) == identity['library_sha256']
    data = next(data for obj, data in archive(library) if obj == record['object'])
    assert hashlib.sha256(data).hexdigest() == identity['object_sha256']
    text = next(s for s in COFF(data).sections if s['flags'] & 0x20)
    original = PE(BINARY.parents[2]/'toolchains/1.0-nt/bin/C23232.EXE')
    assert hashlib.sha256(original.data).hexdigest() == before['target_sha256']
    linked = bytes(original.read(record['start'], record['end']-record['start']))
    assert len(linked) == len(text['data']) == identity['exact_text_bytes']
    assert hashlib.sha256(text['data']).hexdigest() == identity['text_sha256']
    if name == '_strncmp':
        assert not text['relocs'] and linked == text['data']
        left, right = evidence['observed_original'], evidence['observed_native']
        assert {k:left[k] for k in ('left_hex','right_hex','count')} == {k:right[k] for k in ('left_hex','right_hex','count')}
        assert evidence['caller_result_use'] == 'zero/nonzero'
        assert (left['result'] == 0) == (right['result'] == 0), 'CRT caller predicate differs'
    elif name == '_strpbrk':
        assert not text['relocs'] and linked == text['data']
        left, right = evidence['observed_original'], evidence['observed_native']
        assert left == right
        assert left['left_hex'] and left['right_hex'], 'Empty CRT argument inventory'
        assert left['result_nonnull'] and left['result_offset'] == 1
        assert evidence['caller_result_use'] == 'zero/nonzero; pointer offset independently preserved'
    elif name in ('_fread','_freopen','_printf'):
        assert hashlib.sha256(linked).hexdigest() == identity['linked_text_sha256']
        runtime = {r['name']:r for r in (runtime_records()+json.loads((BINARY/'functions.json').read_text())['driver'])}
        compared = bytearray(linked)
        relocations = []
        for offset, symbol, kind in text['relocs']:
            if kind == 20:
                assert symbol['name'] in runtime
                target = (record['start']+offset+4+u32(linked,offset)) & 0xffffffff
                assert target == runtime[symbol['name']]['start']
            else:
                assert name=='_printf' and kind==6 and symbol['name']=='__iob'
                target=u32(linked,offset)
                assert target==0x86198+u32(text['data'],offset)==0x861b8
            relocations.append(dict(offset=offset,symbol=symbol['name'],kind=kind,target=target))
            compared[offset:offset+4] = text['data'][offset:offset+4]
        assert bytes(compared) == text['data']
        assert relocations == identity['relocations']
        left, right = evidence['observed_original'], evidence['observed_native']
        assert left == right
        if name == '_fread':
            assert (left['size'],left['count'],left['result']) == (12,1,1)
            assert len(bytes.fromhex(left['bytes_hex'])) == 12
            assert evidence['caller_result_use'] == 'ignored; complete 12-byte destination is copied to Extended return value'
        elif name=='_printf':
            assert bytes.fromhex(left['format_hex'])==b'\tTITLE\t%s\n\t.386P\n\0'
            assert bytes.fromhex(left['argument_hex'])==b'dbglist.c\0' and left['result']==24
            assert evidence['caller_result_use']=='ignored at write_module_listing287a1; public return24 independently checked'
            assert any(r['symbol']=='__output' and r['target']==0x73890 for r in relocations)
            assert evidence['rejected_wide_alias']=='WPRINTF.OBJ has __woutput relocation, while linked target is proven narrow __output73890 and actual format is narrow ASCII'
        else:
            assert bytes.fromhex(left['name_hex']) == b'dbglist.cod\0'
            assert bytes.fromhex(left['mode_hex']) == b'wt\0'
            assert left['stream_is_stdout'] and left['return_is_stream']
            assert left['before'] == left['after'] == dict(flags=2,count=0,file=1,buffers_null=True,bufsize=0)
            assert evidence['caller_result_use'] == 'zero/nonzero; returned pointer remains the input stdout stream'

    else:
        raise AssertionError('new CRT seams require their own observation validator')
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
    profile = after.get('profile', 'hello')
    assert before.get('profile', 'hello') == profile, 'reference profiles differ'
    name = accepted[old_count]
    # An untyped naked stub can acquire its recovered calling convention at
    # acceptance. Require the same C identity and exact original entry below;
    # only the decoration changes, never the function being accepted.
    assert (before['logical']==name if name.startswith('import:') else native_name(before['logical'])==native_name(name)), 'wrong original entry before acceptance'
    drivers = {r['name']:r for r in config['driver']}
    runtime = {r['name']:r for r in (runtime_records()+json.loads((BINARY/'functions.json').read_text())['driver'])}
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
    case = cache['cases'][profile]
    assert case['last'] == old_count and case['terminal'] == before['logical']
    if name in application_names:
        assert case['expected'][-1] == native_name(name)
        assert runtime_evidence is None, 'runtime evidence supplied for application acceptance'
        library_evidence = None
    else:
        assert runtime_evidence is not None, 'CRT acceptance requires explicit library and call evidence'
        library_evidence = check_runtime_evidence(runtime_evidence, name, before, records[name])
    manifest = BINARY/config['cases'][profile]['fixture_manifest']
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
                checked=compare(data,reference,expected)
                assert artifact['ignored_timestamp']==checked['ignored_timestamp']
                assert checked['normalized_sha256']==artifact['normalized_sha256']
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
    case['pending_runtime'] = not completed and terminal not in application_names
    if terminal in application_names:
        case['expected'].append(native_name(terminal))
    cache['metadata_sha256'] = {n:sha(BINARY/n) for n in ('functions.json','runtime.json')}
    cache['provenance'][profile].setdefault('extensions', []).append(dict(
        accepted_function=name, accepted_kind='application' if name in application_names else 'runtime-library',
        previous_logical=before['logical'],
        original_extent=[hex(records[name]['start']),hex(records[name]['end'])],
        before=dict(path=str(before_path), sha256=sha(before_path), capture=before),
        after=dict(path=str(after_path), sha256=sha(after_path), capture=after)))
    if library_evidence is not None:
        cache['provenance'][profile]['extensions'][-1]['library_evidence'] = library_evidence
    return cache


def begin(directory, profile, reference_path):
    """Register an independently captured case without changing acceptance."""
    cache = json.loads(CACHE.read_text())
    config = json.loads((BINARY/'functions.json').read_text())
    assert profile not in cache['cases'] and profile in config['cases']
    old = json.loads(json.dumps(config))
    case_config = old['cases'].pop(profile)
    assert hashlib.sha256((json.dumps(old,indent=2)+'\n').encode()).hexdigest() == cache['metadata_sha256']['functions.json'], 'new case must be the only configuration change'
    assert sha(BINARY/'runtime.json') == cache['metadata_sha256']['runtime.json']
    receipt = json.loads(reference_path.read_text())
    assert receipt['profile'] == profile
    assert receipt['accepted'] == len(config['accepted']) == case_config['accepted_from']
    assert receipt['config_sha256'] == sha(BINARY/'functions.json')
    assert receipt['target_sha256'] == config['target']['sha256']
    manifest = BINARY/case_config['fixture_manifest']
    assert receipt['manifest_sha256'] == sha(manifest) == case_config['fixture_manifest_sha256']
    phase = json.loads(manifest.read_text())['phases']['c2']
    assert receipt['inputs'] == {r['path']:r['sha256'] for r in phase['inputs']}
    assert receipt['environment'] == phase['environment']
    terminal = receipt['logical']
    drivers = {r['name']:r for r in config['driver']}
    runtime = {r['name']:r for r in runtime_records()}
    records = dict(runtime, **drivers)
    assert terminal in records and terminal not in config['accepted']
    application_terminal = terminal in drivers
    assert not application_terminal or drivers[terminal].get('provisional')
    assert receipt['original_va'] == hex(records[terminal]['start'])
    event = re.fullmatch(r'exception 80000003 rva ([0-9a-f]+) base ([0-9a-f]+) first 1\nexit 99\n',receipt['events'])
    assert event and int(event[1],16)+int(event[2],16) == records[terminal]['start']
    actual = definitions(directory)
    assert actual == {n:r['source'] for n,r in cache['application_definitions'].items()}
    application = {r['logical'] for r in cache['application_definitions'].values()}
    seen = [native_name(n) for n in config['accepted'] if n in application]
    cache['cases'][profile] = dict(first=len(config['accepted']),last=len(config['accepted']),
        seen=seen,expected=[native_name(terminal)] if application_terminal else [],terminal=terminal,
        terminal_kind='application' if application_terminal else 'runtime-library',returncode=-5,
        pending_runtime=not application_terminal)
    cache['provenance'][profile] = dict(initial=dict(path=str(reference_path),sha256=sha(reference_path),capture=receipt))
    cache['metadata_sha256']['functions.json'] = sha(BINARY/'functions.json')
    return cache


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write', action='store_true')
    parser.add_argument('--objects', type=Path, default=BINARY/'build/native-trace')
    parser.add_argument('--extend-before', type=Path, help='explicit original-only receipt before one-function acceptance')
    parser.add_argument('--extend-after', type=Path, help='explicit original-only receipt after one-function acceptance')
    parser.add_argument('--runtime-evidence', type=Path, help='explicit indexed CRT body/call observations for a library acceptance')
    parser.add_argument('--begin-profile', help='new case added alone, with all existing acceptances seeded')
    parser.add_argument('--reference', type=Path, help='fresh original-only initial named-stub receipt')
    args = parser.parse_args()
    if args.begin_profile:
        assert args.reference and not args.write and not args.extend_before and not args.extend_after
        result = begin(args.objects, args.begin_profile, args.reference)
        CACHE.write_text(json.dumps(result,indent=2)+'\n')
    elif args.extend_before or args.extend_after:
        assert args.extend_before and args.extend_after and not args.write
        result = extend(args.objects, args.extend_before, args.extend_after, args.runtime_evidence)
        CACHE.write_text(json.dumps(result,indent=2)+'\n')
    elif args.write:
        result = project(args.objects)
        CACHE.write_text(json.dumps(result, indent=2)+'\n')
    else:
        result = load(args.objects)
    print(len(result['application_definitions']), 'application definitions;', len(result['cases']), 'cached cases')
