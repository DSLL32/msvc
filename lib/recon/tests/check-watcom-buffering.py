"""Real stdout buffering/file visibility and unrelated host-fd ownership."""
import pathlib,subprocess,sys,tempfile,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve())
generic=len(sys.argv)>2 and sys.argv[2]=='default'
expected=b'abc\r\n'+128*b'A'+b'B\r\ntailuser'
with tempfile.TemporaryDirectory() as cwd:
    for row in range(3):
        for mode in ('','live'):
            with tempfile.TemporaryFile() as output:
                p=subprocess.run([binary,str(row)]+([mode] if mode else []),cwd=cwd,stdout=output,stderr=subprocess.PIPE)
                output.seek(0);data=output.read()
            assert p.returncode==(125 if generic else 0),(row,mode,p.returncode,p.stderr)
            assert data==(b'' if generic else expected),(row,mode,data)
            assert b'Watcom' in p.stderr if generic else not p.stderr,(row,mode,p.stderr)
    for mode in ('mode','size','null','other','pending','device','wrap','repeat','changed'):
        with tempfile.TemporaryFile() as output:
            p=subprocess.run([binary,mode],cwd=cwd,stdout=output,stderr=subprocess.PIPE)
            output.seek(0);data=output.read()
        assert p.returncode==125 and not data and b'Watcom' in p.stderr,(mode,p.returncode,p.stderr,data)
print(('generic API remains denied' if generic else 'supplied128B alias/userownership/newline/capacity/explicit+closeflush/CRLF visibility,unrelatedhostfd3/4 survive,x3errors xempty/live108FP;9 denials')+' PASS %.6fs'%(time.monotonic()-start))
