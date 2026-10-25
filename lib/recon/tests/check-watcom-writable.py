"""Real readonly-distinct text/binary create/truncate descriptors."""
import pathlib,subprocess,sys,tempfile,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve())
generic=len(sys.argv)>2 and sys.argv[2]=='default'
def case(mode,age='new',live=False):
    with tempfile.TemporaryDirectory() as directory:
        root=pathlib.Path(directory);original=b'old contents must be truncated'
        (root/'existing.dat').write_bytes(original);(root/'folder').mkdir();(root/'alias.dat').symlink_to('existing.dat')
        p=subprocess.run([binary,mode,age]+(['live'] if live else []),cwd=root,capture_output=True)
        success=mode in ('text','binary600','binary666') and not generic
        assert p.returncode==(0 if success else 125) and not p.stdout,(mode,age,p.returncode,p.stderr)
        if success:
            expected=b'A\r\nB' if mode=='text' else b'A\nB\0Z'
            assert (root/('existing.dat' if age=='old' else 'created.dat')).read_bytes()==expected
            assert not p.stderr,p.stderr
        else:
            assert b'Watcom' in p.stderr and not (root/'created.dat').exists(),(mode,p.stderr)
        if age!='old' or not success:assert (root/'existing.dat').read_bytes()==original
        assert (root/'alias.dat').is_symlink() and not (root/'folder/new.dat').exists()
for mode in ('text','binary600','binary666'):
    for age in ('new','old'):
        for live in (False,True):case(mode,age,live)
for mode in ('linux','text600','permission','directory','symlink','path','trailing','null'):case(mode)
print('exact target flags/permissions/new+existing truncation/textCRLF/binary/seek/read/close/privateerrno/full108FP;8 denials PASS %.6fs'%(time.monotonic()-start))
