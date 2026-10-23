#!/usr/bin/env python3
"""Native-only rich replay ABI, input and fail-closed dependency controls."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
from common import HERE
from rich_reference import CASE,REFERENCE,ARGUMENTS,verify
from importlib.util import spec_from_file_location,module_from_spec

spec=spec_from_file_location('rc_rich_check',HERE/'scripts/check-rich.py')
rich=module_from_spec(spec);spec.loader.exec_module(rich)

def check():
    started=time.perf_counter()
    _,provider,_,observed=verify();rejected=[]
    with tempfile.TemporaryDirectory(prefix='rc-rich-boundary-',dir=str(Path.home()/'.ai-tmp')) as temporary:
        temporary=Path(temporary)
        def directory(name):
            result=temporary/name;result.mkdir()
            for item in provider['source_inputs']:
                destination=result/item;destination.parent.mkdir(parents=True,exist_ok=True)
                shutil.copyfile(CASE/'source'/item,destination)
            return result
        def invoke(mode,mutate=None,expected=125,response=None):
            work=directory(mode)
            if mutate:mutate(work)
            path=REFERENCE/'rcdll-response.bin'
            if response is not None:
                path=temporary/(mode+'.bin');path.write_bytes(response)
            env={key:value for key,value in os.environ.items() if not key.startswith(('WINE','RECON_','RC_'))}
            env.update(RC_DEPENDENCY_RESPONSE=str(path),RC_DEPENDENCY_CWD=observed['cwd']+('X' if mode=='wrong-logical-cwd' else ''),RC_REJECTION=mode)
            result=subprocess.run([str(HERE/'build/native/check-boundary')]+ARGUMENTS,cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5)
            assert result.returncode==expected,(mode,result.returncode,result.stdout,result.stderr)
            assert result.stdout==b'','Unexpected diagnostic callback or stream '+mode
            if expected==125:
                assert result.stderr.startswith(b'RC dependency replay: '),mode
                assert not (work/'resources.res').exists() or mode=='existing-artifact','Rejected request wrote artifact '+mode
                if mode=='existing-artifact':assert (work/'resources.res').read_bytes()==b'existing','Rejected request modified existing artifact'
                rejected.append(mode)
            else:
                assert result.stderr==b'' and (work/'resources.res').read_bytes()==(REFERENCE/'resources.res').read_bytes(),'Positive artifact/alias contract failed'
        invoke('rich-positive',expected=0)
        for mode in ['host-case','window','flags','resource','callback','argc','argv-alias','argv-mutation','argv-terminator','environment-mutation','handler','wrong-logical-cwd']:
            invoke(mode)
        invoke('changed-source',lambda path:(path/'resources.rc').write_bytes(b'changed'))
        invoke('missing-icon',lambda path:(path/'assets/multi.ico').unlink())
        invoke('existing-artifact',lambda path:(path/'resources.res').write_bytes(b'existing'))
        response=(REFERENCE/'rcdll-response.bin').read_bytes()
        invoke('truncated-response',response=response[:-1])
        invoke('trailing-response',response=response+b'X')
        # The Python fixture verifies the exact directory input set before launch;
        # the in-process adapter rechecks every recorded byte at its RC boundary.
        work=directory('unknown-input');(work/'unrecorded.h').write_text('extra')
        try:rich.verify_case_tree(work,provider['source_inputs'])
        except AssertionError:rejected.append('unknown-input-before-execution')
        else:raise AssertionError('Unknown dependency input accepted')
    return {'status':'pass','case':'resources-rich','positive':'actual returned argv allocation+offset identity and exact artifact','rejected':rejected,'elapsed_seconds':time.perf_counter()-started,'wine_invocations':0}

if __name__=='__main__':print(json.dumps(check(),indent=2))
