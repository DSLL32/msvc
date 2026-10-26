import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='watcom'
for mode in ('normal','null','low','wrap'):
 p=subprocess.run([binary,mode],capture_output=True);expected=0 if admitted and mode=='normal' else 125
 assert p.returncode==expected and not p.stdout,(mode,p)
 assert bool(p.stderr)==bool(expected),(mode,p)
print('unsignedbyte65536pairs/ASCIIonlyfold/fullsigneddifference/embeddedNUL/count0noread/readonlyedge/nativeerrors+fullFP/span/profiledenials PASS %.6fs'%(time.monotonic()-start))
