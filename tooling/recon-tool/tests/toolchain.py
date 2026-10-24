#!/usr/bin/env python3
"""Shared launcher checks with a stand-in Wine; no Windows process is started."""
import json
import os
from pathlib import Path
import signal
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[1]
TOOL = ROOT/'target/i686-unknown-linux-gnu/debug/recon-tool'
with tempfile.TemporaryDirectory(prefix='recon-toolchain-', dir=Path.home()/'.ai-tmp') as temporary:
    scratch = Path(temporary)
    wine = scratch/'wine'
    wine.write_text('''#!/usr/bin/env python3
import json,os,signal,sys
from pathlib import Path
keys=['WINEPREFIX','WINEARCH','WINEDEBUG','WINEPATH','INCLUDE','LIB','CL','_CL_',
      'WINEDLLOVERRIDES','WATCOM','WCC','WCC386','WLINK','WLINK_LNK']
Path(os.environ['LAUNCH_RECORD']).write_text(json.dumps(dict(
    args=sys.argv[1:],cwd=os.getcwd(),env={key:os.environ.get(key) for key in keys})))
print('tool stdout',flush=True)
print('tool stderr',file=sys.stderr)
if os.environ.get('LAUNCH_SIGNAL'):os.kill(os.getpid(),signal.SIGTERM)
sys.exit(int(os.environ.get('LAUNCH_STATUS','0')))
''')
    wine.chmod(0o755)
    record = scratch/'record.json'
    env = dict(os.environ, PATH=str(scratch)+os.pathsep+os.environ['PATH'],
               LAUNCH_RECORD=str(record), CL='host flags', _CL_='host flags',
               INCLUDE='host include', LIB='host lib', WCC='host flags', WCC386='host flags',
               WLINK='host flags', WLINK_LNK='host flags')

    def run(chain, tool, arguments=(), cwd=REPO, **extra_env):
        result = subprocess.run([TOOL,'original','tool',chain,tool,'--',*arguments],
                                cwd=cwd,env=dict(env,**extra_env),capture_output=True,text=True)
        assert result.stdout == 'tool stdout\n' and result.stderr == 'tool stderr\n', result
        return result, json.loads(record.read_text())

    args = ['/nologo', '/DVALUE=a b', 'café.c', '@response file', '--literal']
    result, got = run('1.0-nt', 'cl', args, cwd=REPO/'decomp/vc1.0/c1xx')
    assert result.returncode == 0 and got['args'] == [r'R:\toolchains\1.0-nt\bin\CL.EXE',*args]
    assert got['cwd'] == str(REPO/'decomp/vc1.0/c1xx')
    assert got['env']['INCLUDE'] == r'R:\toolchains\1.0-nt\include'
    assert got['env']['LIB'] == r'R:\toolchains\1.0-nt\lib'
    assert got['env']['CL'] is None and got['env']['_CL_'] is None
    assert got['env']['WINEPREFIX'] == str(Path.home()/'.ai-tmp/vc1.0-capture/wine')
    assert got['env']['WINEARCH'] == 'win32' and got['env']['WINEDEBUG'] == '-all'
    # Repository inference also works from scratch, using the executable location.
    result, got = run('5.10','LINK.EXE',['/?'],cwd=scratch,LAUNCH_STATUS='17')
    assert result.returncode == 17 and got['args'][0] == r'R:\toolchains\5.10\bin\LINK.EXE'
    assert got['env']['WINEDLLOVERRIDES'] == 'msvcrt=n'
    assert got['env']['WINEPATH'] == r'R:\toolchains\5.10\bin'
    result, got = run('watcom10.0a','WCC386')
    assert got['args'][0] == r'R:\toolchains\watcom10.0a\binnt\WCC386.EXE'
    assert got['env']['WATCOM'] == r'R:\toolchains\watcom10.0a'
    assert all(got['env'][key] is None for key in ['INCLUDE','LIB','WCC','WCC386','WLINK','WLINK_LNK'])
    result, got = run('watcom10.0a','WLINK')
    assert got['args'][0] == r'R:\toolchains\watcom10.0a\.patched\WLINK.EXE'
    result, _ = run('1.0-nt','LINK',LAUNCH_SIGNAL='1')
    assert result.returncode == -signal.SIGTERM
    before = record.read_bytes()
    for chain, tool in [('unknown','CL'),('1.0-nt','../CL.EXE'),('1.0-nt','MISSING.EXE')]:
        result = subprocess.run([TOOL,'original','tool',chain,tool],env=env,capture_output=True,text=True)
        assert result.returncode != 0 and record.read_bytes() == before
print('shared toolchain launcher: profiles, inference, exact arguments/streams/status/signals PASS')
