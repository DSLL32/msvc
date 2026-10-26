import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='510'
for mode in ('normal','edge','prewrite','null','low','input-wrap','null-delimiters','low-delimiters','delimiter-wrap','other-delimiters','overlap','high','unterminated','mb','gate','width','lc','handle'):
 p=subprocess.run([binary,mode],capture_output=True);expected=0 if admitted and mode in ('normal','edge','prewrite') else 125
 assert p.returncode==expected and not p.stdout,(mode,p)
 assert bool(p.stderr)==bool(expected or mode=='prewrite'),(mode,p)
print('fresh ASCII token alias/singleNUL/fulltails/empty+spaces/4095B page edge/prewrite guard/native errors/fullFP;15 denied scopes/profiles PASS %.6fs'%(time.monotonic()-start))
