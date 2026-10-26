"""Internal unique native names: absence, no creation and real cache lifecycle."""
import pathlib,subprocess,sys,tempfile,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());watcom=len(sys.argv)>2 and sys.argv[2]=='watcom'
for mode in ('','live','names','null','wrap','cwd','permission','flags','directory','existing'):
    with tempfile.TemporaryDirectory() as directory:
        root=pathlib.Path(directory);(root/'existing.dat').write_bytes(b'unchanged');(root/'folder').mkdir()
        result=subprocess.run([binary]+([mode] if mode else []),cwd=root,capture_output=True)
        expected=0 if watcom and mode in ('','live','names') else 125
        assert result.returncode==expected and not result.stdout,(mode,result.returncode,result.stderr)
        assert bool(result.stderr)==bool(expected),(mode,result.stderr)
        assert sorted(str(p.relative_to(root)) for p in root.rglob('*'))==['existing.dat','folder']
        assert (root/'existing.dat').read_bytes()==b'unchanged'
print('caller13B/alias/tails/real absence/no-create/16 unique create-write-reopen-unlink/current errors+FP;7 scope denials PASS %.6fs'%(time.monotonic()-start))
