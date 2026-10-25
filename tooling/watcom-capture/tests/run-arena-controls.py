#!/usr/bin/env python3
"""Explicit reviewed authored Win32 arena controls; never a fixture/original run."""
import argparse
from dataclasses import replace
import hashlib,json,os,struct,subprocess,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'tooling/watcom-capture'))
from arena import Descriptors,Session
from broker import Broker
from cf_identity import Image
from gate import Hold,Span
from ipc import Client,ProtocolError

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def require(value,message):
 if not value:raise AssertionError(message)
def private(path,raw):
 fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600);os.fchmod(fd,0o600)
 with os.fdopen(fd,'wb') as f:f.write(raw)
def win(path):
 path=Path(path).resolve()
 for root,drive in ((ROOT,'R:'),(Path('/root/.ai-tmp/msvc2'),'S:')):
  try:return drive+'\\'+str(path.relative_to(root)).replace('/','\\')
  except ValueError:pass
 raise AssertionError('path outside shared drives')

class FaultBroker(Broker):
 def __init__(self,client,mode,record):super().__init__(client);self.mode=mode;self.record=record;self.arena_base=None
 def trap(self,address,expected):
  if self.mode=='wrong-byte' and self.arena_base is not None and address>=self.arena_base:
   self.record['injection']={'target':address,'actual_prebyte':self.read(address,1).hex(),
                             'correct_expected':expected,'wrong_expected':expected^1}
   return super().trap(address,expected^1)
  return super().trap(address,expected)
 def arm(self,before,decision):
  if self.mode=='wrong-pc' and getattr(decision,'action',None)=='frontier':
   self.record['injection']={'actual_target':decision.next_pc,'wrong_predicted_pc':decision.next_pc+1,
                             'actual_target_byte':self.read(decision.next_pc,1).hex()}
   return super().arm(before,replace(decision,next_pc=decision.next_pc+1))
  return super().arm(before,decision)

def descriptor(spec,plan):
 return Descriptors(spec['path'],spec['sha256'],plan['producer'],plan['producer_sha256'],spec['inputs'])
def main(plan_path):
 plan=json.loads(plan_path.read_text());work=Path(plan['work']);profile=json.loads(Path(plan['profile']).read_text())
 for path,digest in plan['bindings'].items():require(sha(path)==digest,'frozen binding changed: '+path)
 require(plan['maximum_helper_launches']==len(plan['controls'])==9,'control scope changed')
 work.mkdir();env=os.environ.copy();env.update(plan['environment_updates'])
 for name in plan['environment_remove']:env.pop(name,None)
 private(work/'host-environment.json',(json.dumps(dict(sorted(env.items())),indent=2)+'\n').encode())
 receipt={'scope':plan['scope'],'plan_sha256':sha(plan_path),'bindings':plan['bindings'],'cases':[],
          'host_environment_private_sha256':sha(work/'host-environment.json'),'application_accepted_functions':0}
 started=time.monotonic();owned=[]
 def save():
  receipt['elapsed_seconds']=time.monotonic()-started
  (work/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
 try:
  for control in plan['controls']:
   tick=time.monotonic();case=work/control['name'];case.mkdir();(case/'cwd').mkdir();private(case/'stdin.bin',b'')
   record={'name':control['name'],'expected':control};receipt['cases'].append(record);save()
   with Client(case/'channel.bin',seconds=15,heartbeat_seconds=.01) as channel:
    child_info=profile['child'];variant=plan['variants'][control['entry']]
    args=win(variant['fixture'])+(' thread-negative' if control['name']=='unexpected-thread' else '')
    command=['/usr/bin/wine',win(plan['broker']),win(channel.path),win(plan['loader']),args,win(case/'cwd'),
             win(case/'stdin.bin'),win(case/'stdout.bin'),win(case/'stderr.bin'),win(case/'events.bin'),
             '%x'%child_info['entry_rva'],'%x'%plan['entry_byte'],'12000','1000']
    child=subprocess.Popen(command,cwd=case,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    owned.append((child,case,record));record['command']=command;record['owned_launcher_pid']=child.pid
    b=FaultBroker(channel,control['name'],record);session=None
    try:
     first,entry=b.entry(child_info['entry_rva'],plan['entry_byte'])
     require(first['base']==child_info['preferred_base'],'unproved authored PE rebase')
     ready_index,ready_event,ready=b.authored_setup(child_info['ready_rva'],child_info['ready_byte'],child_info['signature_rva'])
     require(control['name']!='unexpected-thread','unexpected thread was continued to Ready')
     globals=child_info['globals'];base=first['base']
     def global_value(name):return struct.unpack('<I',b.read(base+globals[name]['rva'],4))[0]
     arena=global_value('probe_arena');size=global_value('probe_arena_size');arena_entry=global_value('probe_arena_entry')
     image=Image.parse(Path(variant['fixture']).read_bytes(),variant['fixture_sha256'],variant['header'])
     require(size==image.allocation_size and arena_entry==arena+image.entry_offset,'authored actual arena globals mismatch')
     snapshot=b''.join(b.read(arena+off,min(512,image.loaded_size-off)) for off in range(0,image.loaded_size,512))
     record['identity']=image.compare(snapshot,arena);record['actual_ready_context']=ready
     require(global_value('probe_thread_marker')==0,'unexpected authored callback ran')
     functions=[]
     for kind in ('full','ret'):
      authorization=control.get(kind,{})
      if authorization:
       mapped,identity=descriptor(variant['reports'][kind],plan).cf(image,arena,snapshot,authorization)
       functions+=mapped
     ready_functions=descriptor(plan['ready_report'],plan).fixed_pe(plan['loader'],base,b.read,plan['ready_authorization'],
                                  {base+child_info['ready_rva']:child_info['ready_byte']})
     spans=[Span(arena+profile[name]['rva'],profile[name]['size'],name,write=profile[name]['write'])
            for name in ('marker','pointer','unknown_pointer')]
     support=[Span(base+globals['probe_arena_entry']['rva'],4,'published authored CF entry pointer')]
     session=Session(b,first,ready,image,arena,snapshot,ready_functions+functions,spans,support_data_spans=support,
                     max_steps=150,seconds=10)
     b.arena_base=arena;current=session.consume_owned_entry(ready_index,ready)
     result=session.run(current);record['result']=result
     require(control['name'] not in ('forged-return','wrong-byte','wrong-pc'),'expected denial not observed')
     marker=int.from_bytes(b.read(arena+profile['marker']['rva'],4),'little');record['actual_marker']=marker
     require(marker==control['marker'],'authored marker/body mismatch')
     require(result['frontier']['transfer']==control['transfer'],'wrong frontier transfer kind')
     require(result['frontier']['context']['eip']==arena+control['target'],'wrong actual frontier target')
     if control['name']=='prefix-return-progress':
      require(sum(bool(step['decision']['pop_frame']) for step in session.records)==4,'four real returns not observed')
     b.stop();expected_status=0
    except Hold as error:
     require(control['name']=='forged-return' and 'foreign return target' in str(error),'unexpected runtime gate HOLD: '+str(error))
     require(session is not None,'missing forged-return session')
     record['expected_hold']=str(error);record['steps']=session.records;record['stopped_context']=b.context()
     require(session.current==record['stopped_context'],'forged RET denial changed context')
     require(b.read(session.current['eip'],1)==b'\xc3','forged RET already executed')
     record['actual_marker']=int.from_bytes(b.read(arena+profile['marker']['rva'],4),'little')
     require(record['actual_marker']==0,'forged return reached unknown body')
     b.stop();expected_status=0
    except ProtocolError as error:
     require(control['name'] in ('wrong-byte','wrong-pc','unexpected-thread'),'unexpected broker HOLD: '+str(error))
     record['expected_hold']=str(error);expected_status=92
     if session is not None:record['steps']=session.records;record['planned_context']=session.current
    out,err=child.communicate(timeout=6)
    private(case/'launcher.stdout',out);private(case/'launcher.stderr',err)
    record.update(status=child.returncode,events=b.events,channel=channel.records,elapsed_seconds=time.monotonic()-tick)
    log=(case/'events.bin').read_bytes()
    require(b'owned-teardown exit-event=1 signaled=1' in log,'missing actual owned EXIT/signaled')
    require(child.returncode==expected_status and not out and not err,'broker failed/unexpected launcher output')
    require((case/'stdout.bin').read_bytes()==b'' and (case/'stderr.bin').read_bytes()==b'','unexpected child output')
    if control['name']=='unexpected-thread':require(b'event code=2 ' in log and b'authored-ready-stopped' not in log,'missing actual uncontinued newthread')
    for name in ('events.bin','stdout.bin','stderr.bin'):record.setdefault('files',{})[name]=sha(case/name)
    record['actual_broker_terminated']=child.poll() is not None;save()
  receipt['passed']=True;save();print('PASS nine actual authored CF arena controls; application accepted0')
 except BaseException as error:
  receipt['failure']=repr(error)
  raise
 finally:
  for child,case,record in owned:
   if record.get('actual_broker_terminated'):continue
   forced=False
   try:out,err=child.communicate(timeout=5)
   except subprocess.TimeoutExpired:child.kill();out,err=child.communicate(timeout=3);forced=True
   for name,raw in (('launcher.stdout',out),('launcher.stderr',err)):
    p=case/name
    if not p.exists():private(p,raw)
   record['failure_collection']={'status':child.returncode,'forced_outer_stop':forced,'actual_broker_terminated':child.poll() is not None}
   if (case/'events.bin').exists():record['failure_collection']['events_sha256']=sha(case/'events.bin')
  save()
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--plan',type=Path,required=True);main(p.parse_args().plan.resolve())
