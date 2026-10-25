#!/usr/bin/env python3
"""Target errno survives host metadata/setup checkpoints and return bookkeeping."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
TOOL=Path(os.environ.get('RECON_TOOL_TEST_EXE',ROOT/'target/i686-unknown-linux-gnu/debug/recon-tool'))
for fixture in ['native-errno','native-self']:
    subprocess.run(['make','-C',str(ROOT/'tests'/fixture),'-j4','cases'],check=True)
scratch=Path(tempfile.mkdtemp(prefix='recon-errno-',dir=Path.home()/'.ai-tmp'))
try:
    def run(name,case,value):
        source=scratch/(name+'.json');capture=scratch/(name+'.bin');report=scratch/(name+'-report.json')
        source.write_text(json.dumps(value))
        subprocess.run([TOOL,'capture','pack',source,capture],check=True)
        result=subprocess.run([TOOL,'run',case,'--capture',capture,'--report',report],capture_output=True,text=True,timeout=5)
        data=json.loads(report.read_text())[0]
        assert result.returncode==0 and data['pass'],(name,result.stdout,result.stderr,data)
        assert not Path(data['workspace']).exists(),data
        print(f'{name}: pass ({data["elapsed_ms"]:.1f}ms; setup {data["setup_ms"]:.1f}ms, '
              f'execution {data["execution_ms"]:.1f}ms, checks {data["checks_ms"]:.1f}ms)')
    cases=ROOT/'tests/native-errno/cases'
    for name,case,env in [('setup-changes','0000-io',{}),('setup-preserves','0000-io',{'ERRNO_KEEP':'1'}),('direct','0001-direct',{}),('no-setup','0002-no-setup',{})]:
        run(name,cases/case,dict(env=env,stdout='text:',stderr='text:',exit_code=0,
            output_files=[dict(path='errno.txt',data='text:99')],reject_unexpected_files=True))
    subprocess.run([TOOL,'test',cases/'0000-io'],check=True,timeout=5)
    print('forked and sequential unforked unit errno: pass')
    run('self-child',ROOT/'tests/native-self/cases/0000-self',dict(
        args=['T:\\self.exe'],stdin='text:payload\ntail\n',
        stdout='text:parent\nchild\nerrno-exit:99\nparent-status:0\nerrno-exit:99\n',
        stderr='text:child diagnostic\n',exit_code=0,reject_unexpected_files=True,
        logical_cwd='S:\\native-self',assets=[dict(path='marker',data='text:A')],
        env={'RECON_SELF_SPAWN':'1','RECON_PROGRAM_PATH':'T:\\self.exe','SELF_MARKER':'inherited','SELF_ERRNO':'1'}))
    print('native errno seam checks passed')
finally:
    shutil.rmtree(scratch)
