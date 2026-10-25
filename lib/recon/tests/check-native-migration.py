"""Focused exec-shared tracing and purpose-specific loaded-image ownership."""
import json,os,pathlib,subprocess,sys,tempfile,time
trace_lib,trace_driver,app,case,image_driver=map(lambda p:str(pathlib.Path(p).resolve()),sys.argv[1:])
env={k:v for k,v in os.environ.items()if not k.startswith('RECON_TRACE_')};checks=[]
with tempfile.TemporaryDirectory(prefix='recon-native-migration-')as root:
 for mode in ('normal','size','dirty','readonly','fd','hash','count'):
  trace=pathlib.Path(root)/'trace';state=pathlib.Path(root)/'state';start=time.monotonic()
  p=subprocess.run([trace_driver,trace_lib,str(trace),str(state),mode],env=env,capture_output=True)
  assert p.returncode==(0 if mode=='normal'else 126),(mode,p.returncode,p.stdout,p.stderr)
  if mode=='normal':assert trace.read_text().splitlines()==['app_alpha','_Z8cpp_leafi','_ZL6hiddeni','app_beta']
  else:assert b'recon trace:'in p.stderr and not trace.read_bytes(),(mode,p.stderr)
  checks.append(time.monotonic()-start)
 for mode in ('normal','case','foreign-data','heap','readonly-data','data-callback','unbound-case','unbound-app','overlap','role','order','heap-anchor','replace'):
  start=time.monotonic();p=subprocess.run([image_driver,app,case,mode],env=env,capture_output=True)
  assert p.returncode==(0 if mode in ('normal','case')else 125),(mode,p.returncode,p.stdout,p.stderr)
  if p.returncode:assert b'unsupported native operation'in p.stderr,(mode,p.stderr)
  checks.append(time.monotonic()-start)
print(json.dumps({'status':'PASS','focused_checks':len(checks),'execution_check_seconds':sum(checks)}))
