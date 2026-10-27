from pathlib import Path
import subprocess,sys,tempfile
selected,generic=[str(Path(p).resolve()) for p in sys.argv[1:]]
payload=bytes(range(256))*4
for binary,mode in [(selected,m) for m in ('','missing','parent','directory','symlink','parent-symlink','absolute','dotdot','mode','wildcard','nonascii')]+[(generic,'')]:
    with tempfile.TemporaryDirectory(prefix='watcom-open-rw-',dir=Path.home()/'.ai-tmp') as tmp:
        root=Path(tmp);(root/'folder').mkdir();file=root/'folder/binary.dat';file.write_bytes(payload)
        (root/'alias.dat').symlink_to('folder/binary.dat');(root/'alias-folder').symlink_to('folder')
        p=subprocess.run([binary]+([mode] if mode else []),cwd=tmp,capture_output=True)
        expected=0 if binary==selected and not mode else 125
        assert p.returncode==expected and not p.stdout,(mode,p.returncode,p.stdout,p.stderr)
        if not expected:
            assert not p.stderr
            assert file.read_bytes()==payload[:17]+bytes([13,10,26])+payload[20:]
        else:
            assert b'recon: unsupported native operation:' in p.stderr
            assert file.read_bytes()==payload
        assert not (root/'folder/absent.dat').exists()
print('existingRW binary two-argument opens, independent cursors, byte/tail ownership and retained denial domains pass')
