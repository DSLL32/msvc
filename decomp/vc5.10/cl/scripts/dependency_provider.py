"""Owned native C1 response adapter. No Wine, PE execution or capture fallback."""
import hashlib
import json
from pathlib import Path
import re
import shutil

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
    def __init__(self, original, directory):
        self.directory = Path(directory).resolve()
        self.raw = self.directory / 'dependency-response' / 'original'
        copy_raw(original, self.raw)
        self.response = original_response(self.raw)
        self.response_bytes = encoded(self.response)
        self.identity = digest(self.response_bytes)
        self.contract = self.directory / 'dependency-response.json'
        self.contract.write_bytes(self.response_bytes)
        self.config = config_bytes(self.response, self.identity)
        self.config_path = self.directory / 'dependency-config.bin'
        self.config_path.write_bytes(self.config)
        self.previous = None
        self.broker = DllBroker(self.config, self.identity, self.response['sequence'], self._callback)

    def environment(self):
        return {**self.broker.environment(), 'CL_DEPENDENCY_CONFIG': str(self.config_path)}

    def fileno(self):
        return self.broker.fileno()

    def _inputs(self):
        verify_dependencies(self.directory)
        capture = json.loads((self.raw / 'capture.json').read_text())
        for item in capture['inputs']:
            data = (self.directory / item['name']).read_bytes()
            assert len(data) == item['bytes'] and digest(data) == item['sha256'], 'native source input changed'
        for name in self.response['outputs']:
            assert Path(name).name == name and not (self.directory / name).exists(), 'unknown existing C1 output'
        for name in ('test.obj', 'vc50.pdb', 'vc50.idb'):
            assert not (self.directory / name).exists(), 'unobserved C1 case input: ' + name

    def _callback(self, kind, data):
        fields = native_decode(data)
        validate(fields, self.response['contracts'][kind], self.directory, self.directory / 'stdout.bin',
                 self.response['lowio_flags'], previous=self.previous)
        self._inputs()
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
        verify(self.directory)

    def close(self):
        self.broker.close()


def verify(directory):
    """Recheck actual request bytes and final files, independently of server logs."""
    directory = Path(directory)
    raw = directory / 'dependency-response' / 'original'
    response = original_response(raw)
    data = encoded(response)
    assert (directory / 'dependency-response.json').read_bytes() == data
    identity = digest(data)
    config = config_bytes(response, identity)
    assert (directory / 'dependency-config.bin').read_bytes() == config
    receipt = json.loads((directory / 'dependency-native.json').read_text())
    process = json.loads((directory / 'native-process.json').read_text())
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
        value = (directory / name).read_bytes()
        assert expected == {'bytes': len(value), 'sha256': digest(value)}, 'actual C1 output differs'
        outputs[name] = expected
    assert not (directory / 'test.obj').exists(), 'C2 ran before the unload frontier'
    prefix=response['invoke_argv'][2][2:]
    assert {p.name for p in directory.glob(prefix+'*')}==set(response['outputs']),'unexpected C1 intermediate artifact'
    capture=json.loads((raw/'capture.json').read_text())
    for item in capture['inputs']:
        value=(directory/item['name']).read_bytes()
        assert len(value)==item['bytes'] and digest(value)==item['sha256'],'native source input changed'
    verify_dependencies(directory)
    return summary(response)


def summary(response):
    return {'kind': 'verified captured C1 Invoke response', 'calls': 1, 'status': 0,
            'exports': ['_InvokeCompilerPass@12', '_AbortCompilerPass@4'],
            'module_state': 'loaded', 'outputs': response['outputs'],
            'argument_count': 26, 'last_pass': 1,
            'response_capture_sha256': response['capture_sha256']}
