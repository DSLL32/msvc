#!/usr/bin/env python3
"""Linux/GDB-only /help state observation. Never imports an original runner."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import time
from inputs import fingerprints
BINARY=Path(__file__).resolve().parents[1]

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--initial',action='store_true');p.add_argument('--original',type=Path);p.add_argument('--configuration',type=Path,help='read-only immutable original trial for native-only recapture');a=p.parse_args()
start=time.perf_counter();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
exe=BINARY/'build/native/CVTRES';assert exe.exists()
assert subprocess.run(['make','-q','native'],cwd=BINARY).returncode==0,'native build is stale'
shutil.copyfile(BINARY/'build/native/inputs.json',out/'native-build.json')
(out/'configuration.json').write_bytes((a.configuration or (BINARY/'functions.json')).read_bytes())
script='''import gdb,json
result={"events":[]}
def stopped(event):
 if isinstance(event,gdb.SignalEvent):result["events"].append({"signal":event.stop_signal})
def exited(event):result["events"].append({"exit":getattr(event,"exit_code",None)})
gdb.events.stop.connect(stopped);gdb.events.exited.connect(exited)
result["entries"]=[]
def globals_state():
 values={}
 for item in DATA:
  value=gdb.parse_and_eval(item["name"])
  address=int(value.address)
  raw=bytes(gdb.selected_inferior().read_memory(address,item["size"]))
  values[item["name"]]={"address":address,"raw_hex":raw.hex()}
 return values
def crt_state():
 values={}
 for item in CRT:
  value=gdb.parse_and_eval(item["name"])
  raw=bytes(gdb.selected_inferior().read_memory(int(value.address),item["size"]))
  record={"raw_hex":raw.hex()}
  if item["kind"]=="cstring":record["cstring"]=value.string()
  if item["kind"]=="file":
   base=int(value["_base"]);ptr=int(value["_ptr"])
   record["used"]=ptr-base if base else 0
   if record["used"]:record["buffer_hex"]=bytes(gdb.selected_inferior().read_memory(base,record["used"])).hex()
   else:record["buffer_hex"]=""
  values[item["name"]]=record
 return values
class MessageReturn(gdb.FinishBreakpoint):
 def __init__(self,frame,id):
  self.message_id=id
  super().__init__(frame,internal=True)
 def stop(self):
  result.setdefault("message_returns",[]).append({"id":self.message_id,"return_value":int(self.return_value),"globals":globals_state(),"crt":crt_state()})
  return False
class ResourceCall(gdb.Breakpoint):
 def stop(self):
  frame=gdb.newest_frame()
  result.setdefault("resource_calls",[]).append({"handle":int(frame.read_var("handle")),"id":int(frame.read_var("id")),"output":int(frame.read_var("output")),"capacity":int(frame.read_var("capacity")),"expected_loader":int(gdb.parse_and_eval("&recon_LoadStringA")),"globals":globals_state()})
  return False
class Entry(gdb.Breakpoint):
 def __init__(self,name):
  self.name=name
  super().__init__(name,internal=True)
 def stop(self):
  frame=gdb.newest_frame();record={"function":self.name,"globals":globals_state(),"crt":crt_state()}
  if self.name=="main":
   record["argc"]=int(frame.read_var("argc"));argv=frame.read_var("argv")
   record["argv"]=[argv[i].string() for i in range(record["argc"])]
   record["envp_null"]=int(frame.read_var("envp"))==0
  elif self.name=="usage":record["status"]=int(frame.read_var("status"))
  elif self.name=="load_message":
   record["id"]=int(frame.read_var("id"))
   if OBSERVE:MessageReturn(frame,record["id"])
  result["entries"].append(record)
  return False
for name in ["main","usage","load_message"]:Entry(name)
if OBSERVE:ResourceCall("recon_LoadStringA",internal=True)
gdb.execute("run")
result["info_program"]=gdb.execute("info program",to_string=True)
try:
 result["exit_status"]=int(gdb.parse_and_eval("$_exitcode"))
except gdb.error:pass
try:
 result["stop_signal"]=int(gdb.parse_and_eval("$_siginfo.si_signo"))
except gdb.error:pass
try:
 frame=gdb.newest_frame();result["function"]=frame.name()
 result["pc"]=int(frame.pc());result["esp"]=int(gdb.parse_and_eval("$esp"))
 result["stack_hex"]=bytes(gdb.selected_inferior().read_memory(result["esp"],80)).hex()
 result["crt"]=crt_state()
 result["globals"]={}
 for item in DATA:
  value=gdb.parse_and_eval(item["name"])
  address=int(value.address)
  raw=bytes(gdb.selected_inferior().read_memory(address,item["size"]))
  result["globals"][item["name"]]={"address":address,"raw_hex":raw.hex()}

 if frame.name()=="main":
  result["argc"]=int(frame.read_var("argc"));argv=frame.read_var("argv")
  result["argv"]=[argv[i].string() for i in range(result["argc"])]
 if frame.name()=="usage":result["status"]=int(frame.read_var("status"))
 if frame.name()=="load_message":result["id"]=int(frame.read_var("id"))
except gdb.error as error:result["frame_error"]=str(error)
open(OUTPUT,"w").write(json.dumps(result,indent=2)+"\\n")
'''
config=json.loads((out/'configuration.json').read_text())
script='OUTPUT='+repr(str(out/'state.json'))+'\nDATA='+repr(config.get('data',[]))+'\nCRT='+repr(config.get('crt_data',[]))+'\nOBSERVE='+repr(bool(config.get('observations')))+'\n'+script
(out/'observe.py').write_text(script)
args='/help > '+shlex.quote(str(out/'stdout.bin'))+' 2> '+shlex.quote(str(out/'stderr.bin'))
canonical='R:\\toolchains\\5.10\\bin\\CVTRES.EXE'
commands=['gdb','-q','-nx','-batch',str(exe),'-ex','set pagination off','-ex','set confirm off','-ex','set exec-wrapper /usr/bin/env -a '+shlex.quote(canonical),'-ex','set args '+args,'-ex','source '+str(out/'observe.py')]
config=json.loads((out/'configuration.json').read_text())
(out/'trace-filter').write_text(''.join(f['native']+'\n' for f in config['functions']))
(out/'trace-seed').write_text('')
trace=(out/'trace.txt').open('wb')
env=os.environ.copy()
if a.original:
 original_image=(a.original.resolve()/'CVTRES.EXE')
 scratch=Path.home()/'.ai-tmp/msvc2'
 assert original_image.is_relative_to(scratch)
 program_path='S:\\'+str(original_image.relative_to(scratch)).replace('/','\\')
else:program_path='S:\\vc5.10\\cvtres\\help-canonical-reference\\CVTRES.EXE'
env['RECON_PROGRAM_PATH']=program_path
provider=out/'help.resources'
subprocess.run(['python3',str(BINARY/'scripts/prepare-resources.py'),'--output',str(provider)],cwd=BINARY,check=True,stdout=subprocess.DEVNULL)
env['CVTRES_RESOURCE_PROVIDER']=str(provider)
env.update(RECON_TRACE_FD=str(trace.fileno()),RECON_TRACE_FILTER=str(out/'trace-filter'),RECON_TRACE_SEEN=str(out/'trace-seed'))
if a.initial:env['CVTRES_INITIAL_TRAP']='1'
else:env.pop('CVTRES_INITIAL_TRAP',None)
setup=time.perf_counter()-start;t=time.perf_counter();r=subprocess.run(commands,cwd=out,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30,pass_fds=(trace.fileno(),))
trace.close()
(out/'gdb.stdout').write_bytes(r.stdout);(out/'gdb.stderr').write_bytes(r.stderr)
execution=time.perf_counter()-t
receipt=dict(schema=1,purpose='native GDB raw state',case='help',argv=['/help'],initial=a.initial,native_sha256=sha(exe),build_snapshot_sha256=sha(out/'native-build.json'),configuration_sha256=sha(out/'configuration.json'),original_sha256=config['target']['sha256'],rebuilt_sha256=sha(BINARY/'build/CVTRES.EXE'),map_sha256=sha(BINARY/'build/cvtres.map'),gdb_status=r.returncode,raw_sha256={name:sha(out/name) for name in ['state.json','gdb.stdout','gdb.stderr','stdout.bin','stderr.bin','trace.txt','help.resources']},seconds=dict(setup=setup,execution=execution,total=time.perf_counter()-start))
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
assert r.returncode==0 and (out/'state.json').is_file(),repr(r.stderr)
print((out/'state.json').read_text())
