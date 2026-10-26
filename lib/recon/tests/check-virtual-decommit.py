"""Whole owned decommit retains a discarded inaccessible reservation until release."""
import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());generic=len(sys.argv)>2 and sys.argv[2]!='510'
for mode in (('',) if generic else ('','live','null','foreign','interior','partial','zero','reserve','topdown','readonly','repeat','recommit','released')):
    result=subprocess.run([binary]+([mode] if mode else []),capture_output=True)
    expected=0 if mode in ('','live') else 125
    assert result.returncode==expected and not result.stdout,(mode,result.returncode,result.stderr)
    assert bool(result.stderr)==bool(expected),(mode,result.stderr)
print('tracked whole decommit/PROT_NONE reservation/residency discard/release/independent owners/errors/full108FP/profile isolation;11 scope denials PASS %.6fs'%(time.monotonic()-start))
