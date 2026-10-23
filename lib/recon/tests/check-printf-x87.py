import hashlib,json,os,subprocess,sys,tempfile,struct
from pathlib import Path
v=json.loads((Path(__file__).parent/'printf-x87-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'printf-x87-reference.c').read_bytes()).hexdigest()==v['source_sha256']
binary=str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as tmp:
    p=subprocess.run([binary],cwd=tmp,capture_output=True)
    assert p.returncode==0 and not p.stderr,(p.returncode,p.stderr)
    assert hashlib.sha256(p.stdout).hexdigest()==v['stdout_sha256'],p.stdout
    data=(Path(tmp)/'states.bin').read_bytes();assert len(data)==28*220
    for n in range(28):
        before=data[n*220:n*220+108];after=data[n*220+108:n*220+216]
        assert struct.unpack_from('<i',data,n*220+216)[0]==v['results'][n]
        assert before==after,(n,[i for i in range(108) if before[i]!=after[i]])
        assert struct.unpack_from('<H',before)[0]==(0x27f if n<14 else 0x37f)
    for mode in ('locale','codepage','control','nonfinite'):
        p=subprocess.run([binary],cwd=tmp,env=dict(os.environ,RECON_PRINTF_X87_NEGATIVE=mode),capture_output=True)
        assert p.returncode==125,(mode,p.returncode,p.stderr)
print('28 finite plain%g values/control/status/live-stack cases: original bytes/counts and all108 x87 bytes retained')
