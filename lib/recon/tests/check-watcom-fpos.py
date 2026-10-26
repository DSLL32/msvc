"""Four-byte positions, normal save/rewind and unsupported failure domains."""
import pathlib,subprocess,sys,tempfile,time
start=time.monotonic();generic=len(sys.argv)>2 and sys.argv[2]=='default'
with tempfile.TemporaryDirectory() as directory:
    path=pathlib.Path(directory,'input.dat');payload=b'ABCDEFGHIJ\r\n';path.write_bytes(payload)
    for mode in ('','live','text','null','stream','wrap','unaligned','alias','buffer','negative','closed','text-buffered'):
        p=subprocess.run([str(pathlib.Path(sys.argv[1]).resolve())]+([mode] if mode else []),cwd=directory,capture_output=True)
        expected=0 if not generic and mode in ('','live','text') else 125
        assert p.returncode==expected and not p.stdout,(mode,p.returncode,p.stderr)
        assert b'Watcom' in p.stderr if expected else not p.stderr
        assert path.read_bytes()==payload
print('fpos4B typing/save/rewind/canaries/full108FP/profile isolation;9 denials PASS %.6fs'%(time.monotonic()-start))
