"""Strict captured system/copy boundary. Never run a command processor."""
import hashlib,importlib.util,json,time
from pathlib import Path
BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]

def sha(data):return hashlib.sha256(data).hexdigest()

class ProcessFixture:
    def __init__(self,work,environment):
        self.work=Path(work)
        self.environment=environment
        self.directory=BINARY/'cases/native-boundaries/rom-repro'
        self.manifest=json.loads((self.directory/'manifest.json').read_text())
        proof_path=self.directory/self.manifest['provenance']
        proof=json.loads(proof_path.read_text())
        events_path=proof_path.parent/'events-1.jsonl'
        assert sha(events_path.read_bytes())==proof['copy_boundary']['events_sha256'], 'Original copy events changed'
        events=[json.loads(line) for line in events_path.read_text().splitlines()]
        creates={event['pid']:event for event in events if event['event']=='create'}
        begins=[event for event in events if event['event']=='copy-begin']
        ends=[event for event in events if event['event']=='copy-end']
        assert len(begins)==len(ends)==len(self.manifest['steps'])==2
        for step,begin,end in zip(self.manifest['steps'],begins,ends):
            create=creates[begin['pid']]
            assert create['image']==step['file'] and create['command_line']==step['command_line']
            assert begin['cwd']==self.manifest['raw_peb_cwd']
            captured_environment={k.upper():v for k,v in (entry.split('=',1) for entry in begin['environment'])}
            assert captured_environment==self.manifest['environment']
            assert begin['pid']==end['pid'] and end['code']==step['exit']==0
            for stream in ('stdout','stderr'):
                assert begin[stream+'_start']==end[stream+'_end']
            incoming=next(item for item in proof['inputs'] if Path(item['path']).name==step['input']['name'])
            outgoing=next(item for item in proof['outputs'] if item['output_name']==step['output']['name'])
            for key in ('bytes','sha256'):
                assert incoming[key]==step['input'][key]
                assert outgoing[key]==step['output'][key]
        self.calls=[]
        for step in self.manifest['steps']:
            assert step['stdout']==step['stderr']=='', 'Unimplemented nonempty system response streams'
            self.reference(step['input']);self.reference(step['output'])
        spec=importlib.util.spec_from_file_location('recon_repro_spawn',ROOT/'lib/recon/spawn.py')
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        self.broker=module.SpawnBroker(self.request)
        environment.update(self.broker.environment())

    def reference(self,record):
        data=(self.directory/record['path']).read_bytes()
        assert len(data)==record['bytes'] and sha(data)==record['sha256'], 'Captured copy reference changed'
        return data

    def request(self,request):
        index=len(self.calls)
        assert index<len(self.manifest['steps']), 'Unexpected repeated system command'
        step=self.manifest['steps'][index]
        assert request['mode']==0 and request['cwd']==str(self.work), 'System mode/cwd differs'
        assert request['file']==step['file'] and request['argv']==step['argv'], 'System invocation differs'
        assert ' '.join(request['argv'])==step['command_line'], 'Original command line differs'
        environment={key.upper():value for key,value in (entry.split('=',1) for entry in request['environment'])}
        for key,value in self.manifest['environment'].items():
            assert environment.get(key)==value, 'System environment differs: '+key
        for key in self.manifest['unset_environment']:
            assert key not in environment, 'Unexpected system environment: '+key
        source=self.work/step['input']['name'];destination=self.work/step['output']['name']
        incoming=source.read_bytes();expected=self.reference(step['input'])
        assert incoming==expected, 'System copy input differs'
        assert not destination.exists(), 'System copy destination already exists'
        assert destination.parent.is_dir(), 'System copy destination directory missing'
        response=self.reference(step['output'])
        start=time.perf_counter();destination.write_bytes(response)
        self.calls.append(dict(index=index+1,input_bytes=len(incoming),input_sha256=sha(incoming),output_bytes=len(response),output_sha256=sha(response),exit=step['exit'],stdout_bytes=0,stderr_bytes=0,replay_seconds=time.perf_counter()-start))
        return step['exit']

    def fileno(self):return self.broker.fileno()
    def close(self):self.broker.close()
    def check_partial(self):self.broker.check()
    def check(self):
        self.check_partial()
        assert len(self.calls)==len(self.manifest['steps']), 'Missing captured system command'
        assert all(x['action']=='replay' for x in self.broker.requests), 'System command was executed'
    def receipt(self):
        return dict(boundary='captured command response; no shell/Windows execution',manifest_sha256=sha((self.directory/'manifest.json').read_bytes()),calls=self.calls,requests=self.broker.requests,errors=self.broker.errors)
