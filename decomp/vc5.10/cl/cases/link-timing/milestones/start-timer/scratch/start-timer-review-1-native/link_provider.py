"""One exact native LINK response; unknown children never execute or succeed."""
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import struct
from case_inputs import verify_dependencies
from link_response import decode as original_response

PROJECT=Path(__file__).resolve().parents[1];ROOT=PROJECT.parents[2]
digest=lambda data:hashlib.sha256(data).hexdigest()
encoded=lambda value:(json.dumps(value,sort_keys=True,indent=2)+'\n').encode()


def copy_raw(original,destination):
    capture=json.loads((original/'capture.json').read_bytes());destination.mkdir()
    for name in ['capture.json',*capture['files']]:
        relative=Path(name);assert not relative.is_absolute() and '..'not in relative.parts
        source=original/relative;target=destination/relative;target.parent.mkdir(parents=True,exist_ok=True)
        if source.is_symlink():
            proof=json.loads((PROJECT/'cases/link-object/reference/original-files.json').read_bytes())['files'][name]
            assert proof.get('repository_original')
            expected=(ROOT/proof['repository_original']).resolve()
            assert not Path(source.readlink()).is_absolute() and expected.is_relative_to(ROOT/'toolchains/5.10') and source.resolve()==expected
        raw=source.read_bytes()
        if name!='capture.json':assert capture['files'][name]=={'bytes':len(raw),'sha256':digest(raw)}
        target.write_bytes(raw)


def binding(response):
    env=response['parent']['CRT_environment'];os_block=bytes.fromhex(response['parent']['OS_environment_hex'])
    payload=b'RCL1'+struct.pack('<I',1)+digest(encoded(response)).encode('ascii')+struct.pack('<I',len(env))
    for text in env:
        raw=text.encode('ascii');assert raw and b'\0'not in raw and len(raw)<=32767
        payload+=struct.pack('<I',len(raw))+raw
    payload+=struct.pack('<I',len(os_block)//2)+os_block
    assert 76<=len(payload)<=262144
    return payload


class Provider:
    def __init__(self,original,directory,spawn_reader,cached_fixture=False):
        self.directory=Path(directory).resolve();self.raw=self.directory/'link-original'
        copy_raw(Path(original),self.raw);self.response=original_response(self.raw)
        expected=json.loads((PROJECT/'cases/link-object/dependency-packet-v2.json').read_bytes())
        assert self.response==expected,'LINK response differs from independently reviewed owned packet'
        for name,item in self.response['child']['provider_inputs'].items():
            value=(self.raw/name).read_bytes();assert item=={'bytes':len(value),'sha256':digest(value)}
            assert not(self.directory/name).exists();(self.directory/name).write_bytes(value)
        self.identity=digest(encoded(self.response));self.response_path=self.directory/'link-response.json'
        self.response_path.write_bytes(encoded(self.response));self.binding_path=self.directory/'link-binding.bin'
        self.binding_path.write_bytes(binding(self.response));self.cached_fixture=cached_fixture
        if cached_fixture:(self.directory/'cached-link-fixture.json').write_bytes(encoded({'mode':'cached native regression; no fresh acceptance','response_sha256':self.identity}))
        spec=importlib.util.spec_from_file_location('cl_link_spawn_reader',spawn_reader);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        self.spawn_module=module;self.requests=[];self.effects=[];owner=self
        class ObservedSpawn(module.SpawnBroker):
            def _decode(self,payload,state_profile=False):
                owner.requests.append({'state_profile':state_profile,'payload_hex':payload.hex()})
                return module.SpawnBroker._decode(payload,state_profile)
        self.broker=ObservedSpawn(self._spawn)
        self.manifest=self.directory/'link-native.json';self.initial_files=None

    def environment(self):
        return {**self.broker.environment(),'CL_LINK_BINDING':str(self.binding_path),'CL_LINK_RESPONSE_ID':self.identity,
            'RECON_SPAWN_STATE_PROFILE':'msvcrt510-cp0-wait-success-v1','RECON_SPAWN_REQUIRE_EXISTS':'1'}

    def descriptors(self):return(self.broker.fileno(),)

    def _spawn(self,request):
        assert not self.effects,'extra LINK invocation'
        expected=self.response['child'];directory=self.directory
        assert set(request)=={'mode','pid','file','cwd','argv','environment','environment_source','parent_state'},'unknown LINK request fields'
        assert type(request['pid'])is int and 0<request['pid']<=0xffffffff
        assert type(request['mode'])is int and all(type(value)is int for value in request['parent_state'].values())
        assert request['mode']==0 and request['file']==expected['path'] and request['argv']==expected['argv']
        assert Path(request['cwd']).resolve()==directory and request['environment']==expected['environment']
        assert request['environment_source']=='target-os' and request['parent_state']=={'errno':0,'doserrno':2,'last_error':0}
        verify_dependencies(directory)
        for name,item in expected['provider_inputs'].items():
            value=(directory/name).read_bytes();assert item=={'bytes':len(value),'sha256':digest(value)},'native LINK provider input differs: '+name
        for name,item in expected['inputs'].items():
            data=(directory/name).read_bytes();assert item=={'bytes':len(data),'sha256':digest(data)},'native LINK input differs: '+name
        assert not(directory/'test.exe').exists(),'unexpected preexisting LINK output'
        assert self.initial_files is not None,'LINK provider was not armed'
        names={path.name for path in directory.iterdir()}
        observer_names={'native-link-before-gdb.json','streams-link-before-iob.bin','streams-link-before.json',
            'native-process.json','native-entry-errors.json','native-mktemp-gdb.json','mktemp-receipt.bin','terminal-status.bin',
            'stdout.bin','stderr.bin','trace.txt','streams-entry-iob.bin','streams-entry.json'}
        assert names==self.initial_files|{expected['argv'][2][3:]}|(names&observer_names),'unknown prechild artifact'
        output=(self.raw/'observer/link-output/test.exe').read_bytes()
        assert expected['output']=={'name':'test.exe','bytes':len(output),'sha256':digest(output)}
        with (directory/'test.exe').open('xb')as file:file.write(output)
        self.effects.append({'request':request,'output':expected['output']})
        return {'status':0,'stdout':b'','stderr':b'','parent_state':{'before':{'errno':0,'doserrno':2,'last_error':0},'after':{'errno':0,'doserrno':0,'last_error':0}}}

    def arm(self):
        assert self.initial_files is None
        self.initial_files={path.name for path in self.directory.iterdir()}

    def finish(self,pid):
        self.manifest.write_bytes(encoded({'schema':1,'pid':pid,'response_sha256':self.identity,
            'binding_sha256':digest(self.binding_path.read_bytes()),'requests':self.broker.requests,
            'wire':self.requests,'effects':self.effects,'broker_errors':list(self.broker.errors),'cached_fixture':self.cached_fixture}))
        self.broker.check();assert not self.broker.errors,self.broker.errors
        assert len(self.broker.requests)==len(self.effects)==len(self.requests)==1,'missing or extra LINK request'
        assert self.broker.requests[0]['pid']==pid and self.effects[0]['request']['pid']==pid
        assert self.requests[0]['state_profile'] is True
        assert self.broker.requests[0]['parent_state_after']=={'errno':0,'doserrno':0,'last_error':0}
        child=self.response['child'];output=(self.directory/'test.exe').read_bytes()
        assert child['output']=={'name':'test.exe','bytes':len(output),'sha256':digest(output)}
        assert not(self.directory/child['argv'][2][3:]).exists(),'response file was not removed'
        verify_dependencies(self.directory)
        for name,item in child['provider_inputs'].items():
            value=(self.directory/name).read_bytes();assert item=={'bytes':len(value),'sha256':digest(value)}
        assert digest((self.directory/'test.obj').read_bytes())==child['inputs']['test.obj']['sha256']
        control_names={'link-native.json','native-process.json','native-entry-errors.json','native-mktemp-gdb.json',
            'mktemp-receipt.bin','terminal-status.bin','stdout.bin','stderr.bin','trace.txt','streams-entry-iob.bin',
            'streams-entry.json','debugger.txt','native-link-unlink-gdb.json'}
        for phase in ('link-before','link-after','link-postchild','link-exit'):
            control_names|={'native-'+phase+'-gdb.json','streams-'+phase+'-iob.bin','streams-'+phase+'.json'}
        names={path.name for path in self.directory.iterdir()}
        assert names==self.initial_files|{'test.exe'}|(names&control_names),'unknown final LINK artifact'
        result={'schema':1,'pid':pid,'response_sha256':self.identity,'binding_sha256':digest(self.binding_path.read_bytes()),
            'requests':self.broker.requests,'wire':self.requests,'effects':self.effects,'broker_errors':list(self.broker.errors),'cached_fixture':self.cached_fixture}
        self.manifest.write_bytes(encoded(result));return result

    def close(self):self.broker.close()


def projection(response):
    return {'case':'link-object','child':response['child'],'parent_errors':{name:response['parent']['phases'][name]['errors']for name in ('spawn-before','spawn-after','exit-before')},
        'CRT_and_OS_environment_unchanged':True,'CP0_and_stream_ownership_unchanged':True,'unlink_calls':1,'output_sha256':response['child']['output']['sha256']}


def verify(directory,cached_fixture=False):
    directory=Path(directory);response=json.loads((directory/'link-response.json').read_bytes());identity=digest(encoded(response))
    assert response==original_response(directory/'link-original')
    assert response==json.loads((PROJECT/'cases/link-object/dependency-packet-v2.json').read_bytes()),'native response is not the reviewed owned contract'
    if not cached_fixture:
        assert json.loads((directory/'config.json').read_bytes())==json.loads((directory/'link-original/config.json').read_bytes()),'native response belongs to a different masked trial'
    assert binding(response)==(directory/'link-binding.bin').read_bytes()
    native=json.loads((directory/'link-native.json').read_bytes())
    assert native['response_sha256']==identity and native['binding_sha256']==digest(binding(response))
    assert native['broker_errors']==[]
    assert native['cached_fixture']is cached_fixture
    assert cached_fixture==bool((directory/'cached-link-fixture.json').exists()),'cached fixture cannot serve fresh acceptance'
    pid=json.loads((directory/'native-process.json').read_bytes())['pid'];assert native['pid']==pid
    assert len(native['requests'])==len(native['effects'])==len(native['wire'])==1
    assert native['wire'][0]['state_profile']is True
    spec=importlib.util.spec_from_file_location('link_verify_spawn',directory/'spawn_reader.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    decoded=module.SpawnBroker._decode(bytes.fromhex(native['wire'][0]['payload_hex']),True)
    expected=response['child'];assert decoded['pid']==pid
    request=native['requests'][0]
    assert {key:value for key,value in decoded.items()if key!='environment'}=={key:request[key]for key in decoded if key!='environment'}
    assert set(decoded)=={'mode','pid','file','cwd','argv','environment','parent_state','environment_source'}
    assert decoded['mode']==expected['mode'] and decoded['file']==expected['path'] and decoded['argv']==expected['argv']
    assert Path(decoded['cwd']).resolve()==directory and decoded['environment']==expected['environment']
    assert decoded['parent_state']=={'errno':0,'doserrno':2,'last_error':0} and decoded['environment_source']=='target-os'
    assert native['requests'][0]['parent_state_after']=={'errno':0,'doserrno':0,'last_error':0}
    assert native['effects'][0]['request']==decoded and native['effects'][0]['output']==expected['output']
    assert request['action']=='replay' and request['status']==0
    verify_dependencies(directory)
    for name,item in expected['provider_inputs'].items():
        value=(directory/name).read_bytes();assert item=={'bytes':len(value),'sha256':digest(value)}
    for name,item in expected['inputs'].items():
        if name=='test.obj':value=(directory/name).read_bytes()
        else:value=(directory/'link-original/observer/link-input'/name).read_bytes();assert not(directory/name).exists()
        assert item=={'bytes':len(value),'sha256':digest(value)}
    value=(directory/'test.exe').read_bytes();assert expected['output']=={'name':'test.exe','bytes':len(value),'sha256':digest(value)}
    if not cached_fixture:
        from link_state import verify as verify_state
        verify_state(directory,response,identity,pid)
    return projection(response)
