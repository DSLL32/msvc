#!/usr/bin/env python3
"""Captured child policy: real C callback, protocol/status, counts, and cleanup."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
TOOL=Path(os.environ.get('RECON_TOOL_TEST_EXE',ROOT/'target/i686-unknown-linux-gnu/debug/recon-tool'))
FIXTURE=ROOT/'tests/fixture'
subprocess.run(['make','-C',str(FIXTURE),'-j4','cases'],check=True)
scratch=Path(tempfile.mkdtemp(prefix='recon-process-test-',dir=Path.home()/'.ai-tmp'))
try:
    for name,flags,passed in [('success',{},True),('missing-command',{'RECON_PROCESS_FAIL':'1'},True),('status-seven',{'RECON_PROCESS_SEVEN':'1'},True),('rsp2',{'RECON_PROCESS_RSP2':'1'},True),('rejected',{'RECON_PROCESS_BAD':'1'},False),('extra',{'RECON_PROCESS_EXTRA':'1'},False),('missing',{'RECON_PROCESS_MISSING':'1'},False),('timeout',{'RECON_PROCESS_HANG':'1'},False)]:
        value=dict(env={'RECON_CHECK_POLICY':'1',**flags},stdout='text:reply\n',stderr='text:diagnostic\n',exit_code=0,output_files=[dict(path='effect.txt',data='text:captured')],reject_unexpected_files=True,timeout_ms=100)
        source=scratch/(name+'.json');capture=scratch/(name+'.bin');report=scratch/(name+'-report.json')
        source.write_text(json.dumps(value))
        subprocess.run([TOOL,'capture','pack',source,capture],check=True)
        result=subprocess.run([TOOL,'run',FIXTURE/'cases/0008-process','--capture',capture,'--report',report],capture_output=True,text=True,timeout=3)
        data=json.loads(report.read_text())[0]
        assert (result.returncode==0)==passed,(name,result.stdout,result.stderr,data)
        assert not Path(data['workspace']).exists(),name+' leaked workspace'
        if passed:
            assert data['process_requests']['requests']==1 and len(data['process_requests']['pids'])==1
    subprocess.run([TOOL,'test',FIXTURE/'cases/0008-process'],check=True,timeout=3)
    print('captured process policy checks passed')
finally:
    shutil.rmtree(scratch)
