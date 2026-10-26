"""Live caller-frame resume1 without private context or x87 serialization."""
import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());generic=len(sys.argv)>2 and sys.argv[2]=='default'
for mode in ('','null','unknown','foreign','opaque','zero','other','generic'):
    result=subprocess.run([binary]+([mode] if mode else []),capture_output=True)
    expected=0 if not mode and not generic else 125
    assert result.returncode==expected and not result.stdout,(mode,result.returncode,result.stderr)
    assert bool(result.stderr)==bool(expected),(mode,result.stderr)
print('live caller-frame resume1/opaque52B+guards/current full108FP/errors;7 denials PASS %.6fs'%(time.monotonic()-start))
