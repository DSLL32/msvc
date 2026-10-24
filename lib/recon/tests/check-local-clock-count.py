import os,subprocess,sys
for mode,expected in (([],0),(['extra'],42)):
 env=dict(os.environ)
 for key in ('RECON_LOCAL_CLOCK','RECON_FIXED_TIME','RECON_FIXED_MILLISECONDS'):env.pop(key,None)
 p=subprocess.run([sys.argv[1]]+mode,env=env,capture_output=True)
 assert p.returncode==expected and not p.stderr,(mode,p.returncode,p.stdout,p.stderr)
 if not mode:assert p.stdout==b'clock count read-only, sequence/reset/default/errors/FP pass\n'
print('configured-clock inventory0/1/2/3, host extra-call rejection and unchanged reset/default/FP/errors pass')
