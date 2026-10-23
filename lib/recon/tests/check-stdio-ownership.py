import json,subprocess,sys,tempfile
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
v=json.loads((Path(__file__).parent/'stdio-ownership-original.json').read_text())
with tempfile.TemporaryDirectory() as tmp:
    path=Path(tmp)
    with (path/'stdout.bin').open('wb') as out,(path/'stderr.bin').open('wb') as err:
        p=subprocess.run([binary],cwd=tmp,stdout=out,stderr=err)
    assert p.returncode==0,p.returncode
    assert not (path/'stdout.bin').read_bytes() and not (path/'stderr.bin').read_bytes()
    for name,key in [('report.txt','report_hex'),('listing.cod','listing_hex'),('manual.bin','manual_hex')]:
        assert (path/name).read_bytes()==bytes.fromhex(v[key]),(name,(path/name).read_bytes())
print('original close/alloc/set/free/reopen exact FILE/stdio ownership, errors and artifacts')
