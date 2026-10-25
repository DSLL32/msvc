"""Bounded leaf packing and prewrite overlap rejection, without filesystem IO."""
import subprocess,sys,time
start=time.monotonic();generic=len(sys.argv)>2 and sys.argv[2]=='default'
for mode in ('','live','denials'):
    result=subprocess.run([sys.argv[1]]+([mode] if mode else []),capture_output=True)
    expected=125 if generic and mode!='denials' else 0
    assert result.returncode==expected and not result.stdout,(mode,result.returncode,result.stderr)
    assert result.stderr if expected or mode=='denials' else not result.stderr
print('leaf/lastdot/16 selector masks/packed pointers/tails/errors/full108FP;10 prewrite denials PASS %.6fs'%(time.monotonic()-start))
