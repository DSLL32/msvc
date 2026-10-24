"""One dlopened tracer, explicit App filter, and fork-shared first encounters."""
import json,os,pathlib,subprocess,sys,tempfile,time
module,driver=map(lambda p:str(pathlib.Path(p).resolve()),sys.argv[1:3])
env={k:v for k,v in os.environ.items()if not k.startswith('RECON_TRACE_')}
checks=[]
with tempfile.TemporaryDirectory(prefix='recon-trace-module-')as work:
 for mode in ('normal','address','label','newline','null-name','null-address','null-registry','large','fd','readonly','unconfigured'):
  first=pathlib.Path(work)/'first';second=pathlib.Path(work)/'second';start=time.monotonic()
  p=subprocess.run([driver,module,str(first),str(second),mode],env=env,capture_output=True)
  if mode=='normal':
   assert p.returncode==0 and b'full108FP PASS'in p.stdout and not p.stderr,(mode,p.returncode,p.stdout,p.stderr)
   expected=['app_alpha','_Z8cpp_leafi','_ZL6hiddeni','app_beta','app_beta','app_alpha','_Z8cpp_leafi','_ZL6hiddeni']
   assert first.read_text().splitlines()==expected,first.read_bytes()
   assert second.read_bytes()==b'app_alpha\n',second.read_bytes()
  else:
   assert p.returncode==126 and not p.stdout and b'recon trace:'in p.stderr,(mode,p.returncode,p.stdout,p.stderr)
   assert not first.read_bytes() and not second.read_bytes(),mode
  checks.append({'check':mode,'seconds':time.monotonic()-start})
print(json.dumps({'status':'PASS','checks':len(checks),'execution_check_seconds':sum(c['seconds']for c in checks)}))
