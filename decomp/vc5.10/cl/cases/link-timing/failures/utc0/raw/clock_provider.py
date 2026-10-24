"""One fresh, masked timing-case LINK response; no Windows child execution."""
import importlib.util
import json
from pathlib import Path
from case_inputs import verify_dependencies
from clock_packet import decode
from link_provider import binding,copy_raw,digest,encoded

def response_packet(directory):
    packet=decode(directory,response=True)
    return {**packet['LINK'],'clock':packet['clock']}

class Provider:
    def __init__(self,original,directory,spawn_reader,cached_fixture=False):
        assert not cached_fixture,'timing regression not enabled before completed-case review'
        self.directory=Path(directory).resolve();self.raw=self.directory/'clock-original'
        copy_raw(Path(original),self.raw);self.response=response_packet(self.raw)
        self.identity=digest(encoded(self.response));self.requests=[];self.effects=[]
        self.initial_files=None;self.cached_fixture=False
        self.response_path=self.directory/'link-response.json'
        self.response_path.write_bytes(encoded(self.response))
        (self.directory/'clock-response.json').write_bytes(encoded(self.response))
        self.binding_path=self.directory/'link-binding.bin';self.binding_path.write_bytes(binding(self.response))
        for name,item in self.response['child']['provider_inputs'].items():
            raw=(self.raw/name).read_bytes();assert item=={'bytes':len(raw),'sha256':digest(raw)}
            assert not(self.directory/name).exists();(self.directory/name).write_bytes(raw)
        spec=importlib.util.spec_from_file_location('clock_spawn_reader',spawn_reader)
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);self.spawn_module=module
        owner=self
        class ObservedSpawn(module.SpawnBroker):
            def _decode(self,payload,state_profile=False):
                owner.requests.append({'state_profile':state_profile,'payload_hex':payload.hex()})
                return module.SpawnBroker._decode(payload,state_profile)
        self.broker=ObservedSpawn(self._spawn)

    def environment(self):
        clock=self.response['clock']
        return {**self.broker.environment(),'CL_LINK_BINDING':str(self.binding_path),
            'CL_LINK_RESPONSE_ID':self.identity,'RECON_SPAWN_STATE_PROFILE':'msvcrt510-cp0-wait-success-v1',
            'RECON_SPAWN_REQUIRE_EXISTS':'1','RECON_LOCAL_CLOCK':clock['clock_input'],
            'CL_CLOCK_EXPECTED_CALLS':str(clock['expected_calls']),'TZ':'UTC0'}

    def descriptors(self):return(self.broker.fileno(),)
    def close(self):self.broker.close()
    def arm(self):
        assert self.initial_files is None
        self.initial_files={p.name for p in self.directory.iterdir()}

    def _spawn(self,request):
        assert not self.effects,'extra timing-case LINK invocation'
        expected=self.response['child'];directory=self.directory
        assert set(request)=={'mode','pid','file','cwd','argv','environment','environment_source','parent_state'}
        assert type(request['pid'])is int and 0<request['pid']<=0xffffffff
        assert type(request['mode'])is int and all(type(v)is int for v in request['parent_state'].values())
        assert request['mode']==0 and request['file']==expected['path']and request['argv']==expected['argv']
        assert Path(request['cwd']).resolve()==directory and request['environment']==expected['environment']
        assert request['environment_source']=='target-os'and request['parent_state']=={'errno':0,'doserrno':2,'last_error':0}
        verify_dependencies(directory)
        for name,item in {**expected['inputs'],**expected['provider_inputs']}.items():
            raw=(directory/name).read_bytes();assert item=={'bytes':len(raw),'sha256':digest(raw)}
        assert self.initial_files is not None and not(directory/'test.exe').exists()
        controls={'native-link-before-gdb.json','streams-link-before-iob.bin','streams-link-before.json',
            'native-process.json','native-entry-errors.json','native-mktemp-gdb.json','mktemp-receipt.bin',
            'terminal-status.bin','stdout.bin','stderr.bin','trace.txt','streams-entry-iob.bin',
            'streams-entry.json','native-clock-gdb.json'}
        names={p.name for p in directory.iterdir()}
        assert names==self.initial_files|{expected['argv'][2][3:]}|(names&controls),'unknown prechild artifact'
        raw=(self.raw/'observer/link-output/test.exe').read_bytes()
        assert expected['output']=={'name':'test.exe','bytes':len(raw),'sha256':digest(raw)}
        with(directory/'test.exe').open('xb')as output:output.write(raw)
        self.effects.append({'request':request,'output':expected['output']})
        return {'status':0,'stdout':b'','stderr':b'','parent_state':{
            'before':{'errno':0,'doserrno':2,'last_error':0},'after':{'errno':0,'doserrno':0,'last_error':0}}}

    def finish(self,pid):
        receipt={'pid':pid,'response_sha256':self.identity,'binding_sha256':digest(self.binding_path.read_bytes()),
            'requests':self.broker.requests,'wire':self.requests,'effects':self.effects,
            'broker_errors':list(self.broker.errors),'cached_fixture':False}
        (self.directory/'clock-native.json').write_bytes(encoded(receipt))
        self.broker.check();assert not self.broker.errors,self.broker.errors
        assert len(self.broker.requests)==len(self.effects)==len(self.requests)==1
        assert self.broker.requests[0]['pid']==pid==self.effects[0]['request']['pid']
        assert self.requests[0]['state_profile']is True
        assert self.broker.requests[0]['parent_state_after']=={'errno':0,'doserrno':0,'last_error':0}
        child=self.response['child'];completed=self.response['clock']['expected_calls']==2
        output=(self.directory/'test.exe').read_bytes()
        assert child['output']=={'name':'test.exe','bytes':len(output),'sha256':digest(output)}
        response=self.directory/child['argv'][2][3:]
        assert response.exists()==(not completed),'response-file cleanup differs'
        if not completed:
            raw=response.read_bytes();assert child['inputs'][response.name]=={'bytes':len(raw),'sha256':digest(raw)}
        verify_dependencies(self.directory)
        for name,item in {**child['provider_inputs'],'test.obj':child['inputs']['test.obj']}.items():
            raw=(self.directory/name).read_bytes();assert item=={'bytes':len(raw),'sha256':digest(raw)}
        controls={'clock-native.json','native-process.json','native-entry-errors.json','native-mktemp-gdb.json',
            'mktemp-receipt.bin','terminal-status.bin','stdout.bin','stderr.bin','trace.txt','streams-entry-iob.bin',
            'streams-entry.json','debugger.txt','native-link-unlink-gdb.json','native-clock-gdb.json',
            'frontier-memory.json','streams-frontier-iob.bin','streams-frontier.json'}
        for phase in ('link-before','link-after','link-postchild','link-exit'):
            controls|={'native-'+phase+'-gdb.json','streams-'+phase+'-iob.bin','streams-'+phase+'.json'}
        names={p.name for p in self.directory.iterdir()}
        assert names==self.initial_files|{'test.exe'}|({response.name}if not completed else set())|(names&controls)
        return receipt

def projection(response):
    clock=response['clock']
    return {'case':'link-timing','child':response['child'],
        'parent_errors':{name:phase['errors']for name,phase in response['parent']['phases'].items()},
        'clock_input':clock['clock_input'],'clock_calls':[{
            'input':record['before']['timeb'],'output':record['after']['timeb'],
            'before':record['before']['errors'],'after':record['after']['errors'],
            'CW':record['after']['x87_control_word'],'live_FP_preserved':True}for record in clock['calls']],
        'exact_clock_calls':clock['expected_calls'],'timezone_names_matched':False,
        'CRT_OS_FILE_stream_ownership_unchanged':True,'unlink_calls':response['parent']['unlink_calls'],
        'output_sha256':response['child']['output']['sha256']}

def verify(directory):
    directory=Path(directory);response=json.loads((directory/'clock-response.json').read_bytes())
    assert response==response_packet(directory/'clock-original')
    assert response==json.loads((directory/'link-response.json').read_bytes())
    identity=digest(encoded(response));assert binding(response)==(directory/'link-binding.bin').read_bytes()
    assert json.loads((directory/'config.json').read_bytes())==json.loads((directory/'clock-original/config.json').read_bytes())
    native=json.loads((directory/'clock-native.json').read_bytes())
    assert native['cached_fixture']is False and not native['broker_errors']
    assert native['response_sha256']==identity and native['binding_sha256']==digest(binding(response))
    pid=json.loads((directory/'native-process.json').read_bytes())['pid'];assert native['pid']==pid
    assert len(native['requests'])==len(native['effects'])==len(native['wire'])==1
    assert native['wire'][0]['state_profile']is True
    spec=importlib.util.spec_from_file_location('clock_verify_spawn',directory/'spawn_reader.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    decoded=module.SpawnBroker._decode(bytes.fromhex(native['wire'][0]['payload_hex']),True)
    expected=response['child'];request=native['requests'][0]
    assert decoded['pid']==pid
    assert {key:value for key,value in decoded.items()if key!='environment'}=={key:request[key]for key in decoded if key!='environment'}
    assert set(decoded)=={'mode','pid','file','cwd','argv','environment','parent_state','environment_source'}
    assert decoded['mode']==0 and decoded['file']==expected['path']and decoded['argv']==expected['argv']
    assert Path(decoded['cwd']).resolve()==directory and decoded['environment']==expected['environment']
    assert decoded['parent_state']=={'errno':0,'doserrno':2,'last_error':0}and decoded['environment_source']=='target-os'
    assert request['parent_state_after']=={'errno':0,'doserrno':0,'last_error':0}
    assert native['effects'][0]['request']==decoded and native['effects'][0]['output']==expected['output']
    assert request['action']=='replay'and request['status']==0
    verify_dependencies(directory)
    for name,item in expected['provider_inputs'].items():
        raw=(directory/name).read_bytes();assert item=={'bytes':len(raw),'sha256':digest(raw)}
    completed=response['clock']['expected_calls']==2
    for name,item in expected['inputs'].items():
        raw=(directory/'clock-original/observer/link-input'/name).read_bytes()
        assert item=={'bytes':len(raw),'sha256':digest(raw)}
        current=directory/name
        if name=='test.obj'or not completed:assert current.read_bytes()==raw
        else:assert not current.exists()
    raw=(directory/'test.exe').read_bytes()
    assert expected['output']=={'name':'test.exe','bytes':len(raw),'sha256':digest(raw)}
    from clock_state import verify as state
    state(directory,response,identity,pid)
    return projection(response)
