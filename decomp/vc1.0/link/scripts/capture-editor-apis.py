#!/usr/bin/env python3
"""Explicit native GDB API-state check against cached original editor observations.

Not a fixture dependency; no original execution, capture or reference refresh.
"""
import importlib.util,json,os,shlex,shutil,subprocess,tempfile,time,datetime,struct
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,LIBRARY,native_work
BINARY=Path(__file__).resolve().parents[1]
def main():
 out=Path(tempfile.mkdtemp(prefix='editor-api-native-',dir=SCRATCH/'vc1.0/link/frontiers'))
 spec=json.loads((BINARY/'cases/editor-rebase-bind/spec-bind.json').read_text())
 work,drives=native_work(out/'native',r'S:\vc1.0\link\editor-bind')
 for item in spec['inputs']:shutil.copyfile(BINARY/item['path'],work/Path(item['path']).name)
 env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LIB=LIBRARY,TZ='PST8PDT',TMP=str(work),TEMP=str(work))
 for k in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO','COPYCMD'):env.pop(k,None)
 gdb=out/'entry.gdb';gdb.write_text('''set pagination off
set confirm off
python
import gdb,json,struct
rows=[]
def errors():return dict(errno=int(gdb.parse_and_eval('(int)errno')),last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
def words(address,count):return list(struct.unpack('<'+'H'*count,bytes(gdb.selected_inferior().read_memory(address,count*2))))
class Returned(gdb.FinishBreakpoint):
 def __init__(self,name,args):
  super().__init__(gdb.newest_frame(),internal=True);self.name=name;self.args=args
 def stop(self):
  r=dict(name=self.name,before=False,**errors())
  if self.name!='GetSystemTime':r['result']=int(self.return_value)
  if self.name=='GetSystemTime':r['system_time']=words(self.args[0],8)
  if self.name=='SystemTimeToFileTime':r['file_time']=words(self.args[1],4)
  rows.append(r);return False
class Entry(gdb.Breakpoint):
 def __init__(self,name,names):super().__init__('recon_'+name,internal=True);self.name=name;self.names=names
 def stop(self):
  caller=gdb.newest_frame().older()
  if caller is None or caller.name()!='bind_imports':return False
  args=[int(gdb.parse_and_eval(n)) for n in self.names]
  r=dict(name=self.name,before=True,**errors())
  if self.name=='FlushViewOfFile':
   assert args[0]==int(gdb.parse_and_eval('&editor_workspace')) and args[1]==0
   assert int(gdb.parse_and_eval('sizeof(editor_workspace)'))==36
   r.update(base='editor_workspace',bytes=args[1])
  if self.name=='SystemTimeToFileTime':r['system_time']=words(args[0],8)
  if self.name=='SetFileTime':
   assert args[0]==int(gdb.parse_and_eval('editor_workspace.bound.file')) and args[1]==args[2]==0
   r.update(file='current client image',creation=None,access=None,file_time=words(args[3],4))
  rows.append(r);Returned(self.name,args);return False
Entry('FlushViewOfFile',['address','bytes'])
Entry('GetSystemTime',['out'])
Entry('SystemTimeToFileTime',['input','output'])
Entry('SetFileTime',['handle','creation','access','write_time'])
end
run '''+shlex.join(spec['arguments'])+' >'+str(out/'stdout.bin')+' 2>'+str(out/'stderr.bin')+'''
python
assert len(rows)==8
open('''+repr(str(out/'native.json'))+''','w').write(json.dumps(rows,indent=2)+'\\n')
end
''')
 start=time.perf_counter();r=subprocess.run(['gdb','-q','-batch','-x',str(gdb),'--args',str(BINARY/'build/native/LINK.EXE')],cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=35)
 (out/'native.log').write_bytes(r.stdout);assert r.returncode==0,r.stdout.decode()
 native=json.loads((out/'native.json').read_text());original=[json.loads(s) for s in (BINARY/'cases/editor-rebase-bind/capture/bind-apis/events.jsonl').read_text().splitlines()];original=[r for r in original if r['event']=='editor-api']
 expected=[(n,b) for n in ('FlushViewOfFile','GetSystemTime','SystemTimeToFileTime','SetFileTime') for b in (True,False)]
 assert [(r['name'],bool(r['before'])) for r in original]==expected==[(r['name'],r['before']) for r in native]
 assert all(r['errno']==r['last_error']==0 for r in original+native)
 assert all(r['result']==1 for r in original+native if not r['before'] and r['name']!='GetSystemTime')
 def verify_clock(rows,native_words):
  system=next(r['system_time'] for r in rows if r['name']=='GetSystemTime' and not r['before'])
  converted=next(r['file_time'] for r in rows if r['name']=='SystemTimeToFileTime' and not r['before'])
  applied=next(r['file_time'] for r in rows if r['name']=='SetFileTime' and r['before'])
  assert converted==applied
  if native_words:converted=struct.unpack('<II',struct.pack('<HHHH',*converted))
  delta=datetime.datetime(system[0],system[1],system[3],system[4],system[5],system[6])-datetime.datetime(1601,1,1)
  ticks=(delta.days*86400+delta.seconds)*10000000+system[7]*10000
  assert ticks==converted[0]+(converted[1]<<32)
  return ticks
 verify_clock(original,False);ticks=verify_clock(native,True)
 assert (work/'client.exe').stat().st_mtime_ns==(ticks-116444736000000000)*100
 ms=importlib.util.spec_from_file_location('editor_api_artifacts',BINARY/'scripts/artifacts-editor.py');m=importlib.util.module_from_spec(ms);ms.loader.exec_module(m)
 assert m.normalize_artifact((work/'client.exe').read_bytes(),'edited-pe')==m.normalize_artifact((BINARY/'cases/editor-rebase-bind/expected/bind/client.exe').read_bytes(),'edited-pe')
 for key in ('stdout','stderr'):assert (out/(key+'.bin')).read_bytes()==(BINARY/'cases/editor-rebase-bind/capture/bind-apis'/(key+'.bin')).read_bytes()
 receipt=dict(status='pass',seconds=time.perf_counter()-start,mode='native GDB analysis only; cached original observations',expected_inventory=expected,original=original,native=native,clock_rule='Each side independently proves exact UTC SYSTEMTIME to FILETIME conversion and identical requested last-write value; native actual file mtime verified. Absolute wallclock differs between executions.',full_artifact_and_streams_match=True)
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(out/'receipt.json')
if __name__=='__main__':main()
