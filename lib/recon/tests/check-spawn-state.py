import copy,os,socket,struct,subprocess,sys,threading,tempfile
from pathlib import Path
recon=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(recon))
from spawn import SpawnBroker
binary=str(Path(sys.argv[1]).resolve());reference=str(recon/'tests/locale-profile-reference')
cp0=len(sys.argv)>2 and sys.argv[2]=='cp0'
last=0 if cp0 else 2
base={**os.environ,'RECON_SPAWN_STATE_PROFILE':'msvcrt510-wait-success-v1','RECON_HOST_SECRET':'host-only'}
if cp0:base['RECON_SPAWN_STATE_PROFILE']='msvcrt510-cp0-wait-success-v1'
fp_directory=Path(tempfile.mkdtemp(prefix='spawn-state-fp-',dir=Path.home()/'.ai-tmp'))
print('raw native FP images:',fp_directory)
state={'errno':0,'doserrno':2,'last_error':last}
def response():return dict(status=0,stdout=b'wire\r\n',stderr=b'error\r\n',parent_state=dict(before=dict(state),after=dict(errno=0,doserrno=0,last_error=last)))
def inspect(request,kind='positive'):
 assert request['mode']==0 and request['file']=='C:\\captured\\child.exe' and request['argv']==[request['file']]
 if kind=='unbound':assert 'RECON_HOST_SECRET=host-only' in request['environment']
 elif kind=='rsp1-explicit':assert request['environment']==['EXPLICIT=only']
 else:assert request['environment']==['PATH=OS_ONLY','VALUE=os-value']
 if kind=='positive':assert request['parent_state']==state and request['environment_source']=='target-os'
 else:assert 'parent_state' not in request and 'environment_source' not in request
for mode in ('positive','rsp1-os','rsp1-explicit','unbound'):
 def callback(request):
  inspect(request,mode)
  return response() if mode=='positive' else dict(status=0,stdout=b'wire\r\n',stderr=b'error\r\n')
 broker=SpawnBroker(callback);env={**base,**broker.environment()}
 env['RECON_TEST_FP_RECORD']=str(fp_directory/(mode+'.bin'))
 if mode!='positive':env.pop('RECON_SPAWN_STATE_PROFILE')
 p=subprocess.run([binary,reference,mode],env=env,pass_fds=(broker.fileno(),),capture_output=True,timeout=10);broker.close();broker.check()
 assert p.returncode==0 and p.stdout==b'wire\r\n' and p.stderr==b'error\r\n',(mode,p.returncode,p.stdout,p.stderr)
 assert len(broker.requests)==1
print('four positives passed')
for mode in ('errno','dos','last','locale','missing-os','nonascii-os','early-os-nul','mode','explicit','unbound') + (('mb-table','ctype-table','ctype-eof','pctype','mbcp','lccp','mbmax','handle','updates') if cp0 else ()):
 broker=SpawnBroker(lambda _:response());p=subprocess.run([binary,reference,mode],env={**base,**broker.environment()},pass_fds=(broker.fileno(),),capture_output=True,timeout=10);broker.close()
 assert p.returncode==125 and not broker.requests,(mode,p.returncode,p.stderr,broker.requests)
for mutation in ('status','bool-status','bare','missing','extra','before','after','extra-state','bool-error','stream'):
 def callback(request):
  inspect(request);r=response()
  if mutation=='status':r['status']=1
  if mutation=='bool-status':r['status']=False
  if mutation=='bare':return 0
  if mutation=='missing':del r['parent_state']
  if mutation=='extra':r['unknown']=1
  if mutation=='before':r['parent_state']['before']['errno']=1
  if mutation=='after':r['parent_state']['after']['last_error']=2 if cp0 else 0
  if mutation=='extra-state':r['parent_state']['after']['other']=1
  if mutation=='bool-error':r['parent_state']['after']['errno']=False
  if mutation=='stream':r['stdout']='wrong'
  return r
 broker=SpawnBroker(callback);p=subprocess.run([binary,reference,'positive'],env={**base,**broker.environment()},pass_fds=(broker.fileno(),),capture_output=True,timeout=10);broker.close()
 assert p.returncode==125 and broker.errors,(mutation,p.returncode,p.stderr,broker.errors)
print(f'{29 if cp0 else 20} native/schema negatives passed')
def read(sock,n):
 data=b''
 while len(data)<n:
  part=sock.recv(n-len(data));assert part;data+=part
 return data
for mutation in ('magic','action','status','before','after','stream-size'):
 server,target=socket.socketpair();errors=[]
 def worker():
  try:
   magic,size=struct.unpack('<4sI',read(server,8));assert magic==b'RSP2'
   request=SpawnBroker._decode(read(server,size),True);inspect(request)
   effect=[0,2,last,0,0,last]
   if mutation=='before':effect[0]=1
   if mutation=='after':effect[5]=2 if cp0 else 0
   data=struct.pack('<4sii',b'RSP1' if mutation=='magic' else magic,3 if mutation=='action' else 5,1 if mutation=='status' else 0)
   data+=struct.pack('<6I',*effect)+struct.pack('<II',0xffffffff if mutation=='stream-size' else 0,0)
   server.sendall(data)
  except Exception as error:errors.append(str(error))
  finally:server.close()
 thread=threading.Thread(target=worker);thread.start()
 p=subprocess.run([binary,reference,'positive'],env={**base,'RECON_SPAWN_FD':str(target.fileno())},pass_fds=(target.fileno(),),capture_output=True,timeout=10);target.close();thread.join(5)
 assert not thread.is_alive() and not errors and p.returncode==125,(mutation,p.returncode,p.stderr,errors)
print('six native wire negatives passed')

for name in ('missing-broker','unknown-profile'):
 env=dict(base)
 if name=='missing-broker':env.pop('RECON_SPAWN_FD',None)
 else:env['RECON_SPAWN_STATE_PROFILE']='unknown'
 p=subprocess.run([binary,reference,'positive'],env=env,capture_output=True,timeout=10)
 assert p.returncode==125,(name,p.returncode,p.stderr)
print('missing broker/unknown profile denials passed')
for mutation in ('mode','state','namespace','short'):
 fields=[0,1,1,1,0,2,2,2]
 if mutation=='mode':fields[0]=1
 if mutation=='state':fields[4]=1
 if mutation=='namespace':fields[7]=1
 payload=struct.pack('<8I',*fields)
 if mutation=='short':payload=payload[:20]
 try:SpawnBroker._decode(payload,True)
 except ValueError:pass
 else:raise AssertionError(mutation)
print('four request-state/namespace protocol denials passed')

# Supported tuples remain distinct: neither profile may accept the other's entry.
other=dict(base,RECON_SPAWN_STATE_PROFILE=('msvcrt510-wait-success-v1' if cp0 else 'msvcrt510-cp0-wait-success-v1'))
p=subprocess.run([binary,reference,'positive'],env=other,capture_output=True,timeout=10)
assert p.returncode==125,(p.returncode,p.stderr)
print('cross-profile entry rejection passed')

for bad_last in (1,3,0xffffffff):
 payload=struct.pack('<8I',0,1,0,0,0,2,bad_last,2)
 try:SpawnBroker._decode(payload,True)
 except ValueError:pass
 else:raise AssertionError(bad_last)
print('three unsupported LastError tuple rejections passed')
