#!/usr/bin/env python3
"""Default repository mapping and explicit capture set/unset precedence."""
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
scratch=Path(tempfile.mkdtemp(prefix='recon-mapping-test-',dir=Path.home()/'.ai-tmp'))
try:
    for label,env,expected in [('default',{},str(ROOT.parents[1])),('explicit',{'RECON_DRIVE_R':'/explicit-capture'},'/explicit-capture'),('unset',{'RECON_DRIVE_R':None},'unset')]:
        source=scratch/(label+'.json');capture=scratch/(label+'.bin')
        value=dict(env={'RECON_CHECK_REPO':'1',**env},stdout='text:R='+expected+'\n',stderr='text:',exit_code=0)
        if label!='default':value.update(logical_drive='R',logical_cwd='R:\\mapping')
        source.write_text(json.dumps(value))
        subprocess.run([TOOL,'capture','pack',source,capture],check=True)
        subprocess.run([TOOL,'run',FIXTURE/'cases/0004-event','--capture',capture],check=True)
    print('repository mapping and explicit precedence passed')
finally:
    shutil.rmtree(scratch)
