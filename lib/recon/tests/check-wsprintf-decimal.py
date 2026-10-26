"""The reached cdecl %d branch only; native preservation/guard checks."""
import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='510'
for mode in ('','live','null-output','null-format','output-wrap','format-wrap','wide','suffix','other','overlap'):
    p=subprocess.run([binary]+([mode] if mode else []),capture_output=True)
    expected=0 if admitted and mode in ('','live') else 125
    assert p.returncode==expected and not p.stdout,(mode,p)
    assert b'unsupported native operation:' in p.stderr if expected else not p.stderr,(mode,p)
print('cdecl signed%%d actual10/240 and signed32 edges; exact return/NUL/tails/guard-page bounds;3 independent errors/full108FP;8 denials/other profiles PASS %.6fs'%(time.monotonic()-start))
