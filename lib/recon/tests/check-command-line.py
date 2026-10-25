"""Exact mutable command-line alias and bounded old Watcom command tail."""
import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve())
generic=len(sys.argv)>2 and sys.argv[2]=='default'
for mode in ['alias']+[str(n) for n in range(8)]:
    for live in ([],['live']):
        result=subprocess.run([binary,mode]+live,capture_output=True)
        expected=125 if generic and mode!='alias' else 0
        assert result.returncode==expected and not result.stdout,(mode,live,result.returncode,result.stderr)
        assert not result.stderr if not expected else b'Watcom' in result.stderr
denials=('unbound','unboundcmd','nullbind','zero','large','wrap','embedded','nonascii','unterminated','leading','rebind','nullout','overlap','outwrap','mutated','missingnul')
for mode in denials:
    result=subprocess.run([binary,mode],capture_output=True)
    assert result.returncode==125 and not result.stdout and result.stderr,(mode,result.returncode,result.stderr)
print('exact alias/bounded tails/guard page/tails/errors/full108FP/profile isolation;16 denials PASS %.6fs'%(time.monotonic()-start))
