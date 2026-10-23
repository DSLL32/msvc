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
    assert events.count('analysis-stop-before-c2 invocations=1 unloads=1\n') == 1
    assert not re.search(r'^create .*parent=', read('events.txt').decode('ascii'), re.M), 'unexpected child process'
    states = original_states(directory, PHASES, 'observer/events.txt')
    snapshots = {}
    contexts = {}
    for phase in PHASES:
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
    assert [snapshots[p]['event'] for p in PHASES] == sorted(snapshots[p]['event'] for p in PHASES)
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
    assert re.search(r'c1-load event=110 base=10600000 entry=1061202a entry_byte=53', events)
    assert 'c1-entry event=111 module=10600000 reason=1 reserved=00000000' in events
    assert 'c1-entry event=120 module=10600000 reason=0 reserved=00000000' in events
    assert 'module-unload event=121 base=10600000' in events
    assert snapshots['free-after']['event'] == 122 and u32(contexts['free-after'], 176) == 1
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
    assert flags == '-il .\\a01900 -f test.c -W 1 -G5 -Gs4096 -dos -Fotest.obj -ML -Fdvc50.idb '
    assert len(snapshots['spawn-before']['argv']) == 1
    before_os = os_environment(read('observer/invoke-before-os-environment.bin'))
    spawn_os = os_environment(read('observer/spawn-before-os-environment.bin'))
    assert spawn_os.keys() - before_os.keys() == {'MSC_CMD_FLAGS'} and not before_os.keys() - spawn_os.keys()
    assert all(spawn_os[key] == value for key, value in before_os.items())
    assert spawn_os['MSC_CMD_FLAGS'] == flags
    ir = {name: record for name, record in manifest['files'].items() if name.startswith('observer/c1-output/')}
    assert len(ir) == 4
    assert {Path(name).name for name in ir} == {'a01900ex', 'a01900gl', 'a01900in', 'a01900sy'}
    for name, record in ir.items():
        assert record == manifest['files'][Path(name).name]
    access = original_states(directory, ['access-9-before', 'access-9-after'], 'access-events.txt')
    assert access == {'access-9-before': {'errno': 0, 'doserrno': 2, 'last_error': 0},
                      'access-9-after': {'errno': 2, 'doserrno': 2, 'last_error': 2}}
    assert read('access-9-path.bin').endswith(b'\\c2\0')
    assert re.search(r'call sequence=9 .* mode=4\n', read('access-events.txt').decode('ascii'))
    assert 'return sequence=9 result=ffffffff\n' in read('access-events.txt').decode('ascii')
    return {'kind': 'analysis only; not function acceptance or complete dependency replay oracle',
            'manifest': {'path': str(manifest_path), 'sha256': digest(manifest_path.read_bytes())},
            'decoder_sha256': digest(Path(__file__).read_bytes()),
            'canonical_target': TARGET_SHA, 'canonical_crt': CRT_SHA, 'canonical_c1': C1_SHA,
            'timing': capture['timing'], 'phases': snapshots,
            'invoke': {'argc': 26, 'last_pass': 1, 'status': 0, 'stack_cleanup': 12,
                       'argv_bytes_unchanged': True, 'exports': c1_exports},
            'next_child': {'not_started': True, 'argv': snapshots['spawn-before']['argv'],
                           'added_crt_and_os_environment': {'MSC_CMD_FLAGS': flags}, 'complete_ir': ir},
            'first_post_invoke_access': {'mode': 4, 'result': -1, 'path': read('access-9-path.bin')[:-1].decode('ascii'), **access},
            'limits': ['preferred-base DLL only; nonpreferred relocation not directly tested',
                       'no C2 execution/response or process-exit observation',
                       'closed FILE/lowio slots and undefined buffer tails retained raw, not replayable initialized data',
                       'no individual post-Invoke multibyte API return observation',
                       'no callback-registration inventory; imports alone do not establish absence',
                       '_putenv pre/post call and OS environment effects not isolated yet']}


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
