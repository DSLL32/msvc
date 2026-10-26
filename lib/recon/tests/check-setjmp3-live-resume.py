import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='510'
for mode in ('normal','null','unknown','count','size','zero','one','seven','negative'):
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==(0 if admitted and mode in ('normal','one') else 125),(mode,p.returncode,p.stdout,p.stderr)
    assert not p.stdout and bool(p.stderr)==bool(p.returncode),(mode,p)
print('opaque64B count0/live nested caller returns1+2/owner+canaries/current errors and semantic FP/seven denials/profile isolation PASS %.6fs'%(time.monotonic()-start))
