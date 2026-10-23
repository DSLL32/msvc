"""Owned native C1 response adapter. No Wine, PE execution or capture fallback."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import subprocess

from case_inputs import verify_dependencies
from dependency_native import decode as native_decode, validate
from dependency_response import decode as original_response
from dependency_wire import DllBroker, configuration

digest = lambda value: hashlib.sha256(value).hexdigest()
encoded = lambda value: (json.dumps(value, sort_keys=True, indent=2) + '\n').encode()


def copy_raw(original, destination):
    """Keep original receipts byte-for-byte; their old DOS paths are provenance."""
    original, destination = Path(original), Path(destination)
    capture = json.loads((original / 'capture.json').read_text())
    destination.mkdir(parents=True)
    for name in ['capture.json', *capture['files']]:
        relative = Path(name)
        assert not relative.is_absolute() and '..' not in relative.parts
        source = original / relative
        assert source.is_file() and not source.is_symlink()
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)


def config_bytes(response, identity):
    return configuration(identity, response['invoke_argv'][0], response['initial_environment'],
        bytes.fromhex(response['initial_os_environment_hex']), bytes.fromhex(response['post_mbctype_hex']),
        bytes.fromhex(response['post_ctype_hex']))


class Provider:
    def __init__(self, original, directory, spawn_reader=None, cached_fixture=False):
        self.directory = Path(directory).resolve()
        self.cached_fixture=cached_fixture
        self.raw = self.directory / 'dependency-response' / 'original'
        copy_raw(original, self.raw)
        self.response = original_response(self.raw)
        if cached_fixture:
            assert self.response.get("completed"),"cached fixture requires complete owned response"
            (self.directory/"cached-fixture.json").write_bytes(encoded({"mode":"cached-native-regression","capture_sha256":self.response["capture_sha256"]}))
        if 'child'in self.response:
            # These were explicit original child-directory inputs. They remain
            # inert PE bytes; neither the adapter nor the native shim runs them.
            for name,expected in self.response['child']['provider_inputs'].items():
                value=(self.raw/name).read_bytes()
                assert expected=={'bytes':len(value),'sha256':digest(value)}
                target=self.directory/name;assert not target.exists()
                target.write_bytes(value)
        self.response_bytes = encoded(self.response)
        self.identity = digest(self.response_bytes)
        self.contract = self.directory / 'dependency-response.json'
        self.contract.write_bytes(self.response_bytes)
        self.config = config_bytes(self.response, self.identity)
        self.config_path = self.directory / 'dependency-config.bin'
        self.config_path.write_bytes(self.config)
        self.previous = None
        self.broker = DllBroker(self.config, self.identity, self.response['sequence'], self._callback)
        self.spawn_broker=None
        self.spawn_payloads=[]
        self.spawn_observations=[]
        if 'child'in self.response:
            assert spawn_reader is not None,'pinned shared spawn reader required'
            spec=importlib.util.spec_from_file_location('cl_captured_spawn',spawn_reader)
            self.spawn_module=importlib.util.module_from_spec(spec);spec.loader.exec_module(self.spawn_module)
            owner=self
            class ObservedSpawn(self.spawn_module.SpawnBroker):
                def _decode(self,payload,state_profile=False):
                    owner.spawn_payloads.append((state_profile,payload))
                    return owner.spawn_module.SpawnBroker._decode(payload,state_profile)
            self.spawn_broker=ObservedSpawn(self._spawn)

    def environment(self):
        result={**self.broker.environment(), 'CL_DEPENDENCY_CONFIG': str(self.config_path)}
        if self.spawn_broker is not None:
            result.update(self.spawn_broker.environment(),RECON_SPAWN_STATE_PROFILE='msvcrt510-wait-success-v1',RECON_SPAWN_REQUIRE_EXISTS='1')
        return result

    def fileno(self):
        return self.broker.fileno()

    def descriptors(self):
        return (self.fileno(),)if self.spawn_broker is None else(self.fileno(),self.spawn_broker.fileno())

    def _inputs(self,after_invoke=False):
        verify_dependencies(self.directory)
        capture = json.loads((self.raw / 'capture.json').read_text())
        for item in capture['inputs']:
            data = (self.directory / item['name']).read_bytes()
            assert len(data) == item['bytes'] and digest(data) == item['sha256'], 'native source input changed'
        for name in self.response['outputs']:
            assert Path(name).name == name
            path=self.directory/name
            if after_invoke:
                data=path.read_bytes()
                assert self.response['outputs'][name]=={'bytes':len(data),'sha256':digest(data)},'C1 IR input changed'
            else:assert not path.exists(),'unknown existing C1 output'
        for name in ('test.obj', 'vc50.pdb', 'vc50.idb'):
            assert not (self.directory / name).exists(), 'unobserved C1 case input: ' + name

    def _callback(self, kind, data):
        fields = native_decode(data)
        validate(fields, self.response['contracts'][kind], self.directory, self.directory / 'stdout.bin',
                 self.response['lowio_flags'], previous=self.previous)
        self._inputs(after_invoke=kind==3)
        if kind == 2:
            for name, expected in self.response['outputs'].items():
                value = (self.raw / 'observer' / 'c1-output' / name).read_bytes()
                assert expected == {'bytes': len(value), 'sha256': digest(value)}, 'owned C1 response artifact changed'
                with (self.directory / name).open('xb') as output: output.write(value)
            result = {'status': 0, 'effect': 1, 'errno': 0, 'doserrno': 2, 'last_error': 0}
        else:
            result = {'status': 0, 'effect': 0, 'errno': 0, 'doserrno': 0, 'last_error': 0}
        self.previous = fields
        return result

    def _spawn(self,request):
        assert not self.spawn_observations,'extra backend process invocation'
        child=self.response['child']
        assert request=={'mode':0,'pid':self.broker.pid,'file':child['executable'],
            'cwd':str(self.directory),'argv':child['argv'],'environment':child['environment'],
            'parent_state':child['before'],'environment_source':'target-os'},'unknown actual C2 invocation or complete input namespace'
        self._inputs(after_invoke=True)
        executable=(self.directory/'C2.EXE').read_bytes()
        assert child['executable_input']=={'bytes':len(executable),'sha256':digest(executable)}
        for name,expected in child['provider_inputs'].items():
            value=(self.directory/name).read_bytes()
            assert expected=={'bytes':len(value),'sha256':digest(value)},'backend provider input differs'
        for name,expected in child['outputs'].items():
            assert Path(name).name==name
            value=(self.raw/'observer/c2-output'/name).read_bytes()
            assert expected=={'bytes':len(value),'sha256':digest(value)}
            with (self.directory/name).open('xb')as output:output.write(value)
        self.spawn_observations.append(request)
        return {'status':0,'stdout':bytes.fromhex(child['stdout_hex']),'stderr':bytes.fromhex(child['stderr_hex']),
                'parent_state':{'before':child['before'],'after':child['after']}}

    def finish(self, pid):
        path = self.directory / 'dependency-native'
        path.mkdir()
        for index, (kind, data) in enumerate(self.broker.payloads):
            (path / ('request-%d-%d.bin' % (index, kind))).write_bytes(data)
        attempts=[]
        for index,(kind,owner,data)in enumerate(self.broker.attempts):
            (path / ('attempt-%d-%d.bin'%(index,kind))).write_bytes(data)
            attempts.append({'kind':kind,'pid':owner,'bytes':len(data),'sha256':digest(data)})
        receipt = {'kind': 'native C1 response inventory', 'pid': pid, 'response_sha256': self.identity,
                   'configuration_sha256': digest(self.config), 'requests': self.broker.requests,
                   'errors': list(self.broker.errors),
                   'attempts':attempts,
                   'controller_sha256': digest(Path(__file__).read_bytes())}
        (self.directory / 'dependency-native.json').write_bytes(encoded(receipt))
        self.broker.check(pid)
        if self.spawn_broker is not None:
            for index,(profile,data)in enumerate(self.spawn_payloads):
                (path/('spawn-%d.bin'%index)).write_bytes(data)
            (self.directory/'spawn-native.json').write_bytes(encoded({'pid':pid,
                'requests':self.spawn_broker.requests,'errors':self.spawn_broker.errors,
                'observations':self.spawn_observations,
                'payloads':[{'profile':profile,'bytes':len(data),'sha256':digest(data)}for profile,data in self.spawn_payloads]}))
            self.spawn_broker.check()
        verify(self.directory,cached_fixture=self.cached_fixture)

    def close(self):
        self.broker.close()
        if self.spawn_broker is not None:self.spawn_broker.close()


def verify(directory,cached_fixture=False):
    """Recheck actual request bytes and final files, independently of server logs."""
    directory = Path(directory)
    raw = directory / 'dependency-response' / 'original'
    marker=directory/'cached-fixture.json'
    if not cached_fixture:assert not marker.exists(),'cached regression cannot supply fresh acceptance state'
    response = original_response(raw)
    if cached_fixture:
        assert response.get('completed') and json.loads(marker.read_text())=={'mode':'cached-native-regression','capture_sha256':response['capture_sha256']}
    data = encoded(response)
    assert (directory / 'dependency-response.json').read_bytes() == data
    identity = digest(data)
    config = config_bytes(response, identity)
    assert (directory / 'dependency-config.bin').read_bytes() == config
    receipt = json.loads((directory / 'dependency-native.json').read_text())
    process = json.loads((directory / 'native-process.json').read_text())
    if not cached_fixture:
        assert re.findall(r'^NATIVE_PROCESS_PID=(\d+)$',(directory/'debugger.txt').read_text(),re.M)==[str(process['pid'])]
    assert receipt['pid'] == process['pid'] and receipt['response_sha256'] == identity
    assert not receipt['errors']
    assert receipt['configuration_sha256'] == digest(config)
    assert len(receipt['requests']) == len(response['sequence'])
    assert receipt['attempts']==receipt['requests'],'extra or rejected native dependency attempt'
    previous = None
    for index, (kind, request) in enumerate(zip(response['sequence'], receipt['requests'])):
        payload = (directory / 'dependency-native' / ('request-%d-%d.bin' % (index, kind))).read_bytes()
        assert (directory/'dependency-native'/('attempt-%d-%d.bin'%(index,kind))).read_bytes()==payload
        assert request == {'kind': kind, 'pid': process['pid'], 'bytes': len(payload), 'sha256': digest(payload)}
        if kind == 0:
            assert payload == config
        else:
            fields = native_decode(payload)
            validate(fields, response['contracts'][kind], directory, directory / 'stdout.bin', response['lowio_flags'], previous)
            previous = fields
    outputs = {}
    for name, expected in response['outputs'].items():
        path=directory/name
        if 'deletions'in response:
            assert not path.exists(),'temporary IR survived native cleanup'
            value=(raw/'observer/c1-output'/name).read_bytes()
        else:value=path.read_bytes()
        assert expected == {'bytes': len(value), 'sha256': digest(value)}, 'actual C1 output differs'
        outputs[name] = expected
    if 'child'in response:verify_spawn(directory,response,process['pid'],cached_fixture)
    if 'deletions'in response and not cached_fixture:
        removed=json.loads((directory/'native-unlink-gdb.json').read_text())
        assert removed=={'pid':process['pid'],'calls':response['deletions']},'actual native removal inputs/results/errors/effects differ'
    elif 'child'not in response:assert not (directory / 'test.obj').exists(), 'C2 ran before the unload frontier'
    prefix=response['invoke_argv'][2][2:]
    assert {p.name for p in directory.glob(prefix+'*')}==(set()if 'deletions'in response else set(response['outputs'])),'unexpected C1 intermediate artifact'
    capture=json.loads((raw/'capture.json').read_text())
    for item in capture['inputs']:
        value=(directory/item['name']).read_bytes()
        assert len(value)==item['bytes'] and digest(value)==item['sha256'],'native source input changed'
    verify_dependencies(directory)
    return summary(response)


def summary(response):
    result={'kind': 'verified captured C1 Invoke response', 'calls': 1, 'status': 0,
            'exports': ['_InvokeCompilerPass@12', '_AbortCompilerPass@4'],
            'module_state': 'released'if'child'in response else'loaded', 'outputs': response['outputs'],
            'argument_count': 26, 'last_pass': 1,
            'response_capture_sha256': response['capture_sha256']}
    if 'child'in response:
        result['child']={'calls':1,'status':0,'environment_source':'target-os',
                         'outputs':response['child']['outputs'],'parent_state':response['child']['after']}
    if 'deletions'in response:result['deletions']=response['deletions']
    if response.get('completed'):result.update(completed=True,exit=response['exit'])
    return result


def verify_spawn(directory,response,pid,cached_fixture=False):
    child=response['child']
    reader=directory/'spawn_reader.py'
    revision=json.loads((directory/'native-build.json').read_text())['shared_revision']
    root=Path(__file__).resolve().parents[4]
    assert reader.read_bytes()==subprocess.check_output(['git','show',revision+':lib/recon/spawn.py'],cwd=root)
    spec=importlib.util.spec_from_file_location('cl_verify_spawn',reader)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    raw=json.loads((directory/'spawn-native.json').read_text())
    assert raw['pid']==pid and not raw['errors']
    assert len(raw['payloads'])==len(raw['observations'])==len(raw['requests'])==1
    payload=(directory/'dependency-native/spawn-0.bin').read_bytes()
    assert raw['payloads'][0]=={'profile':True,'bytes':len(payload),'sha256':digest(payload)}
    actual=module.SpawnBroker._decode(payload,True)
    expected={'mode':0,'pid':pid,'file':child['executable'],'cwd':str(directory.resolve()),
        'argv':child['argv'],'environment':child['environment'],'parent_state':child['before'],
        'environment_source':'target-os'}
    assert actual==expected==raw['observations'][0],'actual complete backend request differs'
    assert raw['requests'][0]=={**{key:value for key,value in expected.items()if key!='environment'},
        'action':'replay','status':0,'parent_state_after':child['after']}
    # Compare actual post-child GDB pointer words with the actual earlier Free
    # request, not the response's expected pointer values. The initial vector
    # remains borrowed; insertion moves only the current vector and retains its
    # existing 78 owned strings.
    if cached_fixture:
        assert response.get('completed')
    elif response.get('completed'):
        from completion_state import decode
        decode(directory,'postchild',response,pid);decode(directory,'exit',response,pid)
    else:
        verify_frontier_environment(directory)
    for name,expected in child['outputs'].items():
        value=(directory/name).read_bytes()
        assert expected=={'bytes':len(value),'sha256':digest(value)},'actual backend output differs'
    executable=(directory/'C2.EXE').read_bytes()
    assert child['executable_input']=={'bytes':len(executable),'sha256':digest(executable)}
    for name,expected in child['provider_inputs'].items():
        value=(directory/name).read_bytes()
        assert expected=={'bytes':len(value),'sha256':digest(value)},'backend provider input differs'


def verify_frontier_environment(directory):
    frontier=json.loads((directory/'frontier-memory.json').read_text());memory={}
    for item in frontier['ranges']:
        for offset,value in enumerate(bytes.fromhex(item['data'])):
            address=item['address']+offset
            assert address not in memory or memory[address]==value
            memory[address]=value
    import struct
    word=lambda address:struct.unpack('<I',bytes(memory[address+i]for i in range(4)))[0]
    current=word(frontier['context']['crt_addresses']['environment_slot'])
    previous=native_decode((directory/'dependency-native/request-3-3.bin').read_bytes())['environment']
    assert current!=previous['address']
    assert [word(current+4*i)for i in range(78)]==previous['pointers']
    assert word(current+4*78) and word(current+4*78)not in previous['pointers'] and word(current+4*79)==0
