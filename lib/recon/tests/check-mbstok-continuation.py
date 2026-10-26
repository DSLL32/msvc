import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='510'
for mode in ('normal','unbound','changed-nul','changed-end','high','delimiter','overlap','mb','gate','width','locale'):
 p=subprocess.run([binary,mode],capture_output=True);expected=0 if admitted and mode=='normal' else 125
 assert p.returncode==expected and not p.stdout,(mode,p)
 assert bool(p.stderr)==bool(expected),(mode,p)
print('live same-thread token sequence/aliases/singlewrites/fulltails/reset/exhaustion/nativeerrors/fullFP;unbound+changedextent+unknownstate10denials PASS %.6fs'%(time.monotonic()-start))
