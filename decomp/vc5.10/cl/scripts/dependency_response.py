"""A captured C1 response from a fresh masked CL trial, never an analysis oracle.

The first dispatcher trial must stop at the genuine unload_pass entry. C2 and
later completion need their own observed boundary; this reader does not invent
them from the earlier unmasked analysis.
"""
import hashlib
import json
from pathlib import Path
import re
import struct

from binary import PE, u32
from case_inputs import verify_dependencies, load as load_case
from dependency_packet import C1_SHA, TARGET_SHA, exports, vector, directory_entries
from dependency_native import expectation
from mask import construct
from mktemp_state import CRT_SHA, original_states

PHASES = ('putenv-1-before', 'putenv-1-after', 'load-before', 'c1-entry-1',
          'load-after', 'get-invoke-before', 'get-invoke-after', 'get-abort-before',
          'get-abort-after', 'invoke-before', 'invoke-after', 'frontier')
digest = lambda value: hashlib.sha256(value).hexdigest()


def phase_snapshot(directory, read, phase, errors):
    """Decode addressed original records, including the distinct handle tables."""
    events = read('observer/events.txt').decode('ascii')
    matches = re.findall(r'^snapshot phase=' + phase + r' event=(\d+) tid=(\d+) eax=([0-9a-f]{8}) esp=([0-9a-f]{8}) ebp=([0-9a-f]{8})$', events, re.M)
    assert len(matches) == 1, 'missing or repeated original dependency state: ' + phase
    event, thread, eax, esp, ebp = matches[0]
    context = read('observer/' + phase + '-context.bin')
    assert len(context) == 204 and u32(context, 0) & 8
    assert (u32(context, 176), u32(context, 196), u32(context, 180)) == tuple(int(n, 16) for n in (eax, esp, ebp))
    words = struct.unpack('<32I', read('observer/' + phase + '-crt-words.bin'))
    assert words[0] == 0x78000000 and words[23:] == (0,) * 9
    mb = read('observer/' + phase + '-mb-state.bin')
    ctype = read('observer/' + phase + '-ctype.bin')
    assert len(mb) == 261 and len(ctype) == 514 and u32(mb, 0) == words[6]
    parameters = read('observer/' + phase + '-parameters.bin')
    assert len(parameters) == 96 and u32(parameters, 0x48) == words[19]
    texts = {}
    for name, offset in (('cwd', 0x24), ('image', 0x38), ('command', 0x40)):
        value = read('observer/' + phase + '-' + name + '-utf16.bin')
        length, capacity, pointer = struct.unpack_from('<HHI', parameters, offset)
        assert pointer and capacity >= length and len(value) == length + 2 and value[-2:] == b'\0\0'
        texts[name] = value[:-2].decode('utf-16le')
    env_pointer, env = vector(read('observer/' + phase + '-crt-environment.bin'))
    argv_pointer, argv = vector(read('observer/' + phase + '-caller-argv.bin'))
    assert (env_pointer, argv_pointer) == (words[1], words[20])
    streams = json.loads(read('streams-' + phase + '.json'))
    teb, peb = read('streams-' + phase + '-teb.bin'), read('streams-' + phase + '-peb.bin')
    assert teb == read('mktemp-' + phase + '-teb.bin')
    assert u32(teb, 0x18) == streams['teb'] and u32(teb, 0x30) == streams['peb'] == words[17]
    assert u32(peb, 0x10) == streams['parameters'] == words[18]
    assert read('streams-' + phase + '-parameters.bin') == parameters[:36]
    lowio = read('observer/' + phase + '-lowio-0.bin')
    iob = read('observer/' + phase + '-static-iob.bin')
    assert len(lowio) == 1152 and len(iob) == 640
    assert read('streams-' + phase + '-lowio.bin') == lowio[:108]
    assert read('streams-' + phase + '-iob.bin') == iob[:96]
    assert u32(read('streams-' + phase + '-pioinfo-slot.bin'), 0) == streams['lowio']
    assert u32(read('observer/' + phase + '-pioinfo-pointers.bin'), 0) == streams['lowio']
    descriptors = json.loads(read('observer/' + phase + '-open-descriptors.json'))
    assert [item['fd'] for item in descriptors] == [0, 1, 2]
    for index in range(3):
        assert lowio[index * 36 + 4] & 1
        assert u32(lowio, index * 36) == streams['crt'][index]['handle']
        assert u32(parameters, 0x18 + index * 4) == streams['win32'][index]['handle']
        assert descriptors[index]['query'] == streams['crt'][index]
    assert not any(lowio[index * 36 + 4] & 1 for index in range(3, 32))
    # Other allocated lowio blocks must not hide an additional open descriptor.
    pointers = struct.unpack('<64I', read('observer/' + phase + '-pioinfo-pointers.bin'))
    for index, pointer in enumerate(pointers[1:], 1):
        if pointer:
            other = read('observer/' + phase + '-lowio-%d.bin' % index)
            assert len(other) == 1152 and not any(other[k * 36 + 4] & 1 for k in range(32))
    snapshot = {'event': int(event), 'thread': int(thread), 'errors': errors,
        'x87_control_word': u32(context, 28) & 0xffff, 'fmode': words[3], 'commode': words[4],
        'mb_cur_max': words[5], 'mb_codepage': words[6], 'lc_codepage': words[7],
        'lc_handles': list(words[8:14]), 'pctype_address': words[14],
        'mb_state_sha256': digest(mb), 'ctype_sha256': digest(ctype),
        'crt_environment_count': len(env), 'crt_environment_sha256': digest(read('observer/' + phase + '-crt-environment.bin')),
        'os_environment_sha256': digest(read('observer/' + phase + '-os-environment.bin')),
        'argv': [text.decode('ascii') for pointer, text in argv],
        'argument_pointers': [pointer for pointer, text in argv], 'argument_vector': argv_pointer,
        'pass_address': words[21], 'pass_words': list(struct.unpack('<9I', read('observer/' + phase + '-caller-pass.bin'))),
        'file0_2_words': list(struct.unpack('<24I', iob[:96])), 'streams': streams,
        'open_descriptors': descriptors, 'directory': directory_entries(read('observer/' + phase + '-directory.bin')), **texts}
    # This pointer refers to contextual original .rdata, not an observer's
    # guessed spelling of the pass assignment prefix (which includes '=').
    snapshot['pass_environment'] = PE(directory / 'loaded-target.bin').string(snapshot['pass_words'][2])
    return snapshot, context


def decode(directory):
    directory = Path(directory)
    capture_bytes = (directory / 'capture.json').read_bytes()
    capture = json.loads(capture_bytes)
    assert capture.get('dependency_response') is True and capture.get('baseline') is False
    assert not capture.get('dependency_analysis') and not capture.get('dependency_complete')
    assert capture['controller_error'] is None and capture['helper_status'] == 0
    assert capture['case'] == 'minimal-c' and capture['arguments'] == ['/nologo', '/c', 'test.c']
    assert capture['command'][-2] == '1', 'dependency response was not captured from a masked trial'
    def read(name):
        path = directory / name
        assert name in capture['files'] and path.is_file() and not path.is_symlink()
        value = path.read_bytes()
        assert capture['files'][name] == {'bytes': len(value), 'sha256': digest(value)}, 'original response input changed: ' + name
        return value
    for name in capture['files']: read(name)
    assert read('dependency-observer.txt') == b'CL_DEPENDENCY_RESPONSE_V1\n'
    config = json.loads(read('config.json'))
    assert digest(read('config.json')) == capture['config_sha256']
    assert '_execute_pass' in config['accepted'] and '_unload_pass' not in config['accepted']
    masked = construct(config)
    assert capture['target_sha256'] == TARGET_SHA
    assert digest(masked) == capture['original_mask_sha256'] == digest(read('CL.EXE')) == digest(read('loaded-target.bin'))
    assert digest(read('loaded-msvcrt.bin')) == capture['loaded_msvcrt_sha256'] == CRT_SHA
    assert digest(read('observer/loaded-c1.bin')) == digest(read('C1.DLL')) == C1_SHA
    assert digest(read('reference-debugger.exe')) == capture['helper_sha256'] == capture['helper_build']['binary']['sha256']
    for name, expected in capture['helper_sources'].items():
        assert digest(read(name)) == expected == capture['helper_build']['sources']['scripts/' + name]
    # These observations are mandatory in this version. An old helper binary
    # cannot be relabelled as having captured the new post-Invoke frontier.
    assert b'dependency_state("frontier"' in read('reference-debugger.c')
    assert b'"execution-words.bin"' in read('dependency-observer.inc')
    verify_dependencies(directory)
    case = load_case('minimal-c')
    assert capture['inputs'] == case['inputs']
    for item in case['inputs']:
        assert digest(read(item['name'])) == item['sha256'] and len(read(item['name'])) == item['bytes']
    events = read('observer/events.txt').decode('ascii')
    target_events = read('events.txt').decode('ascii')
    assert re.findall(r'^trap .*address=([0-9a-f]{8}) ', target_events, re.M) == ['004054ec']
    assert re.findall(r'^exit .*status=(\d+)$', target_events, re.M) == ['28673'], 'missing controlled trap-stop exit'
    assert not re.search(r'analysis-stop|exception ', target_events)
    assert 'analysis-stop' not in events and not re.search(r'^module-unload .*base=10600000$', events, re.M)
    assert 'analysis-c2-completed' not in events
    errors = original_states(directory, PHASES, 'observer/events.txt')
    phases, contexts = {}, {}
    for name in PHASES:
        phases[name], contexts[name] = phase_snapshot(directory, read, name, errors[name])
    assert [phases[name]['event'] for name in PHASES] == sorted(phases[name]['event'] for name in PHASES)
    c1 = PE(directory / 'observer/loaded-c1.bin')
    assert c1.base == 0x10600000 and c1.entry == 0x1202a
    assert exports(c1)['_InvokeCompilerPass@12'] == {'ordinal': 2, 'rva': 0x12229}
    assert exports(c1)['_AbortCompilerPass@4'] == {'ordinal': 1, 'rva': 0x43975}
    loaded = re.findall(r'^c1-load event=(\d+) base=10600000 entry=1061202a entry_byte=53$', events, re.M)
    assert len(loaded) == 1 and phases['load-before']['event'] < int(loaded[0]) < phases['c1-entry-1']['event'] < phases['load-after']['event']
    assert re.findall(r'^c1-entry .*reason=(\d+)', events, re.M) == ['1']
    assert struct.unpack_from('<3I', read('observer/c1-entry-1-stack.bin'), 4) == (c1.base, 1, 0)
    for name, result, cleanup in (('load', c1.base, 4), ('get-invoke', c1.base + 0x12229, 8), ('get-abort', c1.base + 0x43975, 8)):
        before, after = contexts[name + '-before'], contexts[name + '-after']
        assert u32(after, 176) == result and u32(after, 196) - u32(before, 196) == cleanup
        parameters = struct.unpack_from('<2I', read('observer/' + name + '-before-stack.bin'))
        assert parameters[0] == (phases['load-before']['pass_words'][0] if name == 'load' else c1.base)
        if name != 'load': assert parameters[1] == (0x408524 if name == 'get-invoke' else 0x40850c)
    assert read('observer/get-invoke-before-argument.bin') == b'_InvokeCompilerPass@12\0'
    assert read('observer/get-abort-before-argument.bin') == b'_AbortCompilerPass@4\0'
    before, after, frontier = [phases[name] for name in ('invoke-before', 'invoke-after', 'frontier')]
    invocation = re.findall(r'^invoke-call argc=(\d+) argv=([0-9a-f]{8}) last=(\d+) function=([0-9a-f]{8}) esp=([0-9a-f]{8})$', events, re.M)
    assert len(invocation) == 1
    argc, argv, last, function, esp = invocation[0]
    assert (int(argc), int(last), int(function, 16)) == (26, 1, c1.base + 0x12229)
    assert int(argv, 16) == before['argument_vector']
    assert struct.unpack_from('<3I', read('observer/invoke-before-stack.bin')) == (26, int(argv, 16), 1)
    assert u32(contexts['invoke-after'], 196) - int(esp, 16) == 12 and u32(contexts['invoke-after'], 176) == 0
    assert before['errors'] == {'errno': 0, 'doserrno': 2, 'last_error': 2}
    assert after['errors'] == frontier['errors'] == {'errno': 0, 'doserrno': 2, 'last_error': 0}
    assert (before['mb_codepage'], before['lc_codepage'], before['lc_handles']) == (0, 0, [0] * 6)
    assert (after['mb_codepage'], after['lc_codepage'], after['lc_handles']) == (1252, 1252, [0, 0, 1033, 0, 0, 0])
    stable = ('x87_control_word', 'fmode', 'commode', 'mb_cur_max', 'crt_environment_sha256', 'os_environment_sha256',
              'argv', 'argument_pointers', 'argument_vector', 'cwd', 'file0_2_words', 'streams', 'open_descriptors')
    assert all(before[name] == after[name] for name in stable)
    assert all(after[name] == frontier[name] for name in (*stable, 'errors', 'mb_codepage', 'lc_codepage', 'lc_handles', 'mb_state_sha256', 'ctype_sha256', 'pass_words'))
    assert frontier['pass_words'][5:8] == [c1.base, c1.base + 0x12229, c1.base + 0x43975]
    stack = struct.unpack_from('<16I', read('context.bin'), 204)
    assert stack[1] == frontier['pass_address'] == 0x4090cc
    prefix = before['argv'][2]
    assert re.fullmatch(r'\.\\a0[0-9]{4}', prefix)
    assert before['argv'][0] == before['cwd'].rstrip('\\') + '\\c1.dll'
    assert before['argv'][1:] == ['-il', prefix, '-f', 'test.c', '-W', '1', '-Ze', '-D_MSC_EXTENSIONS',
        '-Zp8', '-ZB64', '-D_INTEGRAL_MAX_BITS=64', '-D_M_IX86=500', '-G5', '-Gs', '-Ot', '-Ob0',
        '-Fotest.obj', '-pc', '\\:/', '-Fdvc50.pdb', '-D_MSC_VER=1100', '-D_WIN32', '-nologo', '-I', 'R:\\toolchains\\5.10\\include']
    initial_address, initial_env = vector(read('environment.bin'))
    putenv_before, putenv_after = phases['putenv-1-before'], phases['putenv-1-after']
    initial_raw = read('observer/putenv-1-before-crt-environment.bin')
    assert read('environment.bin') == initial_raw
    new_address, copied = vector(read('observer/putenv-1-after-crt-environment.bin'))
    assert initial_address != new_address and len(initial_env) == len(copied) == 78
    assert [text for pointer, text in initial_env] == [text for pointer, text in copied]
    assert set(pointer for pointer, text in initial_env).isdisjoint(pointer for pointer, text in copied)
    assert read('observer/putenv-1-before-assignment.bin') == b'MSC_CMD_FLAGS=\0'
    assert putenv_before['errors'] == putenv_after['errors'] == {'errno': 2, 'doserrno': 2, 'last_error': 2}
    assert u32(contexts['putenv-1-after'], 176) == 0 and u32(contexts['putenv-1-before'], 196) == u32(contexts['putenv-1-after'], 196)
    assert read('observer/putenv-1-before-os-environment.bin') == read('observer/putenv-1-after-os-environment.bin')
    assert all(putenv_before[key] == putenv_after[key] for key in ('argv', 'argument_pointers', 'argument_vector', 'cwd', 'file0_2_words', 'streams', 'mb_state_sha256', 'ctype_sha256', 'x87_control_word'))
    contracts = {}
    for kind, phase in ((1, 'load-before'), (2, 'invoke-before')):
        contracts[kind] = expectation(phases[phase], read('observer/' + phase + '-crt-environment.bin'),
            read('environment.bin'), read('observer/' + phase + '-os-environment.bin'),
            read('observer/' + phase + '-mb-state.bin'), read('observer/' + phase + '-ctype.bin'),
            read('observer/' + phase + '-execution-words.bin'), kind)
    ir = {Path(name).name: record for name, record in capture['files'].items() if name.startswith('observer/c1-output/')}
    assert set(ir) == {prefix[2:] + suffix for suffix in ('ex', 'gl', 'in', 'sy')}
    assert all(capture['files'][name] == record for name, record in ir.items())
    assert not any(name.startswith('observer/c2-') for name in capture['files'])
    assert read('stdout.bin') == b'test.c\r\n' and not read('stderr.bin')
    return {'kind': 'fresh masked C1 response; application acceptance requires independent native frontier proof',
            'capture_sha256': digest(capture_bytes), 'config_sha256': capture['config_sha256'],
            'mask_sha256': capture['original_mask_sha256'], 'contracts': contracts,
            'dos_cwd': before['cwd'],
            'invoke_argv': before['argv'], 'initial_environment': [text.decode('latin1') for pointer, text in initial_env],
            'initial_os_environment_hex': read('observer/putenv-1-before-os-environment.bin').hex(),
            'post_mbctype_hex': read('observer/invoke-after-mb-state.bin')[4:].hex(),
            'post_ctype_hex': read('observer/invoke-after-ctype.bin').hex(),
            'lowio_flags': [read('observer/invoke-before-lowio-0.bin')[36 * index + 4] for index in range(3)],
            'outputs': ir, 'sequence': [0, 1, 2], 'next_frontier': 'unload_pass',
            'decoder_sha256': digest(Path(__file__).read_bytes())}
