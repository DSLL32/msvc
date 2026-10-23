#!/usr/bin/env python3
"""Native-only checks that unknown dependency requests fail before callbacks."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
from common import HERE


def check():
    start=time.perf_counter()
    response=(HERE/'cases/help/reference/rcdll-response.bin').read_bytes()
    rejected=[]
    with tempfile.TemporaryDirectory(prefix='rc-boundary-',dir=str(Path.home()/'.ai-tmp')) as temporary:
        directory=Path(temporary)
        def invoke(mode,data=response,expected=125):
            path=directory/'response.bin';path.write_bytes(data)
            env={k:v for k,v in os.environ.items() if not k.startswith(('WINE','RECON_','RC_'))}
            env.update(RC_DEPENDENCY_RESPONSE=str(path),RC_REJECTION=mode)
            result=subprocess.run([str(HERE/'build/native/check-boundary'),'/help'],env=env,cwd=directory,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5)
            assert result.returncode==expected,(mode,result.returncode,result.stderr)
            if expected==125:
                assert result.stdout==b'' and result.stderr.startswith(b'RC dependency replay: '),mode
                rejected.append(mode)
            else:assert result.stdout==b'UNEXPECTED_CALLBACK\r\n','Positive callback control did not execute'
        invoke('positive-callback-control',expected=126)
        for mode in ['host-case','window','flags','resource','callback','argc','argv-alias','argv-mutation','argv-terminator']:
            invoke(mode)
        invoke('truncated-response',response[:-1])
        invoke('trailing-response',response+b'X')
        invoke('wrong-version',b'X'+response[1:])
    return {'status':'pass','rejected':rejected,'elapsed_seconds':time.perf_counter()-start,'wine_invocations':0}

if __name__=='__main__':print(json.dumps(check(),indent=2))
