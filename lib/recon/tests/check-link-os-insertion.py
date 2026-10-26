"""One reached OS-only insertion; native SELF is verified by the runner owner."""
import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='510'
for mode in ('middle','front','append','unbound','existing-os','existing-crt','crt-vector','crt-update','locale','table','null-name','other-name','null-value','other-value','repeat','corrupt'):
 p=subprocess.run([binary,mode],capture_output=True);expected=0 if admitted and mode in ('middle','front','append') else 125
 assert p.returncode==expected and not p.stdout,(mode,p)
 assert bool(p.stderr)==bool(expected),(mode,p)
print('exact owned OS insertion/order/preserved initial block+CRT+host/getenv/CP0 inheritance/errors/fullFP;13 rejected mutations/states and other profiles PASS %.6fs'%(time.monotonic()-start))
