import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='510'
for mode in ('normal','boundary','prewrite','pattern','marker','flag','null','output-wrap','unaligned','overlap-format','overlap-name','null-name','name-wrap','high','unterminated','capacity','mb','gate','width','locale','handle'):
 p=subprocess.run([binary,mode],capture_output=True);expected=0 if admitted and mode in ('normal','boundary','prewrite') else 125
 assert p.returncode==expected and not p.stdout,(mode,p)
 assert bool(p.stderr)==bool(expected or mode=='prewrite'),(mode,p)
print('exact9argument layout/WCHARmarkers+low16signedflag/signed32edges/count+NUL/fulltails/2047boundary/prewrite/nativeerrors/fullFP;18denieddomains/profiles PASS %.6fs'%(time.monotonic()-start))
