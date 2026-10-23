import hashlib,json,os,subprocess,sys,tempfile
from pathlib import Path
v=json.loads((Path(__file__).parent/'wcstol-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'wcstol-reference.c').read_bytes()).hexdigest()==v['source_sha256']
binary=str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as tmp:
    p=subprocess.run([binary],cwd=tmp,capture_output=True)
    assert p.returncode==0 and not p.stderr and len(p.stdout)==v['stdout_bytes'] and hashlib.sha256(p.stdout).hexdigest()==v['stdout_sha256'],(p.returncode,p.stderr)
    data=(Path(tmp)/'wcstol-inputs.bin').read_bytes()
    assert len(data)==v['input_bytes'] and hashlib.sha256(data).hexdigest()==v['input_sha256']
    for mode in ('nonascii','end','base','locale','codepage'):
        p=subprocess.run([binary],cwd=tmp,env=dict(os.environ,RECON_WCSTOL_NEGATIVE=mode),capture_output=True)
        assert p.returncode==125,(mode,p.returncode,p.stderr)
print('52 original UTF16 decimal/sign/space/trailing/overflow/error rows and allinput/tail bytes exact;5 unknown scopes denied')
