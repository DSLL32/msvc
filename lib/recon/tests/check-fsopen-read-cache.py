from pathlib import Path
import subprocess,sys,tempfile
selected,generic=[str(Path(p).resolve()) for p in sys.argv[1:]]
positive=('','view-only','mapping','lease-only')
negative=('share-none','share-rw','writer','unknown','reuse','identity','replace','unknown-view','write-mode','second-lease','late-writer')
for binary,mode,expected in [(selected,m,0) for m in positive]+[(selected,m,125) for m in negative]+[(generic,'',125)]:
    with tempfile.TemporaryDirectory(prefix='fsopen-read-cache-',dir=Path.home()/'.ai-tmp') as tmp:
        root=Path(tmp);data=bytes(range(256))*16;(root/'input.bin').write_bytes(data);(root/'other.bin').write_bytes(b'other inode')
        p=subprocess.run([binary]+([mode] if mode else []),cwd=tmp,capture_output=True)
        assert p.returncode==expected and not p.stdout,(binary,mode,p.returncode,p.stdout,p.stderr)
        if expected:assert b'recon: unsupported native operation:' in p.stderr
        else:assert not p.stderr
        assert (root/'input.bin').read_bytes()==data
print('tracked shared-read cache/view lease, closed mapping lifetime, fd retirement/reuse and writer/unknown denials pass')
