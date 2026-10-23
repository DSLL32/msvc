"""Strict captured original RCPP boundary; no PE/Wine execution or discovery."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import time

BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]

def sha(data):return hashlib.sha256(data).hexdigest()
def load_module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module

def reference(directory,record):
    data=(directory/record['path']).read_bytes()
    assert len(data)==record['bytes'] and sha(data)==record['sha256'],'captured process bytes differ: '+record['path']
    return data

def host_path(text,work,environment=None):
    text=text.replace('\\','/')
    if len(text)>1 and text[1]==':':
        drive=text[0].upper();assert drive in ('R','S'),('unknown captured drive',text)
        environment=environment or {}
        base=environment.get('RECON_DRIVE_'+drive)
        assert base,('captured drive has no fixed namespace',drive)
        path=Path(base)/text[3:]
    else:path=work/text
    result=Path(path.anchor)
    for part in path.parts[1:]:
        if part=='.':continue
        if part=='..':result=result.parent;continue
        if result.exists():
            matches=[p for p in result.iterdir() if p.name.lower()==part.lower()]
            assert len(matches)<=1,('ambiguous case-folded path',path)
            result=matches[0] if matches else result/part
        else:result=result/part
    return result

class ProcessFixture:
    def __init__(self,name,work,environment):
        self.directory=BINARY/'cases/native-boundaries'/name
        manifest=(self.directory/'manifest.json').read_bytes()
        index=json.loads((self.directory.parent/'capture-validation.json').read_text())
        pinned=next(r for r in index['cases'] if r['case']==name)
        assert sha(manifest)==pinned['manifest_sha256'],'captured RCPP manifest pin differs'
        self.metadata=json.loads(manifest)
        self.work,self.environment,self.name=work,environment,name
        self.input=reference(self.directory,self.metadata['input'])
        self.response=reference(self.directory,self.metadata['response'])
        self.stdout=reference(self.directory,self.metadata['stdout']);self.stderr=reference(self.directory,self.metadata['stderr'])
        assert sha((ROOT/'toolchains/1.0-nt/bin/RCPP.EXE').read_bytes())==self.metadata['child_sha256'],'captured child identity changed'
        self.calls=[]
        shared=load_module('rc_shared_spawn',ROOT/'lib/recon/spawn.py')
        self.broker=shared.SpawnBroker(self.request);environment.update(self.broker.environment())
        self.replay_seconds=0

    def request(self,request):
        start=time.perf_counter();m=self.metadata
        assert not self.calls,'unexpected repeated RCPP invocation'
        assert request['mode']==0 and request['cwd']==str(self.work),'RCPP mode/cwd differs'
        assert m['canonical_cwd']==self.environment['RECON_CANONICAL_CWD'],'RCPP canonical cwd differs'
        assert host_path(m['canonical_cwd'],self.work,self.environment).resolve()==self.work.resolve(),'RCPP namespace/cwd differs'
        assert request['file']==m['application'],('RCPP executable spelling differs',request['file'],m['application'])
        assert sha(host_path(request['file'],self.work,self.environment).read_bytes())==m['child_sha256'],'RCPP executable bytes differ'
        env=dict(entry.split('=',1) for entry in request['environment'])
        for key in ('INCLUDE','WINEPATH','RECON_PROGRAM_PATH','TEMP','TMP'):
            assert env.get(key)==self.environment.get(key),'RCPP environment differs: '+key
        if m.get('PATH'):assert env.get('PATH')==m['PATH'],'RCPP PATH differs'
        for key in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS'):
            assert key not in env,'unexpected RCPP environment: '+key
        digits=('%06d'%request['pid'])[-6:]
        incoming_name,outgoing_name='RC'+digits,'RD'+digits
        options=m['argv'][1].replace(m['input_name'],incoming_name).replace(m['output_name'],outgoing_name)
        assert request['argv']==[m['argv'][0],options],('RCPP argv differs',request['argv'])
        actual=(self.work/incoming_name).read_bytes()
        assert actual==self.input,'complete prepared RCPP input differs'
        for item in m['reads']:
            text=item['path'].replace(m['input_name'],incoming_name).replace(m['output_name'],outgoing_name)
            path=host_path(text,self.work,self.environment)
            assert path.exists()==item['exists'],('RCPP input existence differs',text)
            if item['exists']:
                assert path.read_bytes()==reference(self.directory,item['capture']),('RCPP complete read differs',text)
        # Bind only the recorded temporary filename tokens in #line directives.
        # Everything outside these exact captured fields is copied unchanged.
        output=bytearray(self.response)
        for offset in m['temporary_input_response_offsets']:
            old=m['input_name'].encode();new=incoming_name.encode()
            assert output[offset:offset+len(old)]==old and len(old)==len(new)
            output[offset:offset+len(new)]=new
        (self.work/outgoing_name).write_bytes(output)
        self.replay_seconds+=time.perf_counter()-start
        self.calls.append(dict(pid=request['pid'],input=incoming_name,output=outgoing_name,
            input_bytes=len(actual),input_sha256=sha(actual),response_bytes=len(output),response_sha256=sha(output),
            captured_read_attempts=len(m['reads']),status=m['exit']))
        return dict(status=m['exit'],stdout=self.stdout,stderr=self.stderr)

    def fileno(self):return self.broker.fileno()
    def close(self):self.broker.close()
    def check(self):
        self.broker.check();assert len(self.calls)==1,'RCPP boundary not consumed'
    def receipt(self):return dict(kind='verified captured original RCPP response',calls=self.calls,
        requests=self.broker.requests,errors=self.broker.errors,replay_seconds=self.replay_seconds,
        manifest_sha256=sha((self.directory/'manifest.json').read_bytes()),Wine_invocations=0)
