"""Strict native process fixtures for LINK's DLL and CVPACK boundaries."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import time

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]
from case_paths import PROGRAM, LIBRARY


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def sha(data):
    return hashlib.sha256(data).hexdigest()


class ProcessFixture:
    def __init__(self, case, work, environment):
        assert case in ('hello-dll','hello-debug-dll')
        self.case, self.work = case, work
        self.environment = environment
        self.self_seen = self.cv_seen = False
        self.cv_receipt = None
        self.verified_environment = None
        self.boundary = None
        if case=='hello-debug-dll':
            self.directory = BINARY/'cases/native-boundaries/hello-debug-dll'
            self.boundary = json.loads((self.directory/'manifest.json').read_text())
            self.parser = load_module('native_cvpack_artifacts', BINARY.parent/'fixtures/link-debug-dll/artifacts.py')
            before = self.reference(self.boundary['before'])
            self.packed = self.reference(self.boundary['after'])
            self.before_fields = self.boundary['before']['ignored_fields']
            self.after_fields = self.boundary['after']['ignored_fields']
            assert self.parser.timestamp_fields(before,'debug-pe')==self.before_fields
            assert self.parser.timestamp_fields(self.packed,'debug-pe')==self.after_fields
            self.before_normalized = self.clear_clocks(before,self.before_fields)
            assert sha(self.before_normalized)==self.boundary['before']['normalized_sha256']
            assert sha(self.parser.normalize_artifact(self.packed,'debug-pe'))==self.boundary['after']['normalized_sha256']
        shared = load_module('recon_native_spawn', ROOT/'lib/recon/spawn.py')
        self.broker = shared.SpawnBroker(self.request)
        environment.update(self.broker.environment())

    def reference(self, record):
        data = (self.directory/record['path']).read_bytes()
        assert len(data)==record['bytes'] and sha(data)==record['sha256'], 'captured process reference differs'
        return data

    @staticmethod
    def clear_clocks(data, fields):
        result = bytearray(data)
        for field in fields:
            result[field['offset']:field['offset']+field['bytes']] = bytes(field['bytes'])
        return bytes(result)

    def request(self, request):
        assert request['mode']==0 and request['cwd']==str(self.work)
        environment = dict(entry.split('=',1) for entry in request['environment'])
        keys = ['LIB','RECON_PROGRAM_PATH','RECON_SELF_SPAWN','TMP','TEMP',
                'RECON_TRACE_FD','RECON_TRACE_FILTER','RECON_TRACE_SEEN','RECON_TRACE_STATE_FD']
        for key in keys:
            assert environment.get(key)==self.environment.get(key), 'spawn environment: '+key
        for key in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB'):
            assert key not in environment, 'unexpected inherited '+key
        self.verified_environment = {key:environment[key] for key in keys[:3]}
        if request['file']==PROGRAM:
            assert not self.self_seen, 'unexpected repeated LINK self-spawn'
            self.self_seen = True
            if self.case=='hello-dll':
                expected = [PROGRAM,'-lib','-def:','-out:hello.lib','-name:hello.dll',
                            '-machine:ix86','-nologo','hello-dll.obj']
            else:
                expected = [PROGRAM,'-lib','-def:hello-debug.def','-out:hello-debug.lib',
                            '-name:hello-debug.dll','-machine:ix86','-nologo',
                            'hello-debug.obj','helpers.lib','resources.res']
            assert request['argv']==expected, 'LINK self-spawn arguments differ: '+repr(request['argv'])
            return None  # The shared boundary executes the actual native ELF.
        assert self.boundary is not None and self.self_seen and not self.cv_seen, 'unregistered child process'
        self.cv_seen = True
        process = self.boundary['process']
        assert process['stdout']==process['stderr']=='', 'CVPACK boundary has unsupported nonempty streams'
        assert request['file']==process['file'] and request['argv']==process['argv'], 'CVPACK invocation differs'
        start = time.perf_counter()
        target = self.work/request['argv'][-1]
        incoming = target.read_bytes()
        assert self.parser.timestamp_fields(incoming,'debug-pe')==self.before_fields
        normalized = self.clear_clocks(incoming,self.before_fields)
        digest = sha(normalized)
        assert normalized==self.before_normalized, 'CVPACK prepack input differs'
        assert digest==self.boundary['before']['normalized_sha256']
        response = bytearray(self.packed)
        assert len(self.before_fields)==len(self.after_fields)
        for before,after in zip(self.before_fields,self.after_fields):
            assert before['name']==after['name'] and before['bytes']==after['bytes']
            response[after['offset']:after['offset']+after['bytes']] = incoming[before['offset']:before['offset']+before['bytes']]
        _,optional,_ = self.parser.pe_layout(response)
        struct.pack_into('<I',response,optional+64,self.parser.checksum(response))
        target.write_bytes(response)
        self.cv_receipt = dict(input_bytes=len(incoming),input_sha256=sha(incoming),
            normalized_input_sha256=digest,response_bytes=len(response),response_sha256=sha(response),
            captured_response_sha256=self.boundary['after']['sha256'],
            exit=process['exit'],stdout_bytes=0,stderr_bytes=0,
            replay_seconds=time.perf_counter()-start,
            scope='External CVPACK test double; complete original-backed prepack input checked before replay')
        return process['exit']

    def fileno(self):
        return self.broker.fileno()

    def close(self):
        self.broker.close()

    def check(self):
        self.broker.check()
        assert self.self_seen, 'native LINK child not dispatched'
        assert self.cv_seen==(self.case=='hello-debug-dll'), 'CVPACK boundary missing'
        assert len(self.broker.requests)==(2 if self.cv_seen else 1), 'unexpected native process graph'
        assert self.broker.requests[0]['action']=='native', 'LINK child was replayed'

    def receipt(self):
        return dict(requests=self.broker.requests,errors=self.broker.errors,
                    verified_environment=self.verified_environment,
                    self_child=('actual native ELF; preserved canonical argv and inherited descriptors'
                                if any(r['action']=='native' for r in self.broker.requests) else 'not dispatched'),
                    cvpack=self.cv_receipt)
