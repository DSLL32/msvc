import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='510'
for mode in ('normal','boundary','prewrite','pattern','marker','null','output-wrap','unaligned','overlap-format','overlap-name','null-name','name-wrap','high','unterminated','capacity','mb','gate','width','locale','handle'):
 p=subprocess.run([binary,mode],capture_output=True);expected=0 if admitted and mode in ('normal','boundary','prewrite') else 125
 assert p.returncode==expected and not p.stdout,(mode,p)
 assert bool(p.stderr)==bool(expected or mode=='prewrite'),(mode,p)
print('exact6argument reference layout/literalspace+quotes/E001+E002WCHARmarkers/signed32edges/count+NUL/fulltails/2047boundary/prewrite/nativeerrors/fullFP;17denieddomains/profiles PASS %.6fs'%(time.monotonic()-start))
