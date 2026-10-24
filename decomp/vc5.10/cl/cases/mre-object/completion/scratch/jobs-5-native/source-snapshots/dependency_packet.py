#!/usr/bin/env python3
"""Freeze/decode an original-only dependency observation; never accept target code."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

from binary import PE, u32
from mktemp_state import original_states, CRT_SHA

TARGET_SHA = 'aa15ed605515693292c539827ab1d8e08ce2ed5b4e1d9ac2c7f033903e6844dd'
C1_SHA = '7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d'
PHASES = ('load-before', 'c1-entry-1', 'load-after', 'get-invoke-before',
          'get-invoke-after', 'get-abort-before', 'get-abort-after',
          'invoke-before', 'invoke-after', 'free-before', 'c1-entry-0',
          'free-after', 'spawn-before')


def digest(data):
    return hashlib.sha256(data).hexdigest()


def file_record(path):
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': digest(data)}


def enumerate_files(directory):
    files = {}
    for path in sorted(directory.rglob('*')):
        assert not path.is_symlink(), 'symlink in original evidence'
        if path.is_file():
            files[path.relative_to(directory).as_posix()] = file_record(path)
    return files


def freeze(directory, destination):
    directory = directory.resolve()
    assert not destination.resolve().is_relative_to(directory), 'keep derived products outside raw capture'
    value = {'kind': 'original-only dependency analysis raw manifest',
             'directory': str(directory), 'files': enumerate_files(directory)}
    payload = json.dumps(value, indent=2) + '\n'
    if destination.exists():
        assert destination.read_text() == payload, 'immutable raw manifest differs'
    else:
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(payload)
    return value


def vector(data):
    assert len(data) >= 8
    address, count = struct.unpack_from('<II', data)
    assert address and count <= 4096
    offset, entries = 8, []
    for unused in range(count):
        pointer, length = struct.unpack_from('<II', data, offset)
        offset += 8
        text = data[offset:offset + length]
        assert pointer and 1 <= length <= 32768 and len(text) == length
        assert text[-1:] == b'\0' and b'\0' not in text[:-1]
        entries.append((pointer, text[:-1]))
        offset += length
    assert offset == len(data), 'unparsed vector bytes'
    return address, entries


def environment(data):
    unused, entries = vector(data)
    result = {}
    for pointer, text in entries:
        split = text.find(b'=', 1)
        assert split >= 1
        name = text[:split].decode('ascii')
        assert name not in result
        result[name] = text[split + 1:]
    return result


def os_environment(data):
    assert len(data) % 2 == 0 and data.endswith(b'\0\0\0\0')
    result = {}
    for text in data.decode('utf-16le')[:-2].split('\0'):
        split = text.find('=', 1)
        assert split >= 1 and text[:split] not in result
        result[text[:split]] = text[split + 1:]
    return result


def directory_entries(data):
    assert len(data) % 320 == 0
    result = {}
    for offset in range(0, len(data), 320):
        record = data[offset:offset + 320]
        name = record[44:304].split(b'\0', 1)[0].decode('ascii')
        assert name and name not in result and not u32(record, 0) & 0x10
        result[name] = {'attributes': u32(record, 0),
                        'creation_filetime': struct.unpack_from('<Q', record, 4)[0],
                        'access_filetime': struct.unpack_from('<Q', record, 12)[0],
                        'write_filetime': struct.unpack_from('<Q', record, 20)[0],
                        'bytes': (u32(record, 28) << 32) | u32(record, 32),
                        'alternate_name': record[304:318].split(b'\0', 1)[0].decode('ascii')}
    return result


def exports(pe):
    rva = u32(pe.data, pe.optional + 96)
    header = pe.read(pe.base + rva, 40)
    ordinal_base, functions, names, addresses, name_ptrs, ordinals = struct.unpack_from('<IIIIII', header, 16)
    result = {}
    for index in range(names):
        name_rva = u32(pe.read(pe.base + name_ptrs + index * 4, 4), 0)
        ordinal = struct.unpack('<H', pe.read(pe.base + ordinals + index * 2, 2))[0]
        assert ordinal < functions
        result[pe.string(pe.base + name_rva)] = {
            'ordinal': ordinal_base + ordinal,
            'rva': u32(pe.read(pe.base + addresses + ordinal * 4, 4), 0)}
    return result


def decode(manifest_path):
    manifest_path = Path(manifest_path).resolve()
    manifest = json.loads(manifest_path.read_text())
    assert manifest['kind'] == 'original-only dependency analysis raw manifest'
    directory = Path(manifest['directory'])
    assert enumerate_files(directory) == manifest['files'], 'raw observation files changed'
    def read(name):
        assert name in manifest['files'], 'unbound raw input: ' + name
        return (directory / name).read_bytes()
    capture = json.loads(read('capture.json'))
    complete = capture.get('dependency_complete', False)
    phases = PHASES
    if complete:
        phases = ('putenv-1-before', 'putenv-1-after') + PHASES[:-1] + (
            'putenv-2-before', 'putenv-2-after', 'spawn-before', 'spawn-after')
    assert capture['baseline'] is True and capture['dependency_analysis'] is True
    assert capture['controller_error'] is None and capture['helper_status'] == 0
    assert capture['case'] == 'minimal-c' and capture['arguments'] == ['/nologo', '/c', 'test.c']
    assert capture['target_sha256'] == capture['original_mask_sha256'] == TARGET_SHA
    assert digest(read('CL.EXE')) == digest(read('loaded-target.bin')) == TARGET_SHA
    assert digest(read('loaded-msvcrt.bin')) == capture['loaded_msvcrt_sha256'] == CRT_SHA
    assert digest(read('observer/loaded-c1.bin')) == digest(read('C1.DLL')) == C1_SHA
    assert digest(read('reference-debugger.exe')) == capture['helper_sha256'] == capture['helper_build']['binary']['sha256']
    for name, expected in capture['helper_sources'].items():
        assert digest(read(name)) == expected == capture['helper_build']['sources']['scripts/' + name]
    for name, expected in capture['files'].items():
        assert manifest['files'][name] == expected, 'capture/root manifest disagreement'
    assert digest(read('config.json')) == capture['config_sha256']
    for item in capture['inputs']:
        assert file_record(directory / item['name']) == {k: item[k] for k in ('bytes', 'sha256')}
    events = read('observer/events.txt').decode('ascii')
    assert events.count('invoke-call ') == 1
    if complete:
        assert 'analysis-stop-before-c2' not in events
        assert events.count('analysis-c2-completed result=00000000 putenv_calls=2\n') == 1
        assert re.findall(r'^exit .*status=(\d+)$', read('events.txt').decode('ascii'), re.M) == ['0']
    else:
        assert events.count('analysis-stop-before-c2 invocations=1 unloads=1\n') == 1
    assert not re.search(r'^create .*parent=', read('events.txt').decode('ascii'), re.M), 'unexpected child process'
    states = original_states(directory, phases, 'observer/events.txt')
    snapshots = {}
    contexts = {}
    for phase in phases:
        match = re.findall(r'^snapshot phase=' + phase + r' event=(\d+) tid=(\d+) eax=([0-9a-f]{8}) esp=([0-9a-f]{8}) ebp=([0-9a-f]{8})$', events, re.M)
        assert len(match) == 1
        event, tid, eax, esp, ebp = match[0]
        context = read('observer/' + phase + '-context.bin')
        assert len(context) == 204 and u32(context, 0) & 8
        assert (u32(context, 176), u32(context, 196), u32(context, 180)) == tuple(int(n, 16) for n in (eax, esp, ebp))
        contexts[phase] = context
        words = struct.unpack('<32I', read('observer/' + phase + '-crt-words.bin'))
        assert words[0] == 0x78000000 and words[23:] == (0,) * 9
        mb = read('observer/' + phase + '-mb-state.bin')
        ctype = read('observer/' + phase + '-ctype.bin')
        assert len(mb) == 261 and len(ctype) == 514 and u32(mb, 0) == words[6]
        parameters = read('observer/' + phase + '-parameters.bin')
        assert len(parameters) == 96 and u32(parameters, 0x48) == words[19]
        texts = {}
        for field, offset in (('cwd', 0x24), ('image', 0x38), ('command', 0x40)):
            raw = read('observer/' + phase + '-' + field + '-utf16.bin')
            length, capacity, pointer = struct.unpack_from('<HHI', parameters, offset)
            assert pointer and capacity >= length and len(raw) == length + 2 and raw[-2:] == b'\0\0'
            texts[field] = raw[:-2].decode('utf-16le')
        env_address, env = vector(read('observer/' + phase + '-crt-environment.bin'))
        assert env_address == words[1]
        argv_address, argv = vector(read('observer/' + phase + '-caller-argv.bin'))
        assert argv_address == words[20]
        iob = read('observer/' + phase + '-static-iob.bin')
        assert len(iob) == 640
        streams = json.loads(read('streams-' + phase + '.json'))
        lowio = read('observer/' + phase + '-lowio-0.bin')
        assert len(lowio) == 1152
        teb = read('streams-' + phase + '-teb.bin')
        peb = read('streams-' + phase + '-peb.bin')
        assert teb == read('mktemp-' + phase + '-teb.bin')
        assert u32(teb, 0x18) == streams['teb'] and u32(teb, 0x30) == streams['peb'] == words[17]
        assert u32(peb, 0x10) == streams['parameters'] == words[18]
        stream_parameters = read('streams-' + phase + '-parameters.bin')
        assert stream_parameters == parameters[:36]
        assert read('streams-' + phase + '-lowio.bin') == lowio[:108]
        assert read('streams-' + phase + '-iob.bin') == iob[:96]
        assert u32(read('streams-' + phase + '-pioinfo-slot.bin'), 0) == streams['lowio']
        assert u32(read('observer/' + phase + '-pioinfo-pointers.bin'), 0) == streams['lowio']
        descriptors = json.loads(read('observer/' + phase + '-open-descriptors.json'))
        assert [item['fd'] for item in descriptors] == [0, 1, 2]
        assert all(lowio[36 * index + 4] & 1 for index in range(3))
        assert not any(lowio[36 * index + 4] & 1 for index in range(3, 32))
        for index in range(3):
            assert u32(lowio, index * 36) == streams['crt'][index]['handle']
            assert u32(parameters, 0x18 + index * 4) == streams['win32'][index]['handle']
            assert descriptors[index]['query'] == streams['crt'][index]
        entries = directory_entries(read('observer/' + phase + '-directory.bin'))
        for item in ('test.c', 'CL.EXE', 'C1.DLL', 'C2.EXE', 'MSVCRT.DLL', 'MSPDB50.DLL'):
            assert entries[item]['bytes'] == manifest['files'][item]['bytes']
        snapshots[phase] = {
            'event': int(event), 'thread': int(tid), 'errors': states[phase],
            'x87_control_word': u32(context, 28) & 0xffff,
            'fmode': words[3], 'commode': words[4], 'mb_cur_max': words[5],
            'mb_codepage': words[6], 'lc_codepage': words[7], 'lc_handles': list(words[8:14]),
            'pctype_address': words[14], 'mb_state_sha256': digest(mb), 'ctype_sha256': digest(ctype),
            'crt_environment_count': len(env), 'crt_environment_sha256': digest(read('observer/' + phase + '-crt-environment.bin')),
            'os_environment_sha256': digest(read('observer/' + phase + '-os-environment.bin')),
            'argv': [text.decode('ascii') for pointer, text in argv],
            'argument_pointers': [pointer for pointer, text in argv],
            'argument_vector': argv_address, 'pass_address': words[21],
            'pass_words': list(struct.unpack('<9I', read('observer/' + phase + '-caller-pass.bin'))),
            'file0_2_words': list(struct.unpack('<24I', iob[:96])),
            'streams': streams, 'open_descriptors': descriptors,
            'directory': entries, **texts}
    assert [snapshots[p]['event'] for p in phases] == sorted(snapshots[p]['event'] for p in phases)
    call = re.findall(r'^invoke-call argc=(\d+) argv=([0-9a-f]{8}) last=(\d+) function=([0-9a-f]{8}) esp=([0-9a-f]{8})$', events, re.M)
    assert len(call) == 1
    argc, argv, last, function, esp = call[0]
    assert (int(argc), int(last)) == (26, 1)
    assert int(argv, 16) == snapshots['invoke-before']['argument_vector']
    assert struct.unpack_from('<3I', read('observer/invoke-before-stack.bin')) == (26, int(argv, 16), 1)
    assert u32(contexts['invoke-after'], 196) - int(esp, 16) == 12 and u32(contexts['invoke-after'], 176) == 0
    c1 = PE(directory / 'observer/loaded-c1.bin')
    c1_exports = exports(c1)
    assert c1_exports['_InvokeCompilerPass@12'] == {'ordinal': 2, 'rva': 0x12229}
    assert c1_exports['_AbortCompilerPass@4'] == {'ordinal': 1, 'rva': 0x43975}
    assert int(function, 16) == c1.base + c1_exports['_InvokeCompilerPass@12']['rva']
    assert c1.entry == 0x1202a and c1.read(c1.base + c1.entry, 1) == b'\x53'
    load_event = re.findall(r'^c1-load event=(\d+) base=10600000 entry=1061202a entry_byte=53$', events, re.M)
    assert len(load_event) == 1
    assert snapshots['load-before']['event'] < int(load_event[0]) < snapshots['c1-entry-1']['event'] < snapshots['load-after']['event']
    for reason in (1, 0):
        assert 'c1-entry event=%d module=10600000 reason=%d reserved=00000000' % (snapshots['c1-entry-%d' % reason]['event'], reason) in events
        assert struct.unpack_from('<3I', read('observer/c1-entry-%d-stack.bin' % reason), 4) == (c1.base, reason, 0)
    unload_events = re.findall(r'^module-unload event=(\d+) base=10600000$', events, re.M)
    assert len(unload_events) == 1 and snapshots['c1-entry-0']['event'] < int(unload_events[0]) < snapshots['free-after']['event']
    assert u32(contexts['free-after'], 176) == 1
    loader_calls = {}
    for api, result, cleanup in (('load', c1.base, 4), ('get-invoke', c1.base + 0x12229, 8),
                                 ('get-abort', c1.base + 0x43975, 8), ('free', 1, 4)):
        start, end = api + '-before', api + '-after'
        assert u32(contexts[end], 176) == result, 'wrong loader/export result: ' + api
        assert u32(contexts[end], 196) - u32(contexts[start], 196) == cleanup, 'wrong loader stdcall cleanup: ' + api
        parameters = struct.unpack_from('<2I', read('observer/' + start + '-stack.bin'))
        if api == 'load':
            assert parameters[0] == snapshots[start]['pass_words'][0]
        else:
            assert parameters[0] == c1.base
        if api.startswith('get-'):
            assert parameters[1] == (0x408524 if api == 'get-invoke' else 0x40850c)
        loader_calls[api] = {'return_value': result, 'stdcall_cleanup': cleanup,
                             'argument_words': list(parameters[:1 if api in ('load', 'free') else 2])}
    assert read('observer/get-invoke-before-argument.bin') == b'_InvokeCompilerPass@12\0'
    assert read('observer/get-abort-before-argument.bin') == b'_AbortCompilerPass@4\0'
    before, after = snapshots['invoke-before'], snapshots['invoke-after']
    assert before['errors'] == {'errno': 0, 'doserrno': 2, 'last_error': 2}
    assert after['errors'] == {'errno': 0, 'doserrno': 2, 'last_error': 0}
    assert (before['mb_codepage'], before['lc_codepage'], before['lc_handles']) == (0, 0, [0]*6)
    assert (after['mb_codepage'], after['lc_codepage'], after['lc_handles']) == (1252, 1252, [0, 0, 1033, 0, 0, 0])
    stable = ('x87_control_word', 'fmode', 'commode', 'mb_cur_max', 'crt_environment_sha256',
              'os_environment_sha256', 'argv', 'argument_pointers', 'argument_vector', 'cwd',
              'file0_2_words', 'streams', 'open_descriptors')
    assert all(before[key] == after[key] for key in stable), 'additional observable Invoke state change'
    for phase in ('free-before', 'c1-entry-0', 'free-after'):
        for key in ('mb_codepage', 'lc_codepage', 'lc_handles', 'mb_state_sha256', 'ctype_sha256', 'errors'):
            assert snapshots[phase][key] == after[key], 'detach/unload state differs: ' + key
    before_env = environment(read('observer/invoke-before-crt-environment.bin'))
    spawn_env = environment(read('observer/spawn-before-crt-environment.bin'))
    assert spawn_env.keys() - before_env.keys() == {'MSC_CMD_FLAGS'} and not before_env.keys() - spawn_env.keys()
    assert all(spawn_env[key] == value for key, value in before_env.items())
    flags = spawn_env['MSC_CMD_FLAGS'].decode('ascii')
    prefix = before['argv'][before['argv'].index('-il') + 1]
    assert re.fullmatch(r'\.\\a0[0-9]{4}', prefix)
    assert flags == '-il %s -f test.c -W 1 -G5 -Gs4096 -dos -Fotest.obj -ML -Fdvc50.idb ' % prefix
    assert len(snapshots['spawn-before']['argv']) == 1
    spawn_mode, spawn_path, spawn_argv = struct.unpack_from('<3I', read('observer/spawn-before-stack.bin'))
    assert spawn_mode == 0, 'wrong synchronous spawn mode'
    assert spawn_path == snapshots['spawn-before']['pass_words'][0], 'spawn path is not current pass name'
    assert spawn_argv == snapshots['spawn-before']['argument_vector'], 'spawn argv pointer differs from captured vector'
    expected_child_path = snapshots['spawn-before']['cwd'].rstrip('\\') + '\\c2.exe'
    assert snapshots['spawn-before']['argv'] == [expected_child_path], 'unexpected absolute child path'
    frozen_helper = read('dependency-observer.inc')
    if complete:
        assert b'remote_string(parameters[1],executable,sizeof(executable));' in frozen_helper
        assert b'remote_string(path_pointer,argument,sizeof(argument));' in frozen_helper
        assert b'if (strcmp(executable,argument)) ExitProcess(107);' in frozen_helper
        assert b'CreateFileA(executable,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL)' in frozen_helper
    before_os = os_environment(read('observer/invoke-before-os-environment.bin'))
    spawn_os = os_environment(read('observer/spawn-before-os-environment.bin'))
    assert spawn_os.keys() - before_os.keys() == {'MSC_CMD_FLAGS'} and not before_os.keys() - spawn_os.keys()
    assert all(spawn_os[key] == value for key, value in before_os.items())
    assert spawn_os['MSC_CMD_FLAGS'] == flags
    ir = {name: record for name, record in manifest['files'].items() if name.startswith('observer/c1-output/')}
    assert len(ir) == 4
    assert {Path(name).name for name in ir} == {prefix[2:] + suffix for suffix in ('ex', 'gl', 'in', 'sy')}
    for name, record in ir.items():
        if complete:
            assert record == manifest['files']['observer/c2-input/' + Path(name).name]
            assert Path(name).name not in manifest['files'], 'normal CL cleanup did not remove IR'
        else:
            assert record == manifest['files'][Path(name).name]
    access = original_states(directory, ['access-9-before', 'access-9-after'], 'access-events.txt')
    assert access == {'access-9-before': {'errno': 0, 'doserrno': 2, 'last_error': 0},
                      'access-9-after': {'errno': 2, 'doserrno': 2, 'last_error': 2}}
    assert read('access-9-path.bin').endswith(b'\\c2\0')
    assert re.search(r'call sequence=9 .* mode=4\n', read('access-events.txt').decode('ascii'))
    assert 'return sequence=9 result=ffffffff\n' in read('access-events.txt').decode('ascii')
    putenv = []
    if complete:
        for call_number in (1, 2):
            start, end = 'putenv-%d-before' % call_number, 'putenv-%d-after' % call_number
            assignment = read('observer/' + start + '-assignment.bin')
            assert assignment.endswith(b'\0') and b'\0' not in assignment[:-1]
            expected_assignment = 'MSC_CMD_FLAGS=' + (flags if call_number == 2 else '')
            assert assignment == expected_assignment.encode('ascii') + b'\0'
            assert u32(read('observer/' + start + '-stack.bin'), 0)
            assert u32(contexts[start], 196) == u32(contexts[end], 196) and u32(contexts[end], 176) == 0
            assert states[start] == states[end] == {'errno': 2, 'doserrno': 2, 'last_error': 2}
            envs = [environment(read('observer/' + p + '-crt-environment.bin')) for p in (start, end)]
            osenvs = [os_environment(read('observer/' + p + '-os-environment.bin')) for p in (start, end)]
            vectors = [vector(read('observer/' + p + '-crt-environment.bin')) for p in (start, end)]
            assert len({pointer for pointer, text in vectors[0][1]}) == len(envs[0])
            assert len({pointer for pointer, text in vectors[1][1]}) == len(envs[1])
            if call_number == 1:
                assert {pointer for pointer, text in vectors[0][1]}.isdisjoint(pointer for pointer, text in vectors[1][1]), 'initial putenv string ownership overlaps'
            assert [text for pointer, text in vectors[1][1]][:len(envs[0])] == [text for pointer, text in vectors[0][1]], 'putenv changed existing entry order'
            assert vectors[0][0] != vectors[1][0], 'putenv vector ownership transition missing'
            identities = []
            for index, ((old_pointer, text), (new_pointer, new_text)) in enumerate(zip(vectors[0][1], vectors[1][1])):
                assert text == new_text
                assert (old_pointer == new_pointer) == (call_number == 2), 'putenv existing string identity transition differs'
                name = text[:text.find(b'=', 1)].decode('ascii')
                identities.append({'index': index, 'name': name, 'string_sha256': digest(text),
                                   'before_pointer': old_pointer, 'after_pointer': new_pointer,
                                   'pointer_retained': old_pointer == new_pointer})
            assert 'MSC_CMD_FLAGS' not in envs[0] and 'MSC_CMD_FLAGS' not in osenvs[0]
            assert all(envs[1].get(key) == value for key, value in envs[0].items())
            assert all(osenvs[1].get(key) == value for key, value in osenvs[0].items())
            if call_number == 1:
                assert envs[0] == envs[1] and osenvs[0] == osenvs[1]
            else:
                assert envs[1].keys() - envs[0].keys() == {'MSC_CMD_FLAGS'}
                assert osenvs[1].keys() - osenvs[0].keys() == {'MSC_CMD_FLAGS'}
                assert envs[1]['MSC_CMD_FLAGS'] == flags.encode('ascii') and osenvs[1]['MSC_CMD_FLAGS'] == flags
            for key in ('argv', 'argument_pointers', 'argument_vector', 'cwd', 'file0_2_words', 'streams', 'open_descriptors',
                        'mb_codepage', 'lc_codepage', 'lc_handles', 'mb_state_sha256', 'ctype_sha256', 'x87_control_word'):
                assert snapshots[start][key] == snapshots[end][key], 'additional putenv caller-state change: ' + key
            putenv.append({'assignment': expected_assignment, 'result': 0, 'errors_before': states[start],
                           'errors_after': states[end], 'crt_count_before': len(envs[0]), 'crt_count_after': len(envs[1]),
                           'os_count_before': len(osenvs[0]), 'os_count_after': len(osenvs[1]),
                           'argument_address': u32(read('observer/' + start + '-stack.bin'), 0),
                           'crt_vector_before': u32(read('observer/' + start + '-crt-words.bin'), 4),
                           'crt_vector_after': u32(read('observer/' + end + '-crt-words.bin'), 4),
                           'existing_entry_identities': identities,
                           'new_entries': [{'index': index, 'pointer': pointer, 'string_sha256': digest(text)}
                                           for index, (pointer, text) in enumerate(vectors[1][1]) if index >= len(envs[0])]})
        assert states['spawn-before'] == {'errno': 0, 'doserrno': 2, 'last_error': 2}
        assert states['spawn-after'] == {'errno': 0, 'doserrno': 0, 'last_error': 2}
        assert u32(contexts['spawn-before'], 196) == u32(contexts['spawn-after'], 196)
        assert u32(contexts['spawn-after'], 176) == 0
        for key in ('x87_control_word', 'mb_state_sha256', 'ctype_sha256', 'crt_environment_sha256',
                    'os_environment_sha256', 'file0_2_words', 'streams', 'open_descriptors', 'argv', 'cwd'):
            assert snapshots['spawn-before'][key] == snapshots['spawn-after'][key], 'unexpected child-return change: ' + key
        assert read('observer/spawn-executable.bin') == read('C2.EXE')
        assert read('observer/c2-output/test.obj') == read('test.obj')
        assert len(read('test.obj')) == 275 and read('stdout.bin') == b'test.c\r\n' and read('stderr.bin') == b''
    result = {'kind': 'analysis only; not function acceptance or native dependency-provider approval',
            'manifest': {'path': str(manifest_path), 'sha256': digest(manifest_path.read_bytes())},
            'decoder_sha256': digest(Path(__file__).read_bytes()),
            'canonical_target': TARGET_SHA, 'canonical_crt': CRT_SHA, 'canonical_c1': C1_SHA,
            'timing': capture['timing'], 'phases': snapshots,
            'invoke': {'argc': 26, 'last_pass': 1, 'status': 0, 'stack_cleanup': 12,
                       'argv_bytes_unchanged': True, 'exports': c1_exports},
            'loader_calls': loader_calls,
            'next_child': {'not_started': not complete, 'argv': snapshots['spawn-before']['argv'],
                           'raw_arguments': {'mode': spawn_mode, 'path_pointer': spawn_path, 'argv_pointer': spawn_argv},
                           'absolute_executable_path': expected_child_path,
                           'path_bytes_evidence': 'verified frozen helper compares remote path and argv0 bytes before opening that exact path' if complete else 'only argv0 bytes captured; path byte comparison pending',
                           'added_crt_and_os_environment': {'MSC_CMD_FLAGS': flags}, 'complete_ir': ir},
            'putenv': putenv,
            'first_post_invoke_access': {'mode': 4, 'result': -1, 'path': read('access-9-path.bin')[:-1].decode('ascii'), **access},
            'limits': ['preferred-base DLL only; nonpreferred relocation not directly tested',
                       'closed FILE/lowio slots and undefined buffer tails retained raw, not replayable initialized data',
                       'no individual post-Invoke multibyte API return observation',
                       'no callback-registration inventory; imports alone do not establish absence']}
    if complete:
        result['next_child'].update(status=0, errors_before=states['spawn-before'], errors_after=states['spawn-after'],
            executable=manifest['files']['observer/spawn-executable.bin'],
            outputs={'test.obj': manifest['files']['observer/c2-output/test.obj']})
        result['completed_case'] = {'status': 0, 'stdout': manifest['files']['stdout.bin'],
                                    'stderr': manifest['files']['stderr.bin'], 'test.obj': manifest['files']['test.obj'],
                                    'intermediate_files_removed': True}
        result['limits'].append('C2 full absolute _spawnvp path/input bytes observed; no child loaded-hFile event under DEBUG_ONLY_THIS_PROCESS')
    else:
        result['limits'] += ['no C2 execution/response or process-exit observation',
                             '_putenv pre/post call and OS environment effects not isolated yet']
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', required=True, type=Path)
    parser.add_argument('--manifest', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    freeze(args.capture, args.manifest)
    value = decode(args.manifest)
    payload = json.dumps(value, indent=2) + '\n'
    assert not args.output.exists() or args.output.read_text() == payload, 'immutable decoded packet differs'
    args.output.write_text(payload)
    print('Dependency analysis packet verified:', args.output, '; native success boundary remains closed')
