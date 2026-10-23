"""Strict captured process failures; unknown dependencies always fail."""
import importlib.util
import json
from state import BINARY,ROOT

spec=importlib.util.spec_from_file_location('cvpack_shared_spawn',ROOT/'lib/recon/spawn.py')
shared=importlib.util.module_from_spec(spec)
spec.loader.exec_module(shared)

class Processes:
    def __init__(self,name,work,environment,expected):
        self.name,self.work,self.environment,self.expected=name,work,environment,expected
        self.reference=None
        if name=='help':
            path=BINARY/'cases/help/reference/spawn.json'
            if path.exists():self.reference=json.loads(path.read_text())
        self.broker=shared.SpawnBroker(self.request)
        environment.update(self.broker.environment())

    def request(self,request):
        assert self.reference is not None,'unregistered process dependency'
        assert not self.broker.requests,'unexpected repeated help-viewer request'
        reference=self.reference
        assert request['cwd']==str(self.work) and request['mode']==reference['mode']
        assert request['file']==reference['file'] and request['argv']==reference['argv']
        environment=dict(entry.split('=',1) for entry in request['environment'])
        for key in ('LIB','PATH','RECON_PROGRAM_PATH'):
            assert environment.get(key)==self.environment.get(key),key
        assert environment.get('PATH')=='' and '_MSC_IDE_FLAGS' not in environment
        assert not (self.work/'qh.exe').exists(),'missing viewer unexpectedly supplied'
        assert not list(self.work.iterdir()),'unexpected inputs to missing-viewer boundary'
        assert reference['response']=={'status':-1,'errno':2}
        return dict(reference['response'])

    def fileno(self):return self.broker.fileno()
    def close(self):self.broker.close()
    def check(self):
        self.broker.check()
        assert len(self.broker.requests)==self.expected,self.broker.requests
    def receipt(self):return dict(requests=self.broker.requests,errors=self.broker.errors)
