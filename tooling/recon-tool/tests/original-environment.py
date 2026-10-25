#!/usr/bin/env python3
"""Original launch namespace tests with a stand-in Wine; no Windows process runs."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
REPO=Path(os.environ.get('RECON_TOOL_TEST_REPO',ROOT.parents[1]))
TOOL=Path(os.environ.get('RECON_TOOL_TEST_EXE',ROOT/'target/i686-unknown-linux-gnu/debug/recon-tool'))
shared=Path.home()/'.ai-tmp/msvc2'
shared.mkdir(parents=True,exist_ok=True)
scratch=Path(tempfile.mkdtemp(prefix='recon-original-env-',dir=shared))
try:
    probe=scratch/'environment-test'
    subprocess.run(['gcc','-m32','-std=c89','-pedantic','-Wall','-Wextra','-Werror',ROOT/'tests/original-environment.c','-o',probe],check=True)
    subprocess.run([probe],check=True)
    # Use a shipped PE only as a read-only identity input. The stand-in intercepts
    # Wine before a Windows broker or target can run; no PE is created or modified.
    program=REPO/'toolchains/1.0-nt/bin/CVPACK.EXE'
    data=program.read_bytes();pe=struct.unpack_from('<I',data,0x3c)[0]
    entry=struct.unpack_from('<I',data,pe+24+16)[0]
    case=scratch/'case.so';source=scratch/'case.c'
    source.write_text('#include "recon-tool.h"\nstatic int io(int argc,char **argv){return 0;}\n'
        +f'const ReconBinary binary_descriptor={{RECON_ABI_VERSION,sizeof(ReconBinary),"env-host-test","toolchains/1.0-nt/bin/CVPACK.EXE","{hashlib.sha256(data).hexdigest()}",{entry},0,0,0,0}};\n'
        +'const ReconCase case_descriptor={RECON_ABI_VERSION,sizeof(ReconCase),"env-host-test","host launch test",0,0,0,0,RECON_IO,RECON_EXIT,0,0,io,0,0,0};\n')
    subprocess.run(['gcc','-m32','-shared','-fPIC','-I'+str(ROOT/'include'),source,'-o',case],check=True)
    helper=scratch/'unused-helper.exe';helper.write_bytes(b'never executed')
    wine=scratch/'wine';record=scratch/'record.json'
    wine.write_text('''#!/usr/bin/python3
import json,os,sys
from pathlib import Path
path=sys.argv[-1]
assert path.startswith('T:\\\\'),path
patch=Path('/tmp/recon')/path[3:].replace('\\\\','/')
keys=['PATH','WINEPREFIX','WINEARCH','WINEDEBUG','MARKER_ENV','EMPTY','REMOVE']
Path(os.environ['LAUNCH_RECORD']).write_text(json.dumps(dict(exe=sys.argv[0],env={k:os.environ.get(k) for k in keys},overlay=patch.read_bytes().hex())))
sys.exit(91)
''')
    wine.chmod(0o755)
    host=dict(os.environ,PATH=str(scratch)+os.pathsep+os.environ['PATH'],LAUNCH_RECORD=str(record),
              WINEPREFIX='/must-not-be-used',WINEARCH='host-input',WINEDEBUG='host-input',
              MARKER_ENV='host-value',EMPTY='host-empty',REMOVE='host-remove')
    overlay={'PATH':'','WINEPREFIX':None,'WINEDEBUG':None,'WINEARCH':'target-arch',
             'MARKER_ENV':'target-only','EMPTY':'','REMOVE':None}
    def invoke(values):
        author=scratch/'input.json';capture=scratch/'input.bin'
        author.write_text(json.dumps(dict(env=values)))
        subprocess.run([TOOL,'capture','pack',author,capture],check=True)
        return subprocess.run([TOOL,'original','capture',case,'--repo',REPO,'--helper',helper,
            '--capture',capture,'--output',scratch/'never-captured.bin'],env=host,capture_output=True,text=True,timeout=5)
    result=invoke(overlay)
    assert result.returncode!=0 and record.exists(),(result.stdout,result.stderr)
    got=json.loads(record.read_text())
    assert Path(got['exe']).is_absolute() and Path(got['exe'])==wine,got
    assert got['env']==dict(PATH=host['PATH'],WINEPREFIX=str(Path.home()/'.ai-tmp/vc1.0-capture/wine'),
        WINEARCH='win32',WINEDEBUG='-all',MARKER_ENV='host-value',EMPTY='host-empty',REMOVE='host-remove'),got
    raw=bytes.fromhex(got['overlay']);assert raw.endswith(b'\0\0')
    entries=raw[:-1].split(b'\0')[:-1]
    expected=[(key+('='+value if value is not None else '')).encode('ascii') for key,value in sorted(overlay.items())]
    assert entries==expected,(entries,expected)
    before=record.read_bytes()
    for bad in [dict(PATH='',path=None),{'BAD':'nonascii-é'},{'BIG':'x'*32767}]:
        result=invoke(bad)
        assert result.returncode!=0 and record.read_bytes()==before,(result.stdout,result.stderr)
    assert not (scratch/'never-captured.bin').exists()
    print('original host launch: absolute executable, fixed prefix, host PATH, exact target overlay PASS')
finally:
    shutil.rmtree(scratch)
