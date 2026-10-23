"""Host-only rejection checks for captured LINK_REPRO copy responses."""
import importlib.util,json,tempfile,time
from pathlib import Path
BINARY=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('native_repro_checked',Path(__file__).with_name('native-repro.py'))
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

def main():
    start=time.perf_counter()
    directory=Path(tempfile.mkdtemp(prefix='repro-boundary-',dir=Path.home()/'.ai-tmp/msvc2/vc1.0/link/native'))
    results=[]
    for kind in ('wrong-command','wrong-environment','changed-input','existing-output','missing-directory','valid-copy'):
        work=directory/kind;work.mkdir();(work/'repro').mkdir()
        environment=dict(json.loads((BINARY/'cases/native-boundaries/rom-repro/manifest.json').read_text())['environment'])
        fixture=module.ProcessFixture(work,environment)
        step=fixture.manifest['steps'][0]
        source=work/step['input']['name'];source.write_bytes(fixture.reference(step['input']))
        request=dict(mode=0,pid=123,file=step['file'],argv=list(step['argv']),cwd=str(work),environment=[k+'='+v for k,v in environment.items()])
        if kind=='wrong-command':request['argv'][-1]+=' changed'
        if kind=='wrong-environment':request['environment']=[e for e in request['environment'] if not e.startswith('LIB=')]
        if kind=='changed-input':source.write_bytes(b'changed')
        if kind=='existing-output':(work/step['output']['name']).write_bytes(b'old')
        if kind=='missing-directory':(work/'repro').rmdir()
        try:
            status=fixture.request(request)
            assert kind=='valid-copy' and status==0
            assert (work/step['output']['name']).read_bytes()==fixture.reference(step['output'])
            results.append(dict(test=kind,status='pass',processes_launched=0))
        except AssertionError as error:
            if kind=='valid-copy':raise
            assert not fixture.calls, 'Rejected response changed state'
            results.append(dict(test=kind,status='rejected',error=str(error),processes_launched=0))
        finally:
            fixture.close();fixture.check_partial()
    receipt=dict(status='pass',scope='Host-only direct protocol callbacks; no executable or shell launched',tests=results,total_seconds=time.perf_counter()-start)
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(directory/'receipt.json')
if __name__=='__main__':main()
