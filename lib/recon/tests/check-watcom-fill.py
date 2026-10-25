"""Shared semantic fill; original register encoding remains period-owned."""
import subprocess,sys,time
start=time.monotonic();generic=len(sys.argv)>2 and sys.argv[2]=='default'
for mode in ('','live','null','wrap','count'):
    result=subprocess.run([sys.argv[1]]+([mode] if mode else []),capture_output=True)
    expected=125 if generic or mode in ('null','wrap','count') else 0
    assert result.returncode==expected and not result.stdout,(mode,result.returncode,result.stderr)
    assert b'Watcom DWORD fill' in result.stderr if expected else not result.stderr
print('unaligned patterns/counts/endptr/count0/tails/errors/full108FP/profile isolation/3 denials PASS %.6fs'%(time.monotonic()-start))
