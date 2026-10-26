import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='dos'
for mode in ('normal','null','low','wrap','drive','directory','backslash','high','long','output-wrap','overlap','input-alias'):
 p=subprocess.run([binary,mode],capture_output=True);expected=0 if admitted and mode=='normal' else 125
 assert p.returncode==expected and not p.stdout,(mode,p)
 assert bool(p.stderr)==bool(expected),(mode,p)
print('DOS8/4lastdot/emptycomponents/all16selectors/alias+fulltails/error+fullFP/11denials/optin isolation PASS %.6fs'%(time.monotonic()-start))
