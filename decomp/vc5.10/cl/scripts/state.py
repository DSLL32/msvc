#!/usr/bin/env python3
"""Decode main-entry state from original memory blobs or actual native GDB values."""
import hashlib
import json
from pathlib import Path
import struct
import re

ENVIRONMENT_KEYS=('CL','_CL_','INCLUDE','TMP','PATH')
PROJECTION='cl-main-entry-v1: argc/argv, consumed environment and CRT stream/path/precision contract'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()


def vector(path):
    data=Path(path).read_bytes()
    address,count=struct.unpack_from('<II',data)
    assert count<=4096
    cursor=8;strings=[]
    for _ in range(count):
        pointer,length=struct.unpack_from('<II',data,cursor);cursor+=8
        assert pointer and 1<=length<=32768
        value=data[cursor:cursor+length];cursor+=length
        assert len(value)==length and value.endswith(b'\0') and b'\0' not in value[:-1]
        strings.append(value[:-1].decode('latin1'))
    assert cursor==len(data),'truncated or extra vector bytes'
    return address,strings


def environment(strings):
    result={name:None for name in ENVIRONMENT_KEYS}
    for value in strings:
        key,separator,text=value.partition('=')
        if separator and key.upper() in result:
            assert result[key.upper()] is None,'duplicate consumed Windows environment variable'
            result[key.upper()]=text
    return result


def original_main(directory):
    directory=Path(directory)
    context=(directory/'context.bin').read_bytes();crt=(directory/'crt-state.bin').read_bytes()
    assert len(context)==268 and len(crt)==132,'unexpected captured Win32 ABI size'
    assert struct.unpack_from('<I',context,0)[0]&8,'floating context was not captured'
    stack=struct.unpack_from('<16I',context,204)
    argv_address,argv=vector(directory/'argv.bin')
    env_address,env=vector(directory/'environment.bin')
    assert stack[1]==len(argv) and stack[2]==argv_address and stack[3]==env_address,'raw stack/vector alias differs'
    crtbase,fmode,commode,mb_cur_max,pgmptr,ioinfo=struct.unpack_from('<6I',crt)
    assert crtbase and pgmptr and ioinfo
    program=(directory/'pgmptr.bin').read_bytes()
    assert program.endswith(b'\0') and b'\0' not in program[:-1]
    fields={'argc':len(argv),'argv':argv,'environment':environment(env),
        'crt':{'fmode':fmode,'commode':commode,'mb_cur_max':mb_cur_max,
            'program_path':program[:-1].decode('latin1'),
            'osfile':[crt[24+36*i+4] for i in range(3)],
            # x87 control word is architecturally 16 bits; high CONTEXT DWORD
            # bits are retained raw and are not part of that register.
            'x87_control_word':struct.unpack_from('<I',context,28)[0]&0xffff}}
    return fields,['context.bin','globals.bin','argv.bin','environment.bin','crt-state.bin','pgmptr.bin']


def native_main(directory):
    directory=Path(directory)
    raw=json.loads((directory/'native-crt.json').read_text())
    argv_address,argv=vector(directory/'argv.bin')
    env_address,env=vector(directory/'environment.bin')
    assert raw['argc']==len(argv) and raw['argv_address']==argv_address
    assert raw['envp_address']==env_address==raw['environ_address'],'native environment vector alias differs'
    program=(directory/'pgmptr.bin').read_bytes()
    assert program.endswith(b'\0') and b'\0' not in program[:-1]
    fields={'argc':len(argv),'argv':argv,'environment':environment(env),
        'crt':{'fmode':raw['fmode'],'commode':raw['commode'],'mb_cur_max':raw['mb_cur_max'],
            'program_path':program[:-1].decode('latin1'),'osfile':raw['osfile'],
            'x87_control_word':raw['x87_control_word']}}
    return fields,['native-crt.json','argv.bin','environment.bin','pgmptr.bin','debugger.txt','gdb-capture.py']


def basic_project(platform,directory):
    directory=Path(directory)
    terminal_file=directory/('events.txt' if platform=='original' else 'debugger.txt')
    text=terminal_file.read_text() if terminal_file.exists() else ''
    completed=(not re.search(r'^trap ',text,re.M) and re.search(r'^exit .*status=0$',text,re.M)) if platform=='original' else 'NATIVE_COMPLETION_STATUS=0\n' in text
    if completed:
        inputs=[terminal_file.name]
        if platform=='native':
            from native_status import read
            posix=re.findall(r'^NATIVE_POSIX_STATUS=(\d+)$',text,re.M)
            assert len(posix)==1 and read(directory,int(posix[0]))==0
            inputs+=['terminal-status.bin','status_reader.py','native-process.json','native-build.json','native_status.py','gdb_capture.py','gdb-capture.py']
        return {'projection':('cl-case-completion-v1'if(directory/'case-inputs.json').exists()else'cl-help-completion-v1'),'fields':{'status':0},'decoder_sha256':sha(Path(__file__)),
            'raw_inputs':{name:sha(directory/name)for name in inputs}}
    if (directory/'frontier-memory.json').exists():
        from frontier_state import decode
        return decode(directory,platform)
    fields,inputs=(original_main if platform=='original' else native_main)(directory)
    return {'projection':PROJECTION,'fields':fields,
        'decoder_sha256':sha(Path(__file__)),
        'raw_inputs':{name:sha(directory/name)for name in inputs}}


def project(platform,directory):
    directory=Path(directory);result=basic_project(platform,directory)
    result['wrapper_decoder_sha256']=sha(Path(__file__))
    response_mode=((directory/'dependency-native.json').exists() if platform=='native' else
        ((directory/'capture.json').exists() and json.loads((directory/'capture.json').read_text()).get('dependency_response',False))or
        ((directory/'dependency-observer.txt').exists() and (directory/'dependency-observer.txt').read_bytes()==b'CL_DEPENDENCY_RESPONSE_V1\n'))
    if response_mode:
        import dependency_provider,dependency_response,dependency_native,dependency_wire
        if platform=='native':
            result['fields']['dependency']=dependency_provider.verify(directory)
            paths=[directory/name for name in ('dependency-response.json','dependency-config.bin','dependency-native.json',
                'dependency_provider.py','dependency_response.py','dependency_native.py','dependency_wire.py')]
            paths+=list((directory/'dependency-native').glob('*.bin'))
            if (directory/'spawn-native.json').exists():
                paths+=[directory/'spawn-native.json',directory/'spawn_reader.py']
                paths+=[directory/name for name in result['fields']['dependency']['child']['outputs']]
                paths+=[directory/name for name in ('MSVCRT.DLL','MSPDB50.DLL')]
            paths += [p for p in (directory/'dependency-response').rglob('*')if p.is_file()]
            paths += [directory/name for name in result['fields']['dependency']['outputs']if (directory/name).exists()]
            if 'deletions'in result['fields']['dependency']:
                paths += [directory/'native-unlink-gdb.json',directory/'gdb_unlink.py']
                if result['fields']['dependency'].get('completed'):
                    paths += [directory/name for name in ('native-postchild-gdb.json','native-exit-gdb.json','gdb_exit.py','completion_state.py')]
                    paths += [directory/('streams-'+phase+suffix)for phase in ('postchild','exit')for suffix in ('.json','-iob.bin')]
        else:
            response=dependency_response.decode(directory)
            result['fields']['dependency']=dependency_provider.summary(response)
            capture=json.loads((directory/'capture.json').read_text())
            paths=[directory/name for name in capture['files']]+[directory/'capture.json']
        result['raw_inputs'].update({p.relative_to(directory).as_posix():sha(p)for p in paths})
        result['dependency_decoders']={module.__name__:sha(Path(module.__file__))for module in
            (dependency_provider,dependency_response,dependency_native,dependency_wire)}
    if (directory/'dependency-inputs.json').exists():
        from case_inputs import verify_dependencies
        proof=verify_dependencies(directory);result['fields']['compiler_inputs']=proof
        result['raw_inputs'].update({name:sha(directory/name)for name in ['dependency-inputs.json']+[item['name']for item in proof['files']]})
    if (directory/'case-inputs.json').exists():
        from case_inputs import load
        case=json.loads((directory/'case-inputs.json').read_text())
        assert case==load(case['case']),'captured case differs from declared source inputs'
        for item in case['inputs']:
            assert (directory/item['name']).stat().st_size==item['bytes'] and sha(directory/item['name'])==item['sha256']
        result['fields']['case_inputs']=case
        import mktemp_state
        result['fields']['main_entry_errors']=(mktemp_state.original_entry(directory) if platform=='original' else json.loads((directory/'native-entry-errors.json').read_text()))
        names=['case-inputs.json']+[item['name']for item in case['inputs']]
        if platform=='original':
            names+=['capture.json','loaded-msvcrt.bin','main-entry-context.bin','mktemp-events.txt']
            names+=['mktemp-entry-'+kind+'.bin'for kind in ('teb','ptd','tls-index')]
        else:names+=['native-entry-errors.json','gdb_entry.py','gdb-entry.py','native-process.json','entry-profile.json']
        if platform=='native' and (directory/'native-filesystem.json').exists():
            names+=['native-filesystem.json']
        result['raw_inputs'].update({name:sha(directory/name)for name in names})
        if (directory/'streams-entry.json').exists():
            import streams_state
            phases=('entry',) if result['fields'].get('status')==0 else ('entry','frontier')
            result['fields']['streams']={phase:getattr(streams_state,platform)(directory,phase)
                for phase in phases}
            names=[]
            for phase in phases:
                prefix='streams-'+phase
                names += [prefix+'.json',prefix+'-iob.bin']
                if platform=='original':names += [prefix+'-'+kind+'.bin'for kind in ('teb','peb','parameters','lowio','pioinfo-slot')]
            if platform=='original' and 'frontier' in phases:
                names+=['mktemp-frontier-'+kind+'.bin'for kind in ('teb','ptd','tls-index')]
                names+=['context.bin']
            if platform=='native':names+=['gdb_streams.py']
            result['raw_inputs'].update({name:sha(directory/name)for name in names})
            result['stream_decoder_sha256']=sha(Path(streams_state.__file__))
        config=json.loads((directory/'config.json').read_text())
        if '_prepare_jobs' in config['accepted']:
            assert case['case']=='minimal-c','MRE invocation boundary not established for this case'
            if platform=='original':
                text=(directory/'events.txt').read_text()
                guards=re.findall(r'^mre-guard site=([0-9a-f]{8}) iat=([0-9a-f]{8})$',text,re.M)
                sites=[(0x403597,0x40a300),(0x4035c1,0x40a314),(0x4035ca,0x40a304),(0x403686,0x40a308),(0x4036b9,0x40a30c),(0x4036e2,0x40a310)]
                if '_next_jobs'in config['accepted']:
                    sites += [(0x40375c,0x40a300),(0x40378c,0x40a314),(0x4037b4,0x40a2fc),(0x403801,0x40a2f8),(0x403818,0x40a2f8),(0x403877,0x40a308),(0x4038e4,0x40a30c),(0x40390c,0x40a310)]
                assert guards==[('%08x'%site,'%08x'%slot)for site,slot in sites]
                assert 'mre-unexpected-call' not in text
                count=(directory/'mre-calls.txt').read_text();assert count=='0\n'
                from mre_imports import declarations
                assert sha(directory/'loaded-mspdb50.bin')==declarations()['provider_sha256']
                names=['mre-calls.txt','events.txt','loaded-mspdb50.bin']
            else:
                phase='entry' if result['fields'].get('status')==0 else 'frontier'
                record=json.loads((directory/('streams-'+phase+'.json')).read_text());assert record['mre_calls']==0
                names=['streams-'+phase+'.json','gdb_streams.py']
            result['fields']['external_mre_calls']=0
            result['raw_inputs'].update({name:sha(directory/name)for name in names})
    observed=(directory/'mktemp-before.bin').exists() if platform=='original' else (directory/'mktemp-expected.json').exists()
    if observed:
        import mktemp_state
        result['fields']['temporary_name_boundary']=getattr(mktemp_state,platform)(directory)
        result['projection']+='; temporary-name-boundary-v1'
        result['boundary_decoder_sha256']=sha(Path(mktemp_state.__file__))
        if platform=='original':
            names=['capture.json','loaded-msvcrt.bin','input-directory.json','main-entry-context.bin',
                'mktemp-events.txt','mktemp-before.bin','mktemp-after.bin']
            names += ['mktemp-'+phase+'-'+kind+'.bin'for phase in ('entry','before','after')for kind in ('teb','ptd','tls-index')]
            names += [r['name']for r in json.loads((directory/'input-directory.json').read_text())['entries']]
        else:
            names=['mktemp-expected.json','mktemp-receipt.bin','mktemp_reader.py','native-mktemp-gdb.json',
                'native-entry-errors.json','native-process.json','native-build.json','gdb_entry.py','gdb-entry.py','gdb_mktemp.py','entry-profile.json','debugger.txt']
        result['raw_inputs'].update({name:sha(directory/name)for name in names})
    return result
