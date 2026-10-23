#!/usr/bin/env python3
"""Native-only error replay input guards and full post-callback-buffer control."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
from importlib.util import spec_from_file_location,module_from_spec
from common import HERE
from error_reference import REFERENCE,ARGUMENTS,verify

spec=spec_from_file_location('rc_error_check',HERE/'scripts/check-error.py')
error_check=module_from_spec(spec);spec.loader.exec_module(error_check)

def check():
    started=time.perf_counter();_,observed=verify();rejected=[]
    response=(REFERENCE/'rcdll-response.bin').read_bytes()
    with tempfile.TemporaryDirectory(prefix='rc-error-boundary-',dir=str(Path.home()/'.ai-tmp')) as temporary:
        temporary=Path(temporary)
        def invoke(mode,mutate=None,expected=125,data=response):
            directory=temporary/mode;directory.mkdir()
            if mutate:mutate(directory)
            path=temporary/(mode+'.bin');path.write_bytes(data)
            environment={key:value for key,value in os.environ.items() if not key.startswith(('WINE','RECON_','RC_'))}
            environment.update(RC_DEPENDENCY_RESPONSE=str(path),RC_DEPENDENCY_CWD=observed['cwd']+('X' if mode=='wrong-logical-cwd' else ''),RC_REJECTION=mode)
            result=subprocess.run([str(HERE/'build/native/check-boundary')]+ARGUMENTS,cwd=directory,env=environment,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5)
            assert result.returncode==expected,(mode,result.returncode,result.stdout,result.stderr)
            if expected==126:assert result.stdout==b'UNEXPECTED_CALLBACK\r\n','Positive callback control did not execute'
            else:
                assert result.stdout==b'' and result.stderr.startswith(b'RC dependency replay: '),'Bad request reached callback '+mode
                rejected.append(mode)
        invoke('positive-callback-control',expected=126)
        for mode in ['host-case','window','flags','resource','callback','argc','argv-alias','argv-mutation','argv-terminator','environment-mutation','handler','wrong-logical-cwd']:
            invoke(mode)
        invoke('source-present',lambda path:(path/'missing%source.rc').write_bytes(b'new input'))
        invoke('output-present',lambda path:(path/'missing%source.res').write_bytes(b'existing'))
        invoke('truncated-response',data=response[:-1]);invoke('trailing-response',data=response+b'X')
        directory=temporary/'unknown-input';directory.mkdir();(directory/'unknown.rc').write_bytes(b'unknown')
        try:error_check.verify_absence_tree(directory)
        except AssertionError:rejected.append('unknown-input-before-execution')
        else:raise AssertionError('Unknown input accepted')
        # Flip an expected byte AFTER the callback's inserted NUL: printed text
        # stays identical through that callback, so only the complete-buffer
        # contract can reject it. Use the real diagnostic, not the probe callback.
        after=bytes.fromhex(observed['callbacks'][1]['after']);assert response.count(after)==1
        position=response.index(after)+len(after)-2
        assert response[position]==10 and after[-3]==0
        changed=bytearray(response);changed[position]=11
        path=temporary/'bad-post-buffer.bin';path.write_bytes(changed)
        directory=temporary/'bad-post-buffer';directory.mkdir()
        environment={key:value for key,value in os.environ.items() if not key.startswith(('WINE','RECON_','RC_'))}
        environment.update(RC_DEPENDENCY_RESPONSE=str(path),RC_DEPENDENCY_CWD=observed['cwd'])
        result=subprocess.run([str(HERE/'build/native/rc')]+ARGUMENTS,cwd=directory,env=environment,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5)
        assert result.returncode==125 and b'callback result/mutable buffer' in result.stderr
        assert result.stdout==(REFERENCE/'stdout.bin').read_bytes()[:-4],'Callback did not reach the otherwise-correct visible output'
        rejected.append('wrong-callback-byte-after-NUL')
    return {'status':'pass','case':'missing-percent-source','rejected':rejected,'elapsed_seconds':time.perf_counter()-started,'wine_invocations':0}

if __name__=='__main__':print(json.dumps(check(),indent=2))
