import json,subprocess,sys,tempfile
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
v=json.loads((Path(__file__).parent/'file-time-original.json').read_text())
with tempfile.TemporaryDirectory() as tmp:
    p=subprocess.run([binary],cwd=tmp,capture_output=True)
    assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex']),(p.returncode,p.stdout,p.stderr)
    assert (Path(tmp)/'clock.dat').stat().st_mtime_ns==v['artifact_mtime_ns']
    for mode in ['invalid','create']:
        p=subprocess.run([binary,mode],cwd=tmp,capture_output=True)
        assert p.returncode==125,(mode,p.returncode,p.stderr)
print('original UTC/FILETIME values/errors/actual filesystemmtime exact;2unsupported controls passed')
