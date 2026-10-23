"""Decode actual masked LINK-only inputs/effects; no native success is supplied."""
import hashlib
import json
from pathlib import Path
import re
import struct
from binary import PE,u32
from case_inputs import load as load_case,verify_dependencies
from dependency_packet import vector,directory_entries
from dependency_response import phase_snapshot
from mask import construct
from mktemp_state import original_states,CRT_SHA

PHASES=('spawn-before','spawn-after','exit-before')
digest=lambda data:hashlib.sha256(data).hexdigest()


def decode(directory):
    directory=Path(directory);capture=json.loads((directory/'capture.json').read_bytes())
    assert capture.get('link_response') is True and capture['baseline'] is False
    assert not any(capture.get(name)for name in ('dependency_analysis','dependency_complete','dependency_response'))
    assert capture['case']=='link-object' and capture['controller_error'] is None and capture['helper_status']==0
    assert capture['command'][-2]=='1','LINK observation was not a masked trial'
    def read(name):
        assert name in capture['files'],'unbound LINK evidence: '+name
        path=directory/name;assert path.is_file() and not path.is_symlink()
        raw=path.read_bytes();assert capture['files'][name]=={'bytes':len(raw),'sha256':digest(raw)},'changed LINK evidence: '+name
        return raw
    for name in capture['files']:read(name)
    assert read('dependency-observer.txt')==b'CL_LINK_RESPONSE_V1\n'
    config=json.loads(read('config.json'));assert digest(read('config.json'))==capture['config_sha256']
    assert digest(construct(config))==capture['original_mask_sha256']==digest(read('CL.EXE'))==digest(read('loaded-target.bin'))
    assert capture['target_sha256']==config['target']['sha256']=='aa15ed605515693292c539827ab1d8e08ce2ed5b4e1d9ac2c7f033903e6844dd'
    assert digest(read('loaded-msvcrt.bin'))==capture['loaded_msvcrt_sha256']==CRT_SHA
    assert digest(read('reference-debugger.exe'))==capture['helper_sha256']==capture['helper_build']['binary']['sha256']
    for name,sha in capture['helper_sources'].items():assert digest(read(name))==sha==capture['helper_build']['sources']['scripts/'+name]
    helper=read('dependency-observer.inc')
    assert b'if (dependency_link_response && i!=5 && i!=7 && i!=8) continue;'in helper
    assert b'if (strcmp(executable,argument)) ExitProcess(107);'in helper
    assert b'save_file(image,"observer\\\\spawn-executable.bin")'in helper
    assert b'dependency_link_paths(response,sizeof(response),destination,sizeof(destination))'in helper
    case=load_case('link-object');assert all(capture[k]==case[k]for k in ('case','arguments','inputs'))
    for item in case['inputs']:assert capture['files'][item['name']]=={k:item[k]for k in ('bytes','sha256')}
    dependency=verify_dependencies(directory);assert len(dependency['files'])==1 and dependency['files'][0]['name']=='LINK.EXE'
    linker_sha=dependency['files'][0]['sha256'];assert digest(read('LINK.EXE'))==digest(read('observer/spawn-executable.bin'))==linker_sha
    events=read('observer/events.txt').decode('ascii');target_events=read('events.txt').decode('ascii')
    assert re.findall(r'^snapshot phase=(\S+)',events,re.M)==list(PHASES),'unexpected full-snapshot inventory'
    assert re.findall(r'^exit .*status=(\d+)$',target_events,re.M)==['0']
    assert not re.search(r'^trap |^exception ',target_events,re.M)
    assert events.count('observed-child family=link result=00000000 putenv_calls=0\n')==1
    assert 'analysis-c2-completed'not in events and 'invoke-call'not in events and 'c1-load'not in events
    errors=original_states(directory,PHASES,'observer/events.txt');phases={};contexts={}
    for phase in PHASES:phases[phase],contexts[phase]=phase_snapshot(directory,read,phase,errors[phase])
    before,after,terminal=[phases[name]for name in PHASES]
    assert before['event']<after['event']<terminal['event']
    assert errors=={'spawn-before':{'errno':0,'doserrno':2,'last_error':0},'spawn-after':{'errno':0,'doserrno':0,'last_error':0},'exit-before':{'errno':0,'doserrno':0,'last_error':0}}
    for phase,item in phases.items():
        assert (item['fmode'],item['commode'],item['mb_cur_max'],item['mb_codepage'],item['lc_codepage'],item['lc_handles'],item['x87_control_word'])==(0,0,1,0,0,[0]*6,639)
        assert item['pass_address']==0x4090a8+4*36 and item['pass_words'][2:]==[0,0x2f43,11,0,0,0,0]
        assert item['pass_environment']is None
        execution=struct.unpack('<5I',read('observer/'+phase+'-execution-words.bin'))
        assert execution==(0,item['pass_address'],0 if phase=='exit-before'else 1,0x4095d8,item['pass_address'])
        for field in ('cwd','image','command','pctype_address','mb_state_sha256','ctype_sha256','crt_environment_count','crt_environment_sha256','os_environment_sha256','file0_2_words','streams','open_descriptors'):
            assert item[field]==before[field],'unexpected LINK parent effect: '+field
        assert read('observer/'+phase+'-parameters.bin')==read('observer/spawn-before-parameters.bin')
        assert read('observer/'+phase+'-static-iob.bin')==read('observer/spawn-before-static-iob.bin')
    mode,path,argv=struct.unpack_from('<3I',read('observer/spawn-before-stack.bin'))
    assert mode==0 and path==before['pass_words'][0] and argv==before['argument_vector']
    assert u32(contexts['spawn-before'],196)==u32(contexts['spawn-after'],196) and u32(contexts['spawn-after'],176)==0
    assert after['argv']==before['argv'] and after['argument_pointers']==before['argument_pointers'] and after['argument_vector']==before['argument_vector']
    assert len(before['argv'])==3 and before['argv'][0]==before['cwd'].rstrip('\\')+'\\link.exe' and before['argv'][1]=='-link'
    argument=before['argv'][2];assert re.fullmatch(r'@\.\\[A-Za-z0-9]{6}lk',argument)
    response_name=argument[3:];assert before['argv'][2]=='@.\\'+response_name
    object_bytes=read('test.obj');response=read('observer/link-input/'+response_name)
    assert read('observer/link-input/test.obj')==object_bytes
    assert response==b'/nologo\r\n/out:test.exe\r\n/ENTRY:main\r\n/SUBSYSTEM:CONSOLE\r\n/NODEFAULTLIB\r\ntest.obj\r\n'
    assert read('observer/link-output/test.exe')==read('test.exe') and len(read('test.exe'))==1024
    assert response_name in before['directory'] and 'test.exe'not in before['directory']
    assert after['directory']['test.exe']['bytes']==1024 and after['directory'][response_name]==before['directory'][response_name]
    assert response_name not in terminal['directory'] and terminal['directory']['test.exe']==after['directory']['test.exe']
    # The debugger writes these exact bounded transport records in the same
    # working directory. Retain their full raw metadata, but do not replay them
    # as target outputs. Every other directory entry participates in the delta.
    observer_name=re.compile(r'(?:mktemp-(?:spawn-after|exit-before|unlink-1-(?:before|after))-(?:ptd|teb|tls-index)\.bin|streams-(?:spawn-after|exit-before)-(?:iob|lowio|parameters|peb|pioinfo-slot|teb)\.bin|streams-(?:spawn-after|exit-before)\.json|events\.txt|trace\.txt)')
    def target_directory(item):
        result={}
        for name,metadata in item['directory'].items():
            if observer_name.fullmatch(name):read(name)
            else:result[name]=metadata
        return result
    before_files,after_files,terminal_files=[target_directory(item)for item in (before,after,terminal)]
    assert {name:item for name,item in before_files.items()if name!=response_name}=={name:item for name,item in after_files.items()if name not in (response_name,'test.exe')}
    assert {name:item for name,item in after_files.items()if name!=response_name}==terminal_files
    assert read('stdout.bin')==read('stderr.bin')==b''
    unlink=re.findall(r'^unlink-call sequence=(\d+) function=([0-9a-f]{8}) argument=([0-9a-f]{8}) esp=([0-9a-f]{8})$',events,re.M)
    assert len(unlink)==1 and unlink[0][0]=='1'
    provider=PE(directory/'loaded-msvcrt.bin')
    from dependency_packet import exports
    assert int(unlink[0][1],16)==provider.base+exports(provider)['_unlink']['rva']
    assert read('observer/unlink-1-before-filename.bin')==('.\\'+response_name+'\0').encode('ascii')
    assert re.findall(r'^unlink-return sequence=1 result=([0-9a-f]{8}) esp=([0-9a-f]{8})$',events,re.M)==[('00000000',unlink[0][3])]
    unlink_errors=original_states(directory,['unlink-1-before','unlink-1-after'],'observer/events.txt')
    assert all(state==errors['spawn-after']for state in unlink_errors.values())
    assert re.findall(r'^exit-call status=(\d+) function=([0-9a-f]{8}) esp=([0-9a-f]{8})$',events,re.M)==[('0','%08x'%(provider.base+exports(provider)['exit']['rva']),'%08x'%u32(contexts['exit-before'],196))]
    assert u32(read('observer/exit-before-stack.bin'),0)==0
    env_address,env=vector(read('observer/spawn-before-crt-environment.bin'))
    os_block=read('observer/spawn-before-os-environment.bin');assert os_block.endswith(b'\0\0\0\0')
    os_strings=os_block.decode('utf-16le')[:-2].split('\0');crt_strings=[text.decode('ascii')for pointer,text in env]
    assert crt_strings==os_strings,'this case requires exact matching CRT/OS contents and order'
    trace=read('trace.txt').decode('ascii').splitlines();assert len(trace)==len(set(trace)) and 'write_link_message'in trace
    return {'kind':'fresh masked LINK-only external response; acceptance still held','case':'link-object',
        'capture':{'path':str(directory/'capture.json'),'sha256':digest((directory/'capture.json').read_bytes())},
        'config_sha256':capture['config_sha256'],'mask_sha256':capture['original_mask_sha256'],
        'child':{'mode':mode,'path':before['argv'][0],'argv':before['argv'],'cwd':before['cwd'],'environment_source':'target-os','environment':os_strings,'linker_sha256':linker_sha,
            'inputs':{name:capture['files']['observer/link-input/'+name]for name in ('test.obj',response_name)},'status':0,'stdout_hex':'','stderr_hex':'','output':{'name':'test.exe',**capture['files']['test.exe']}},
        'parent':{'phases':phases,'CRT_environment':crt_strings,'OS_environment_hex':os_block.hex(),'unlink_calls':1,'unlink_errors':unlink_errors,'status':0,'trace':trace}}


if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);parser.add_argument('--output',type=Path)
    args=parser.parse_args();result=decode(args.directory);payload=json.dumps(result,indent=2)+'\n'
    if args.output:args.output.write_text(payload)
    else:print(payload,end='')
