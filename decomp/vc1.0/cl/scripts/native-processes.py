"""Strict original-backed synchronous compiler/linker responses for native CL."""
import gzip
import importlib.machinery
import json
import os
from pathlib import Path
import signal
import sys

HERE=Path(__file__).resolve().parent
cases=importlib.machinery.SourceFileLoader('cl_native_cases',str(HERE/'native-cases.py')).load_module()
sys.path.insert(0,str(cases.ROOT/'lib/recon'))
from spawn import SpawnBroker
STORE=cases.CASES/'native-boundaries'


def contents(record):
    data=gzip.decompress((STORE/'blobs'/(record['sha256']+'.gz')).read_bytes())
    assert len(data)==record['bytes'] and cases.sha(data)==record['sha256'], 'captured blob corrupt'
    return data


def inventory(work):
    return {str(p.relative_to(work)):dict(sha256=cases.sha(p.read_bytes()),bytes=p.stat().st_size)
            for p in sorted(work.rglob('*')) if p.is_file()}


def verify_toolchain():
    expected=json.loads((STORE/'toolchain.json').read_text())
    actual={str(p.relative_to(cases.TC)) for p in cases.TC.rglob('*') if p.is_file()}
    assert actual==set(expected), 'toolchain dependency set changed'
    for name,record in expected.items():
        path=cases.TC/name
        assert path.stat().st_size==record['bytes'] and cases.sha(path.read_bytes())==record['sha256'], 'toolchain dependency changed: '+name


class ProcessFixture:
    def __init__(self,case,reference,work):
        self.case,self.reference,self.work=case,reference,work
        self.count=0
        self.missing_probe=0
        self.broker=SpawnBroker(self.respond)

    def respond(self,request):
        assert self.count<len(self.reference['boundaries']), 'unexpected child invocation'
        expected=self.reference['boundaries'][self.count]
        assert request['mode']==expected['mode']==0, 'spawn mode mismatch'
        assert expected['cwd']==cases.canonical_cwd(self.case['id']), 'captured DOS cwd mismatch'
        # Broker cwd is host transport; the target _getcwd API uses canonical S:.
        assert Path(request['cwd']).resolve()==self.work.resolve(), 'child cwd mismatch'
        namespace=self.work.parents[2]
        assert cases.host_path(expected['cwd'],namespace).resolve()==self.work.resolve(), 'DOS cwd mapping mismatch'
        expected_file=expected['file']
        missing=expected['status']==-1
        if missing:
            # Reference capture wraps _spawnvpe; the shared broker wraps each
            # inner _spawnve. Retain native PATH search and validate every
            # missing candidate against the frozen aggregate request/readset.
            assert expected['errno']==2 and not contents(expected['stdout']) and not contents(expected['stderr'])
            assert expected['before']==expected['after'], 'failed-spawn capture has side effects'
            candidates=[expected_file]+[part.strip().strip('"')+'\\'+expected_file for part in self.reference['environment']['PATH'].split(';') if part.strip()]
            assert self.missing_probe<len(candidates), 'extra missing-file probe'
            expected_file=candidates[self.missing_probe]
            if ':' in expected_file:
                host=cases.host_path(expected_file)
            else:host=self.work/expected_file
            assert host.parent.is_dir() and not any(p.name.casefold()==host.name.casefold() for p in host.parent.iterdir()), 'captured missing dependency now exists'
        assert request['file']==expected_file, 'child file mismatch: '+repr((request['file'],expected_file))
        assert request['argv']==expected['argv'], 'child argv mismatch: '+repr((request['argv'],expected['argv']))
        environment=dict(item.split('=',1) for item in request['environment'] if '=' in item and not item.startswith('='))
        if expected['environment'] is not None:
            expected_environment=dict(item.split('=',1) for item in expected['environment'])
            # Tracing and the broker travel separately from the child's logical
            # single-variable environment; compare all logical child entries.
            actual={k:v for k,v in environment.items() if not k.startswith('RECON_')}
            assert actual==expected_environment, 'child explicit environment mismatch: '+repr((actual,expected_environment))
        else:
            for key,value in self.reference['environment'].items():
                assert environment.get(key)==value, 'child inherited environment mismatch: '+key
            for key in ('CL','_CL_','_MSC_IDE_FLAGS','LINK','_LINK_','MSC_CMD_FLAGS'):
                if key not in self.reference['environment']:
                    assert key not in environment, 'unexpected child environment: '+key
        if expected.get('executable_input'):
            record=expected['executable_input']
            if record.get('fixture_source'):
                source=record['fixture_source'];image=cases.BINARY/source['path']
                assert cases.sha(image.read_bytes())==source['sha256'], 'captured helper source changed'
                contents(record) # The captured original helper image, never executed.
            else:
                image=cases.host_path(record['path'])
                assert image.is_file() and cases.sha(image.read_bytes())==record['sha256'], 'captured child executable changed'
        assert inventory(self.work)==expected['before'], 'child complete input tree differs'
        for name in set(expected['before'])-set(expected['after']):
            (self.work/name).unlink()
        for name,record in expected['after'].items():
            destination=self.work/name
            destination.parent.mkdir(parents=True,exist_ok=True)
            destination.write_bytes(contents(record))
        if missing:
            self.missing_probe+=1
            if self.missing_probe==len(candidates):self.count+=1
        else:self.count+=1
        if self.case.get('mode')=='handler-child':
            os.kill(request['pid'],signal.SIGINT)
        result=dict(status=expected['status'],stdout=contents(expected['stdout']),stderr=contents(expected['stderr']))
        if expected['status']==-1:result['errno']=expected['errno']
        return result

    def finish(self):
        self.broker.close();self.broker.check()
        assert self.count==len(self.reference['boundaries']), 'captured children were skipped'
