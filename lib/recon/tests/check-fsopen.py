import json,subprocess,sys,tempfile,time,hashlib
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
v=json.loads((Path(__file__).parent/'fsopen-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'fsopen-reference.c').read_bytes()).hexdigest()==v['source_sha256']
with tempfile.TemporaryDirectory() as tmp:
    root=Path(tmp); (root/'input.bin').write_bytes(bytes.fromhex(v['input_hex']))
    start=time.monotonic();p=subprocess.run([binary],cwd=tmp,capture_output=True)
    assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex']),(p.returncode,p.stdout,p.stderr)
    for name,digest in v['outputs'].items():
        assert (root/name).read_bytes()==bytes.fromhex(digest)
    for mode in ('alias','mode','share','dup','dup2','mapping','rawalias','prioralias','priormap'):
        p=subprocess.run([binary,mode],cwd=tmp,capture_output=True)
        # The exact tracked READ/share-READ prior handle is now a supported
        # compatible alias. All remaining controls retain their prior denial.
        expected=0 if mode=='prioralias' else 125
        assert p.returncode==expected,(mode,p.returncode,p.stderr)
        if not expected:
            assert not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex'])
    print('canonical binary fsopen streams/files/retained FILE fields exact; eight scope denials and tracked-read alias positive pass (%.3fs)'%(time.monotonic()-start))
