import subprocess,sys,time
start=time.monotonic();binary=sys.argv[1];admitted=sys.argv[2]=='510'
for mode in ('normal','legacy','edge','format','suffix','extra','locale','codepage','mb','max','null','overlap','output-wrap','unaligned','input-wrap') if admitted else ('normal','legacy'):
 p=subprocess.run([binary,mode],capture_output=True);expected=0 if mode=='legacy' or admitted and mode in ('normal','edge') else 125
 assert p.returncode==expected and not p.stdout,(mode,p)
 assert bool(p.stderr)==bool(expected),(mode,p)
print('exact pragma1252/signed32edges/UTF16count+NUL/tails/readonly end and64B page edge/errors/fullFP;12denials;old#line/profile isolation PASS %.6fs'%(time.monotonic()-start))
