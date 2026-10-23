"""Native-only comparison with the explicit canonical provider capture."""
import hashlib,json,os,struct,subprocess,sys,tempfile
from pathlib import Path
reference=json.loads(Path(__file__).with_name('mb-strstr-original.json').read_text())
binary=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='recon-mb-strstr-') as work:
    result=subprocess.run([str(binary)],cwd=work,capture_output=True)
    assert result.returncode==0 and not result.stderr,(result.returncode,result.stderr)
    assert hashlib.sha256(result.stdout).hexdigest()==reference['stdout_sha256']
    data=(Path(work)/'states.bin').read_bytes();rows=reference['rows'];assert len(data)==rows*1244
    observations=bytearray()
    for index,line in enumerate(result.stdout.splitlines()):
        record=data[index*1244:(index+1)*1244]
        assert record[:256]==record[512:768] and record[256:512]==record[768:1024]
        assert record[1024:1132]==record[1132:1240],('x87 environment/registers',index)
        assert struct.unpack('<i',record[1240:])[0]==int(line.split()[2])
        observations.extend(record[:1024]+record[1240:])
    assert hashlib.sha256(observations).hexdigest()==reference['buffer_offsets_sha256']
    for mode in ('text','needle','null-text','null-needle','codepage','1252','locale','lc-codepage','max'):
        result=subprocess.run([str(binary),mode],cwd=work,capture_output=True)
        assert result.returncode==125,(mode,result.returncode,result.stderr)
print('556 canonical rows: complete inputs/borrowed offsets/errors/live x87; nine scope denials passed')
