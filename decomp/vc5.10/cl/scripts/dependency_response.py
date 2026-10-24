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
CHILD_PHASES=PHASES[:-1]+('free-before','c1-entry-0','free-after','putenv-2-before',
                         'putenv-2-after','spawn-before','spawn-after','frontier')
digest = lambda value: hashlib.sha256(value).hexdigest()


def phase_snapshot(directory, read, phase, errors, mre_prefix=False, mre_closed=False):
    """Decode addressed original records, including the distinct handle tables."""
    events = read('observer/mre-events.txt'if mre_prefix else'observer/events.txt').decode('ascii')
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
    if words[20]:
        argv_pointer, argv = vector(read('observer/' + phase + '-caller-argv.bin'))
    else:
        assert mre_prefix or phase in ('frontier','exit-before'),'missing live argv at a dependency call'
        argv_pointer,argv=0,[]
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
    if mre_closed:
        assert mre_prefix and phase in('mre-6-after','mre-6-after-copy-after')
        assert read('mre-observer.txt')==b'CL_MRE_JOBS_RESPONSE_V1\n'
        assert u32(lowio,108)==0xffffffff and not lowio[112]&1,'fd3 is not actually closed'
    expected_fds=[0,1,2,3]if mre_prefix and phase!='mre-1-before'and not mre_closed else[0,1,2]
    assert [item['fd'] for item in descriptors] == expected_fds
    for index in range(3):
        assert lowio[index * 36 + 4] & 1
        assert u32(lowio, index * 36) == streams['crt'][index]['handle']
        assert u32(parameters, 0x18 + index * 4) == streams['win32'][index]['handle']
        assert descriptors[index]['query'] == streams['crt'][index]
    for index in expected_fds[3:]:
        assert lowio[index*36+4]&1 and u32(lowio,index*36)==descriptors[index]['query']['handle']
    assert not any(lowio[index * 36 + 4] & 1 for index in range(len(expected_fds), 32))
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
        'pass_address': words[21], 'pass_words': (None if mre_prefix else list(struct.unpack('<9I', read('observer/' + phase + '-caller-pass.bin')))),
        'file0_2_words': list(struct.unpack('<24I', iob[:96])), 'streams': streams,
        'open_descriptors': descriptors, 'directory': directory_entries(read('observer/' + phase + '-directory.bin')), **texts}
    # This pointer refers to contextual original .rdata, not an observer's
    # guessed spelling of the pass assignment prefix (which includes '=').
    if mre_prefix:
        assert words[20:23]==(0,0,0),'MRE prefix must not invent a compiler-pass caller'
        snapshot['pass_environment']=None
    else:
        snapshot['pass_environment'] = (PE(directory / 'loaded-target.bin').string(snapshot['pass_words'][2])
                                        if snapshot['pass_words'][2] else None)
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
    assert '_execute_pass' in config['accepted']
    full_stage='_unload_pass'in config['accepted']
    cleanup_stage='_cleanup_files'in config['accepted']
    deletion_stage='_delete_temporary'in config['accepted']
    completed='_driver_exit'in config['accepted']
    assert not completed or deletion_stage and '_next_jobs'in config['accepted']
    terminal='exit-before'if completed else'frontier'
    scheduler_stage='_next_jobs'in config['accepted']
    next_address='00404fda' if scheduler_stage else '00403709' if deletion_stage else '00404f6f' if cleanup_stage else '00404ed1' if full_stage else '004054ec'
    next_frontier=None if completed else 'driver_exit' if scheduler_stage else 'next_jobs' if deletion_stage else 'delete_temporary' if cleanup_stage else 'cleanup_files' if full_stage else 'unload_pass'
    phases_expected=(CHILD_PHASES[:-1]+('exit-before',)if completed else CHILD_PHASES)if full_stage else PHASES
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
    assert re.findall(r'^trap .*address=([0-9a-f]{8}) ', target_events, re.M) == ([]if completed else[next_address])
    assert re.findall(r'^exit .*status=(\d+)$', target_events, re.M) == (['0']if completed else['28673']), 'missing actual terminal/controlled stop status'
    assert not re.search(r'analysis-stop|exception ', target_events)
    assert 'analysis-stop' not in events
    if not full_stage:
        assert not re.search(r'^module-unload .*base=10600000$', events, re.M)
        assert 'analysis-c2-completed' not in events
    errors = original_states(directory, phases_expected, 'observer/events.txt')
    phases, contexts = {}, {}
    for name in phases_expected:
        phases[name], contexts[name] = phase_snapshot(directory, read, name, errors[name])
        assert phases[name]['pass_environment']=='MSC_CMD_FLAGS=', 'minimal-C C1/C2 pass environment must be non-NULL and exact'
    if cleanup_stage and not deletion_stage:
        assert u32(read('observer/frontier-stack.bin'),4)==0,'unexpected first temporary suffix index'
    assert [phases[name]['event'] for name in phases_expected] == sorted(phases[name]['event'] for name in phases_expected)
    c1 = PE(directory / 'observer/loaded-c1.bin')
    assert c1.base == 0x10600000 and c1.entry == 0x1202a
    assert exports(c1)['_InvokeCompilerPass@12'] == {'ordinal': 2, 'rva': 0x12229}
    assert exports(c1)['_AbortCompilerPass@4'] == {'ordinal': 1, 'rva': 0x43975}
    loaded = re.findall(r'^c1-load event=(\d+) base=10600000 entry=1061202a entry_byte=53$', events, re.M)
    assert len(loaded) == 1 and phases['load-before']['event'] < int(loaded[0]) < phases['c1-entry-1']['event'] < phases['load-after']['event']
    assert re.findall(r'^c1-entry .*reason=(\d+)', events, re.M) == (['1','0']if full_stage else['1'])
    assert struct.unpack_from('<3I', read('observer/c1-entry-1-stack.bin'), 4) == (c1.base, 1, 0)
    for name, result, cleanup in (('load', c1.base, 4), ('get-invoke', c1.base + 0x12229, 8), ('get-abort', c1.base + 0x43975, 8)):
        before, after = contexts[name + '-before'], contexts[name + '-after']
        assert u32(after, 176) == result and u32(after, 196) - u32(before, 196) == cleanup
        parameters = struct.unpack_from('<2I', read('observer/' + name + '-before-stack.bin'))
        assert parameters[0] == (phases['load-before']['pass_words'][0] if name == 'load' else c1.base)
        if name != 'load': assert parameters[1] == (0x408524 if name == 'get-invoke' else 0x40850c)
    assert read('observer/get-invoke-before-argument.bin') == b'_InvokeCompilerPass@12\0'
    assert read('observer/get-abort-before-argument.bin') == b'_AbortCompilerPass@4\0'
    before, after, frontier = [phases[name] for name in ('invoke-before', 'invoke-after', terminal)]
    assert before['pass_words'][5:8]==after['pass_words'][5:8]==[c1.base,c1.base+0x12229,c1.base+0x43975]
    invocation = re.findall(r'^invoke-call argc=(\d+) argv=([0-9a-f]{8}) last=(\d+) function=([0-9a-f]{8}) esp=([0-9a-f]{8})$', events, re.M)
    assert len(invocation) == 1
    argc, argv, last, function, esp = invocation[0]
    assert (int(argc), int(last), int(function, 16)) == (26, 1, c1.base + 0x12229)
    assert int(argv, 16) == before['argument_vector']
    assert struct.unpack_from('<3I', read('observer/invoke-before-stack.bin')) == (26, int(argv, 16), 1)
    assert u32(contexts['invoke-after'], 196) - int(esp, 16) == 12 and u32(contexts['invoke-after'], 176) == 0
    assert before['errors'] == {'errno': 0, 'doserrno': 2, 'last_error': 2}
    assert after['errors'] == {'errno': 0, 'doserrno': 2, 'last_error': 0}
    assert (before['mb_codepage'], before['lc_codepage'], before['lc_handles']) == (0, 0, [0] * 6)
    assert (after['mb_codepage'], after['lc_codepage'], after['lc_handles']) == (1252, 1252, [0, 0, 1033, 0, 0, 0])
    stable = ('x87_control_word', 'fmode', 'commode', 'mb_cur_max', 'crt_environment_sha256', 'os_environment_sha256',
              'argv', 'argument_pointers', 'argument_vector', 'cwd', 'file0_2_words', 'streams', 'open_descriptors')
    assert all(before[name] == after[name] for name in stable)
    if not full_stage:
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
    if full_stage:
        contracts[3]=expectation(phases['free-before'],read('observer/free-before-crt-environment.bin'),
            read('environment.bin'),read('observer/free-before-os-environment.bin'),
            read('observer/free-before-mb-state.bin'),read('observer/free-before-ctype.bin'),
            read('observer/free-before-execution-words.bin'),3)
    ir = {Path(name).name: record for name, record in capture['files'].items() if name.startswith('observer/c1-output/')}
    assert set(ir) == {prefix[2:] + suffix for suffix in ('ex', 'gl', 'in', 'sy')}
    if not deletion_stage:assert all(capture['files'][name] == record for name, record in ir.items())
    if not full_stage:assert not any(name.startswith('observer/c2-') for name in capture['files'])
    assert read('stdout.bin') == b'test.c\r\n' and not read('stderr.bin')
    result={'kind': 'fresh masked dependency response; application acceptance requires independent native frontier proof',
            'capture_sha256': digest(capture_bytes), 'config_sha256': capture['config_sha256'],
            'mask_sha256': capture['original_mask_sha256'], 'contracts': contracts,
            'dos_cwd': before['cwd'],
            'invoke_argv': before['argv'], 'initial_environment': [text.decode('latin1') for pointer, text in initial_env],
            'initial_os_environment_hex': read('observer/putenv-1-before-os-environment.bin').hex(),
            'post_mbctype_hex': read('observer/invoke-after-mb-state.bin')[4:].hex(),
            'post_ctype_hex': read('observer/invoke-after-ctype.bin').hex(),
            'lowio_flags': [read('observer/invoke-before-lowio-0.bin')[36 * index + 4] for index in range(3)],
            'outputs': ir, 'sequence': [0, 1, 2,3]if full_stage else[0,1,2],
            'next_frontier': next_frontier, 'completed':completed,
            'decoder_sha256': digest(Path(__file__).read_bytes())}
    if full_stage:result['child']=child_response(directory,read,capture,events,phases,contexts,ir,prefix,
                                               frontier_argument=0 if cleanup_stage else 1,deletion_stage=deletion_stage,scheduler_stage=scheduler_stage,terminal=terminal)
    if deletion_stage:result['deletions']=deletion_response(directory,read,capture,events,phases,prefix,ir,terminal)
    if completed:
        calls=re.findall(r'^exit-call status=(\d+) function=([0-9a-f]{8}) esp=([0-9a-f]{8})$',events,re.M)
        crt=PE(directory/'loaded-msvcrt.bin')
        assert len(calls)==1 and calls[0][0]=='0' and int(calls[0][1],16)==crt.base+exports(crt)['exit']['rva']
        assert int(calls[0][2],16)==u32(contexts[terminal],196)
        assert u32(read('observer/exit-before-stack.bin'),0)==0,'actual exit formal differs'
        result['exit']={'status':0,'errors':phases[terminal]['errors'],'x87_control_word':phases[terminal]['x87_control_word']}
    return result


def child_response(directory,read,capture,events,phases,contexts,ir,prefix,frontier_argument,deletion_stage,scheduler_stage,terminal):
    after=phases['invoke-after']
    assert phases['free-before']['pass_words'][5:8]==after['pass_words'][5:8]
    for phase in ('free-before','c1-entry-0','free-after'):
        for name in ('errors','mb_codepage','lc_codepage','lc_handles','mb_state_sha256','ctype_sha256',
                     'x87_control_word','crt_environment_sha256','os_environment_sha256','file0_2_words','streams'):
            assert phases[phase][name]==after[name],'unreviewed detach/free effect: '+name
    assert struct.unpack_from('<3I',read('observer/c1-entry-0-stack.bin'),4)==(0x10600000,0,0)
    unload=re.findall(r'^module-unload event=(\d+) base=10600000$',events,re.M)
    assert len(unload)==1 and phases['c1-entry-0']['event']<int(unload[0])<phases['free-after']['event']
    assert u32(contexts['free-after'],176)==1 and u32(contexts['free-after'],196)-u32(contexts['free-before'],196)==4
    expected_flags='-il %s -f test.c -W 1 -G5 -Gs4096 -dos -Fotest.obj -ML -Fdvc50.idb '%prefix
    assert read('observer/putenv-2-before-assignment.bin')==('MSC_CMD_FLAGS='+expected_flags).encode('ascii')+b'\0'
    old_address,old=vector(read('observer/putenv-2-before-crt-environment.bin'))
    new_address,new=vector(read('observer/putenv-2-after-crt-environment.bin'))
    assert old_address!=new_address and len(old)==78 and len(new)==79 and new[:78]==old
    assert new[-1][1]==('MSC_CMD_FLAGS='+expected_flags).encode('ascii')
    from dependency_packet import os_environment
    old_os=read('observer/putenv-2-before-os-environment.bin')
    new_os=read('observer/putenv-2-after-os-environment.bin')
    assert old_os[:-2]+('MSC_CMD_FLAGS='+expected_flags+'\0\0').encode('utf-16le')==new_os
    assert list(os_environment(new_os))[-1]=='MSC_CMD_FLAGS'
    assert phases['putenv-2-before']['errors']==phases['putenv-2-after']['errors']=={'errno':2,'doserrno':2,'last_error':2}
    assert u32(contexts['putenv-2-after'],176)==0 and u32(contexts['putenv-2-before'],196)==u32(contexts['putenv-2-after'],196)
    for name in ('argv','argument_pointers','argument_vector','cwd','file0_2_words','streams','mb_state_sha256','ctype_sha256','x87_control_word'):
        assert phases['putenv-2-before'][name]==phases['putenv-2-after'][name]
    before,after,frontier=[phases[name]for name in ('spawn-before','spawn-after',terminal)]
    assert before['errors']=={'errno':0,'doserrno':2,'last_error':2} and after['errors']=={'errno':0,'doserrno':0,'last_error':2}
    if not deletion_stage:assert frontier['errors']==after['errors']
    assert u32(contexts['spawn-after'],176)==0 and u32(contexts['spawn-before'],196)==u32(contexts['spawn-after'],196)
    mode,path,args=struct.unpack_from('<3I',read('observer/spawn-before-stack.bin'))
    assert mode==0 and path==before['pass_words'][0] and args==before['argument_vector']
    executable=before['cwd'].rstrip('\\')+'\\c2.exe'
    assert before['argv']==[executable]
    helper=read('dependency-observer.inc')
    assert b'if (strcmp(executable,argument)) ExitProcess(107);'in helper
    assert b'CreateFileA(executable,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL)'in helper
    assert read('observer/spawn-executable.bin')==read('C2.EXE')
    assert digest(read('MSVCRT.DLL'))==CRT_SHA
    assert digest(read('MSPDB50.DLL'))=='cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33'
    assert events.count('analysis-c2-completed result=00000000 putenv_calls=2\n')==1
    for name in ('x87_control_word','fmode','commode','mb_cur_max','mb_codepage','lc_codepage','lc_handles',
                 'crt_environment_sha256','os_environment_sha256','mb_state_sha256','ctype_sha256','file0_2_words','streams'):
        assert before[name]==after[name]==frontier[name],'unreviewed child/frontier state: '+name
    assert not frontier['argv'] and frontier['argument_vector']==0 and frontier['pass_words'][5:8]==[0,0,0]
    if terminal=='exit-before':
        assert u32(read('observer/exit-before-stack.bin'),0)==0,'unexpected actual exit formal'
    elif scheduler_stage:
        assert u32(read('context.bin'),208)==0,'unexpected actual final status argument'
    elif deletion_stage:
        context=read('context.bin')
        state,pending=struct.unpack_from('<II',context,208)
        assert state==0x4095d8 and pending==u32(context,180)-24,'next_jobs actual formal/caller identity differs'
    else:
        assert struct.unpack_from('<I',read('context.bin'),208)[0]==frontier_argument,'unexpected cleanup/helper argument'
    for name,record in ir.items():
        assert capture['files']['observer/c2-input/'+name]==record
        if not deletion_stage:assert capture['files'][name]==record
    assert read('observer/c2-output/test.obj')==read('test.obj') and len(read('test.obj'))==275
    return {'mode':0,'executable':executable,'argv':[executable],'environment_source':'target-os',
            'environment':new_os.decode('utf-16le')[:-2].split('\0'),'status':0,
            'before':before['errors'],'after':after['errors'],'IR_inputs':ir,
            'outputs':{'test.obj':capture['files']['observer/c2-output/test.obj']},
            'executable_input':capture['files']['observer/spawn-executable.bin'],
            'provider_inputs':{name:capture['files'][name]for name in ('MSVCRT.DLL','MSPDB50.DLL')},
            'stdout_hex':'','stderr_hex':''}


def deletion_response(directory,read,capture,events,phases,prefix,ir,terminal):
    """Fresh actually executed CRT calls; never a replayed filesystem effect."""
    names=[prefix+suffix for suffix in (('ex','sy','gl','in','st','db','lk')if terminal=='exit-before'else('ex','sy','gl','in','st','db'))]
    phase_names=[f'unlink-{index}-{when}'for index in range(1,len(names)+1)for when in ('before','after')]
    errors=original_states(directory,phase_names,'observer/events.txt')
    from dependency_packet import exports
    crt=PE(directory/'loaded-msvcrt.bin')
    function=crt.base+exports(crt)['_unlink']['rva']
    calls=re.findall(r'^unlink-call sequence=(\d+) function=([0-9a-f]{8}) argument=([0-9a-f]{8}) esp=([0-9a-f]{8})$',events,re.M)
    returns=re.findall(r'^unlink-return sequence=(\d+) result=([0-9a-f]{8}) esp=([0-9a-f]{8})$',events,re.M)
    assert len(calls)==len(returns)==len(names),'missing/extra unlink invocation'
    records=[]
    previous=phases['spawn-after']['errors']
    for index,(filename,call,returned)in enumerate(zip(names,calls,returns),1):
        before,after=f'unlink-{index}-before',f'unlink-{index}-after'
        assert int(call[0])==int(returned[0])==index and int(call[1],16)==function
        text=read('observer/'+before+'-filename.bin')
        assert text==filename.encode('ascii')+b'\0'
        pre=read('observer/'+before+'-context.bin');post=read('observer/'+after+'-context.bin')
        stack=read('observer/'+before+'-stack.bin')
        assert len(pre)==len(post)==204 and len(stack)==64
        argument=u32(stack,0)
        assert argument==int(call[2],16)==u32(pre,180)-1024,'unlink input is not helper local filename'
        assert u32(pre,196)==int(call[3],16)==u32(post,196)==int(returned[2],16),'unlink cdecl stack differs'
        status=struct.unpack('<i',struct.pack('<I',int(returned[1],16)))[0]
        assert u32(post,176)==int(returned[1],16)
        before_files=directory_entries(read('observer/'+before+'-directory.bin'))
        after_files=directory_entries(read('observer/'+after+'-directory.bin'))
        key=filename[2:]
        assert errors[before]==previous,'unobserved error mutation between unlink calls'
        present=key in before_files
        assert status==(0 if present else -1) and key not in after_files
        for other in [prefix[2:]+suffix for suffix in ('ex','sy','gl','in','st','db','lk')]+['test.obj']:
            if other!=key:assert before_files.get(other)==after_files.get(other),'unlink changed another target artifact'
        assert present==(key in ir)
        if present:
            assert before_files[key]['bytes']==ir[key]['bytes']
            assert key not in capture['files']
        records.append({'filename':filename,'present_before':present,'present_after':False,
            'result':status,'before':errors[before],'after':errors[after],'argument_is_local_buffer':True})
        previous=errors[after]
    assert previous==phases[terminal]['errors']
    assert not any(name in capture['files']for name in ir),'temporary IR survived cleanup'
    return records
