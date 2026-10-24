"""One fresh, masked timing-case LINK response; no Windows child execution."""
import importlib.util
import json
from pathlib import Path
from case_inputs import verify_dependencies
from clock_packet import decode
from link_provider import binding,copy_raw,digest,encoded

PROJECT=Path(__file__).resolve().parents[1];ROOT=PROJECT.parents[2]

def response_packet(directory):
    packet=decode(directory,response=True)
    return {**packet['LINK'],'clock':packet['clock']}

def cached_packet():
    case=PROJECT/'cases/link-timing';reference=case/'reference'
    master=json.loads((reference/'fixture.json').read_bytes())
    raw=(case/'dependency-packet.json').read_bytes()
    assert digest(raw)==master['clock_response_sha256']
    proof=master['clock_response_provenance']
    assert proof['file']=='packet-provenance.json'
    content=(reference/proof['file']).read_bytes();assert digest(content)==proof['sha256']
    provenance=json.loads(content)
    assert provenance['schema']==1 and provenance['kind']=='explicit offline raw-derived clock response'
    assert provenance['packet_sha256']==digest(raw) and provenance['capture_sha256']==master['capture_sha256']
    assert {'clock_packet.py','link_response.py','dependency_response.py','clock_provider.py'}<=set(provenance['sources'])
    for name,record in provenance['sources'].items():
        assert Path(name).name==name and record['file']=='packet-sources/'+name
        source=(reference/record['file']).read_bytes()
        assert len(source)==record['bytes']and digest(source)==record['sha256']
        assert record['historical_source']=='decomp/vc5.10/cl/scripts/'+name
    packet=json.loads(raw);assert packet['case']=='link-timing'and packet['clock']['expected_calls']==2
    assert packet['parent']['status']==0 and packet['parent']['unlink_calls']==1
    capture=(reference/'original/capture.json').read_bytes()
    assert digest(capture)==master['capture_sha256']==packet['capture']['sha256']
    observed=json.loads(capture)
    assert observed['clock_response']is True and observed['baseline']is False and observed['controller_error']is None
    manifest=(reference/'original-files.json').read_bytes()
    assert digest(manifest)==master['owned_original']['sha256']
    inventory=json.loads(manifest)
    assert inventory['source_capture_sha256']==master['capture_sha256']
    return packet,inventory['files']

def copy_cached(original,destination):
    reference=PROJECT/'cases/link-timing/reference/original'
    assert original.resolve()==reference.resolve(),'unknown cached timing oracle'
    packet,inventory=cached_packet();destination.mkdir()
    names=['MSVCRT.DLL','MSPDB50.DLL','observer/link-output/test.exe']
    names+=['observer/link-input/'+name for name in packet['child']['inputs']]
    for name in names:
        source=original/name;record=inventory[name];raw=source.read_bytes()
        assert record['bytes']==len(raw)and record['sha256']==digest(raw)
        if source.is_symlink():
            assert record.get('repository_original')
            expected=(ROOT/record['repository_original']).resolve()
            assert not source.readlink().is_absolute()and source.resolve()==expected and expected.is_relative_to(ROOT/'toolchains/5.10')
        target=destination/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
    return packet

class Provider:
    def __init__(self,original,directory,spawn_reader,cached_fixture=False):
        self.directory=Path(directory).resolve();self.raw=self.directory/'clock-original'
        if cached_fixture:
            self.response=copy_cached(Path(original),self.raw)
        else:
            copy_raw(Path(original),self.raw,'link-timing');self.response=response_packet(self.raw)
        self.identity=digest(encoded(self.response));self.requests=[];self.effects=[]
        self.initial_files=None;self.cached_fixture=cached_fixture
        self.response_path=self.directory/'link-response.json'
        self.response_path.write_bytes(encoded(self.response))
        (self.directory/'clock-response.json').write_bytes(encoded(self.response))
        if cached_fixture:(self.directory/'cached-clock-fixture.json').write_bytes(encoded({
            'mode':'cached native regression; never fresh acceptance','response_sha256':self.identity}))
        self.binding_path=self.directory/'link-binding.bin';self.binding_path.write_bytes(binding(self.response))
        zone=Path('/usr/share/zoneinfo/UTC');assert zone.is_file()
        self.host_inputs={'TZ':'UTC','TZDIR':'/usr/share/zoneinfo','zonefile':{
            'path':str(zone),'resolved_path':str(zone.resolve()),'sha256':digest(zone.read_bytes())},
            'clock_input':self.response['clock']['clock_input'],
            'expected_calls':self.response['clock']['expected_calls'],'response_sha256':self.identity,
            'target_TZ':'absent; CRT and OS namespaces remain bound to the original'}
        (self.directory/'clock-host-inputs.json').write_bytes(encoded(self.host_inputs))
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
            'CL_CLOCK_EXPECTED_CALLS':str(clock['expected_calls']),'TZ':'UTC','TZDIR':'/usr/share/zoneinfo'}

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
            'broker_errors':list(self.broker.errors),'cached_fixture':self.cached_fixture}
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
        'FP_scope':{'architectural_fields_matched':['CW','status','tag','all eight 80-bit registers','FIP','FDP','opcode'],
            'selector_pair':{'original':[0x23,0x2b],'native':[0,0]},
            'selector_premise':'Both saved pointers zero, status zero, all tags empty and register bytes zero; selectors unconsumed and do not escape. Full within-process FP preservation remains strict.'},
        'CRT_OS_FILE_stream_ownership_unchanged':True,'unlink_calls':response['parent']['unlink_calls'],
        'output_sha256':response['child']['output']['sha256']}

def verify(directory,cached_fixture=False):
    directory=Path(directory);response=json.loads((directory/'clock-response.json').read_bytes())
    native=json.loads((directory/'clock-native.json').read_bytes())
    assert native['cached_fixture']is cached_fixture and not native['broker_errors']
    assert (directory/'cached-clock-fixture.json').exists()is cached_fixture
    if cached_fixture:assert response==cached_packet()[0]
    else:assert response==response_packet(directory/'clock-original')
    assert response==json.loads((directory/'link-response.json').read_bytes())
    identity=digest(encoded(response));assert binding(response)==(directory/'link-binding.bin').read_bytes()
    host=json.loads((directory/'clock-host-inputs.json').read_bytes())
    assert host['TZ']=='UTC'and host['TZDIR']=='/usr/share/zoneinfo'
    assert host['clock_input']==response['clock']['clock_input']and host['expected_calls']==response['clock']['expected_calls']
    assert host['response_sha256']==identity and host['target_TZ']=='absent; CRT and OS namespaces remain bound to the original'
    zone=Path('/usr/share/zoneinfo/UTC')
    assert host['zonefile']=={'path':str(zone),'resolved_path':str(zone.resolve()),'sha256':digest(zone.read_bytes())}
    if not cached_fixture:
        assert json.loads((directory/'config.json').read_bytes())==json.loads((directory/'clock-original/config.json').read_bytes())
    else:
        assert response['clock']['expected_calls']==2
    if cached_fixture:
        assert json.loads((directory/'cached-clock-fixture.json').read_bytes())=={
            'mode':'cached native regression; never fresh acceptance','response_sha256':identity}
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
    if not cached_fixture:
        from clock_state import verify as state
        state(directory,response,identity,pid)
    return projection(response)
