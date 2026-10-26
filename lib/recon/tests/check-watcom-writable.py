"""Real readonly-distinct text/binary create/truncate descriptors."""
import pathlib,subprocess,sys,tempfile,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve())
generic=len(sys.argv)>2 and sys.argv[2]=='default'
changed=len(sys.argv)>3 and sys.argv[3]=='changed'
def case(mode,age='new',live=False):
    with tempfile.TemporaryDirectory() as directory:
        root=pathlib.Path(directory);original=b'old contents must be truncated'
        (root/'existing.dat').write_bytes(original);(root/'folder').mkdir();(root/'alias.dat').symlink_to('existing.dat');(root/'aliasdir').symlink_to('folder');(root/'folder/sub').mkdir()
        for parent in ('folder','folder/sub'):(root/parent/'existing.dat').write_bytes(original)
        inventory=lambda:{str(path.relative_to(root)):('link',str(path.readlink())) if path.is_symlink() else ('file',path.read_bytes()) for path in root.rglob('*') if path.is_symlink() or path.is_file()}
        before=inventory()
        p=subprocess.run([binary,mode,str(root/'unqualified.dat') if mode=='absolute' else age]+(['live'] if live else []),cwd=root,capture_output=True)
        success=(mode in ('text','binary600','binary666','binaryzero') or mode=='exclusive' and age=='new') and not generic
        assert p.returncode==(0 if success else 125) and not p.stdout,(mode,age,p.returncode,p.stderr)
        if success:
            expected=b'A\r\nB' if mode=='text' else b'A\nB\0Z'
            assert (root/('folder/sub' if age.startswith('nested') else 'folder' if '-' in age else '')/('existing.dat' if age.endswith('old') else 'created.dat')).read_bytes()==expected
            assert not p.stderr,p.stderr
        else:
            assert b'Watcom' in p.stderr and not (root/'created.dat').exists(),(mode,p.stderr)
        if success:
            target=pathlib.Path('folder/sub' if age.startswith('nested') else 'folder' if '-' in age else '')/('existing.dat' if age.endswith('old') else 'created.dat')
            before[str(target)]=('file',expected)
        assert inventory()==before,(mode,age,inventory(),before)
if changed:
    for age in ('new','old'):
        for live in (False,True):case('binaryzero',age,live)
    for mode in ('binary600','binary666'):case(mode)
    for mode in ('textzero','exclusivezero','temporaryzero'):case(mode)
else:
    for mode in ('text','binary600','binary666','binaryzero'):
        for age in ('new','old','directory-new','directory-old','backslash-new','backslash-old','nested-new','nested-old'):
            for live in (False,True):case(mode,age,live)
    for age in ('new','old','directory-new','directory-old','backslash-new','backslash-old','nested-new','nested-old'):
        for live in (False,True):case('exclusive',age,live)
    for mode in ('exclusive600','exclusiveflags','linux','text600','permission','directory','symlink','path','trailing','null','parent-symlink','parent-file','traversal','dot','double','absolute','drive','unc','trailing-parent','trailing-slash','textzero','exclusivezero','temporaryzero'):case(mode)
print('exact flags/permissions incl0262 permission0 default0600;new+existing truncation/binary seek/read/write/close/privateerrno/semanticFP; otherzero tuples deny PASS %.6fs'%(time.monotonic()-start))
