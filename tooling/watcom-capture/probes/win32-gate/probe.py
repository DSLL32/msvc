#!/usr/bin/env python3
"""Four explicitly reviewed authored-child debugger probes. Never a fixture."""
import hashlib,json,os,struct,subprocess,sys,time
from dataclasses import asdict
from pathlib import Path
ROOT=Path('/root/msvc2');WORK=Path('/root/.ai-tmp/msvc2/watcom-capture/broker-runtime3')
sys.path.insert(0,str(ROOT/'tooling/watcom-capture'))
from ipc import Client,ProtocolError
from broker import Broker
from gate import Gate,Function,Span

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def win(path):return 'S:\\'+str(Path(path).relative_to('/root/.ai-tmp/msvc2')).replace('/','\\')
def private(path,data):
 fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600);os.fchmod(fd,0o600)
 with os.fdopen(fd,'wb') as stream:stream.write(data)
def main():
 profile=json.loads((WORK/'profile.json').read_text());plan=json.loads((WORK/'plan.json').read_text())
 for path,expected in plan['bindings'].items():assert sha(path)==expected,path
 assert sha(profile['child'])==profile['child_sha256']
 assert sha(profile['broker'])==profile['broker_sha256']
 env=os.environ.copy();env.update(plan['environment_updates'])
 for key in plan['environment_remove']:env.pop(key,None)
 environment=WORK/'host-environment.json'
 private(environment,(json.dumps(dict(sorted(env.items())),indent=2)+'\n').encode())
 receipt={'scope':plan['scope'],'plan_sha256':sha(WORK/'plan.json'),'profile_sha256':sha(WORK/'profile.json'),
          'runner_sha256':sha(__file__),'bindings':plan['bindings'],
          'host_environment':{'private_path':str(environment),'sha256':sha(environment),'mode':'0600'},
          'cases':[]};total=time.monotonic();owned=[]
 def save():
  receipt['elapsed_seconds']=time.monotonic()-total
  (WORK/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
 try:
  for mode in ('push','frontier-call','same-path-leaf-ret','wrong-prebyte'):
   started=time.monotonic();case=WORK/mode;case.mkdir();(case/'work').mkdir()
   private(case/'stdin.bin',b'')
   with Client(case/'channel.bin',seconds=12,heartbeat_seconds=.01) as channel:
    record={'name':mode,'steps':[]};receipt['cases'].append(record);save()
    command=['/usr/bin/wine',win(Path(profile['broker'])),win(channel.path),win(Path(profile['child'])),
       '',win(case/'work'),win(case/'stdin.bin'),win(case/'stdout.bin'),win(case/'stderr.bin'),
       win(case/'events.bin'),'%x'%profile['entry_rva'],
       '90' if mode=='wrong-prebyte' else '%x'%profile['entry_byte'],'10000','1000']
    child=subprocess.Popen(command,cwd=case,env=env,stdin=subprocess.DEVNULL,
                           stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    owned.append((child,case,record));record.update(command=command,owned_pid=child.pid)
    b=Broker(channel)
    if mode=='wrong-prebyte':
     event=b.event();assert event['code']==3
     assert event['base']==profile['preferred_base']
     assert b.read(event['base']+profile['marker_rva'],4)==b'\0'*4
     assert b.read(event['entry'],1)==bytes([profile['entry_byte']])
     try:b.trap(event['entry'],0x90)
     except ProtocolError as error:record['expected_hold']=str(error)
     else:raise AssertionError('wrong prebyte accepted')
     expected_status=92
    else:
     event,before=b.entry(profile['entry_rva'],profile['entry_byte'])
     base=event['base'];assert base==profile['preferred_base'],'unreviewed PE rebase'
     marker=base+profile['marker_rva'];assert b.read(marker,4)==b'\0'*4
     flat,segments=b.flat_selectors(before);record['selector_proof']=segments
     stack_base,stack_limit=struct.unpack('<II',b.read(event['teb']+4,8))
     region=b.region(before['esp'])
     assert region['state']==0x1000 and region['protect'] in (4,8,0x40,0x80),region
     low=max(stack_limit,region['base']);high=min(stack_base,region['base']+region['size'])
     assert low<=before['esp']-128<before['esp']<=high
     stack=Span(low,high-low,'actual writable committed TEB stack intersection',write=True)
     record['stack_proof']={'teb':event['teb'],'base':stack_base,'limit':stack_limit,'region':region,
                             'owned_start':low,'owned_size':high-low}
     functions=[];spans=[stack,Span(marker,4,'actual declared probe_marker DWORD',write=True)]
     for name in ('caller','leaf') if mode=='same-path-leaf-ret' else ('caller',):
      owner=profile[name]
      instructions={base+address:bytes.fromhex(raw) for address,raw in owner['instructions']}
      for address,raw in instructions.items():assert b.read(address,len(raw))==raw
      functions.append(Function('probe_'+name,base+owner['entry_rva'],instructions))
      spans.append(Span(base+owner['entry_rva'],owner['size'],'reviewed authored '+name+' bytes'))
     gate=Gate(functions,b.read,spans,stack,event['tid'],
               Span(base+profile['text_rva'],profile['text_size'],'actual authored text bound'),
               flat_segments=flat,max_steps=100,max_events=200,seconds=8)
     gate.event('initial-stop',event['tid']);current=before
     for step in range(80):
      decision=gate.plan(current,event['tid'])
      if decision.action=='frontier':
       assert mode=='frontier-call' and decision.transfer=='call'
       assert b.read(marker,4)==b'\0'*4
       target=decision.next_pc;prebyte=b.read(target,1)
       assert prebyte==b'\x53';trap=b.trap(target,prebyte[0])
       assert b.read(target,1)==b'\xcc'
       # Successful broker TRAP means actual saved-byte/write1/flush/readback checks
       # all passed; its source-bound patch log retains those observations.
       gate.arm_frontier(decision,current,event['tid'],b'\x53',prebyte,True,1,True)
       armed=b.arm(current,decision);observed,after=b.step()
       assert observed['tid']==event['tid'] and after['eflags']&0x100==0
       frontier=gate.observe_frontier(after,event['tid'])
       assert b.read(marker,4)==b'\0'*4
       record['frontier']={**frontier,'trap_index':trap,'armed_context':armed,
                           'actual_return_word':struct.unpack('<I',b.read(after['esp'],4))[0]}
       break
      assert decision.action=='step'
      armed=b.arm(current,decision);observed,after=b.step()
      assert observed['tid']==event['tid'] and after['eflags']&0x100==0
      gate.observe(after,event['tid'])
      record['steps'].append({'before':current,'armed':armed,'after':after,'decision':asdict(decision)})
      if mode=='push':
       expected=dict(current);expected.update(eip=current['eip']+1,esp=current['esp']-4)
       assert after==expected,(current,after)
       assert struct.unpack('<I',b.read(after['esp'],4))[0]==current['ebx']
       assert b.read(marker,4)==b'\0'*4
       record['push_actual_value']=current['ebx'];break
      if mode=='same-path-leaf-ret' and decision.transfer=='return':
       assert after['eip']==base+0x10032 and not gate.frames
       assert b.read(marker,4)==b'\x01\0\0\0'
       record['real_return']={'pc':after['eip'],'esp':after['esp'],'marker':1};break
      current=after
     else:raise AssertionError('authored step budget exhausted')
     record['marker_before_teardown']=int.from_bytes(b.read(marker,4),'little')
     b.stop();expected_status=0
    out,err=child.communicate(timeout=15)
    private(case/'launcher.stdout',out);private(case/'launcher.stderr',err)
    record.update(status=child.returncode,events=b.events,channel=channel.records,
                  elapsed_seconds=time.monotonic()-started,
                  log_sha256=sha(case/'events.bin'),stdout_sha256=sha(case/'stdout.bin'),
                  stderr_sha256=sha(case/'stderr.bin'),actual_broker_terminated=child.poll() is not None)
    log=(case/'events.bin').read_bytes()
    assert b'owned-teardown exit-event=1 signaled=1' in log,log
    assert child.returncode==expected_status and not out and not err,(child.returncode,out,err,log)
    assert (case/'stdout.bin').read_bytes()==b'' and (case/'stderr.bin').read_bytes()==b''
    save()
  receipt['all_owned_brokers_terminated']=all(child.poll() is not None for child,_,_ in owned)
  assert receipt['all_owned_brokers_terminated'];receipt['passed']=True;save()
  print('PASS: four actual authored Win32 debugger controls; no original app/acceptance')
 finally:
  for child,case,record in owned:
   if 'actual_broker_terminated' in record:continue
   forced=False
   try:
    # Client close stops heartbeat. Permit its1s lease plus3s owned-child drain.
    out,err=child.communicate(timeout=5)
   except subprocess.TimeoutExpired:
    child.kill();out,err=child.communicate(timeout=3);forced=True
    receipt['forced_broker_stop_failed_proof']=True
   for name,data in [('launcher.stdout',out),('launcher.stderr',err)]:
    path=case/name
    if not path.exists():private(path,data)
    else:assert path.read_bytes()==data,'previous launcher stream changed'
   record['failure_collection']={'status':child.returncode,'forced_outer_stop':forced,
       'actual_broker_terminated':child.poll() is not None,
       'launcher_stdout_sha256':sha(case/'launcher.stdout'),
       'launcher_stderr_sha256':sha(case/'launcher.stderr')}
   if (case/'events.bin').exists():
    record['failure_collection']['events_sha256']=sha(case/'events.bin')
    record['failure_collection']['owned_child_exit_recorded']=(
        b'owned-teardown exit-event=1 signaled=1' in (case/'events.bin').read_bytes())
  save()
if __name__=='__main__':main()
