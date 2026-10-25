#!/usr/bin/env python3
"""Focused native runner routing and authorization checks, never Wine."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from spawn import SpawnBroker

executable=Path(sys.argv[1]).resolve()
root=Path.home()/'.ai-tmp/msvc2/recon-maintenance/self-command'
root.mkdir(parents=True,exist_ok=True)
work=Path(tempfile.mkdtemp(dir=root))
copy=work/'other-elf'
shutil.copyfile(executable,copy)
canonical=r'Z:\virtual\LINK.EXE'
checks=[]
def run(mode,code=0,broker=True,self_flag='1',output=b''):
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    env.update(RECON_PROGRAM_PATH=canonical,CHILD_MARKER='inherited',OTHER_ELF=str(copy))
    if self_flag is not None:env['RECON_SELF_SPAWN']=self_flag
    def response(request):
        assert request['mode']==0
        assert request['file']==(r'Z:\virtual\OTHER.EXE' if mode=='wrongself' else canonical)
        assert request['argv']==[canonical,'payload']
        assert 'CHILD_MARKER=inherited' in request['environment']
        assert request['cwd']==str(work)
        return 9 if mode=='replay' else None
    b=SpawnBroker(response) if broker else None
    if b:env.update(b.environment())
    start=time.perf_counter()
    try:
        p=subprocess.run([str(executable),mode],cwd=work,env=env,
                         pass_fds=(b.fileno(),) if b else (),capture_output=True,timeout=10)
    finally:
        if b:b.close()
    assert p.returncode==code,(mode,p.returncode,p.stdout,p.stderr)
    assert p.stdout==output,(mode,p.stdout)
    if code==0:
        assert not p.stderr,(mode,p.stderr)
        if b:b.check()
    else:assert b'native self' in p.stderr,(mode,p.stderr)
    checks.append(dict(mode=mode,broker=broker,self_flag=self_flag,seconds=time.perf_counter()-start))
run('route',output=b'routed\n')
run('route',125,broker=False)
run('wrongself')
run('disabled',self_flag=None)
run('wrongexe',125)
run('copyexe',125)
run('zero',125)
run('twice',125)
run('replay')
run('legacy',output=b'legacy\n')
run('legacy',broker=False,output=b'legacy\n')
receipt=dict(status='passed',checks=checks,seconds=sum(x['seconds'] for x in checks))
(work/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(dict(status='passed',checks=len(checks),seconds=receipt['seconds'],receipt=str(work/'receipt.json'))))
