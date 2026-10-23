import hashlib,json,subprocess,sys,tempfile
from pathlib import Path
v=json.loads((Path(__file__).parent/'ctype-default-reference.json').read_text())
for executable in sys.argv[1:]:
    with tempfile.TemporaryDirectory() as tmp:
        result=subprocess.run([str(Path(executable).resolve())],cwd=tmp,capture_output=True)
        assert result.returncode==0 and not result.stdout and not result.stderr,(executable,result.returncode,result.stderr)
        data=(Path(tmp)/'ctype.bin').read_bytes()
        assert len(data)==514 and hashlib.sha256(data).hexdigest()==v['table_sha256']
print('Full257 default-C ushort entries/EOF/staticpointer,131584 masks and independent errors match shipped static tables')
