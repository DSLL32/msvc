#!/usr/bin/env python3
"""Host-only rejection checks: never launch LINK, CVPACK, Wine or originals."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import tempfile

BINARY = Path(__file__).resolve().parents[1]


def main():
    spec = importlib.util.spec_from_file_location('native_boundary_checks',Path(__file__).with_name('native-processes.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    scratch = Path.home()/'.ai-tmp/msvc2/vc1.0/link/native'
    scratch.mkdir(parents=True,exist_ok=True)
    directory = Path(tempfile.mkdtemp(prefix='boundary-rejections-',dir=scratch))
    source = (BINARY/'cases/native-boundaries/hello-debug-dll/before.dll').read_bytes()
    results=[]
    for label in ('input-mutation','wrong-invocation'):
        work=directory/label
        work.mkdir()
        file=work/'hello-debug.dll'
        file.write_bytes(source)
        environment=dict(LIB=module.LIBRARY,RECON_PROGRAM_PATH=module.PROGRAM,RECON_SELF_SPAWN='1')
        fixture=module.ProcessFixture('hello-debug-dll',work,environment)
        fixture.self_seen=True  # Isolate the external boundary; no child dispatched.
        process=fixture.boundary['process']
        argv=list(process['argv'])
        if label=='input-mutation':
            changed=bytearray(source)
            changed[0x500]^=1  # .text byte, outside all declared clock fields.
            file.write_bytes(changed)
        else:
            argv[2]='/cvold'
        before=hashlib.sha256(file.read_bytes()).hexdigest()
        request=dict(mode=0,pid=os.getpid(),file=process['file'],cwd=str(work),argv=argv,
                     environment=[key+'='+value for key,value in environment.items()])
        try:
            fixture.request(request)
            raise RuntimeError('negative boundary was accepted')
        except AssertionError as error:
            message=str(error)
        finally:
            fixture.close()
        expected='CVPACK prepack input differs' if label=='input-mutation' else 'CVPACK invocation differs'
        assert message==expected,message
        assert hashlib.sha256(file.read_bytes()).hexdigest()==before
        results.append(dict(test=label,status='rejected',error=message,packed_response_not_applied=True,processes_launched=0))
    (directory/'receipt.json').write_text(json.dumps(results,indent=2)+'\n')
    print(directory/'receipt.json')


if __name__=='__main__':
    main()
