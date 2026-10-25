#!/usr/bin/env python3
"""Post-exit checker waits for all descendants and preserves raw output."""
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
scratch=Path(tempfile.mkdtemp(prefix='recon-output-test-',dir=Path.home()/'.ai-tmp'))
try:
    for name,flag,passed in [('complete','RECON_OUTPUT_DELAY',True),('callback-false','RECON_OUTPUT_FALSE',False),('raw-mutation','RECON_OUTPUT_RAW',False),('checker-timeout','RECON_OUTPUT_HANG',False)]:
        env={'RECON_CHECK_OUTPUT':'1'}
        if flag:env[flag]='1'
        value=dict(env=env,stdout='text:reply\npid:EXPECTED\n',stderr='text:diagnostic\n',exit_code=0,output_files=[dict(path='effect.txt',data='text:captured'),dict(path='descendant.txt',data='text:done')],reject_unexpected_files=True,timeout_ms=300)
        source=scratch/(name+'.json');capture=scratch/(name+'.bin');report=scratch/(name+'-report.json')
        source.write_text(json.dumps(value))
        subprocess.run([TOOL,'capture','pack',source,capture],check=True)
        result=subprocess.run([TOOL,'run',FIXTURE/'cases/0009-output','--capture',capture,'--report',report],capture_output=True,text=True,timeout=3)
        data=json.loads(report.read_text())[0]
        assert (result.returncode==0)==passed,(name,result.stdout,result.stderr,data)
        assert not Path(data['workspace']).exists(),name+' leaked workspace'
        if passed:
            assert data['checks_ms']>=25 and data['execution_ms']>=15,data
    print('post-exit output lifecycle and raw stream checks passed')
finally:
    shutil.rmtree(scratch)
