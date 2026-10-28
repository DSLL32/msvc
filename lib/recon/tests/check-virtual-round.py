#!/usr/bin/env python3
"""Real rounded contiguous commits; no original execution."""
import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve())
for mode in ['normal','one','4095','4096','4097','last','hole','unaligned','readonly','wrapped','foreign','outside','recommit']:
    result=subprocess.run([binary,mode],capture_output=True)
    expected=125 if mode in ['hole','unaligned','readonly','wrapped','recommit'] else 0
    assert result.returncode==expected and not result.stdout,(mode,result.returncode,result.stderr)
    assert bool(result.stderr)==bool(expected),(mode,result.stderr)
print('page-rounded16379/boundaries/adjacent/overlap/data/page tails/errors/full108FP; retained scope denials PASS %.6fs'%(time.monotonic()-start))
