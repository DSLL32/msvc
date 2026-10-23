#!/usr/bin/env python3
"""Native-only GDB capture of lifecycle boundaries and real process completion."""
import json,subprocess,time,argparse
from pathlib import Path
from capture_native_support import PROJECT,ROOT,SCRATCH,sha
from provenance import freeze,native_sources

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--name',required=True);a=p.parse_args()
 assert a.name.replace('-','').isalnum()
 d=SCRATCH/a.name;d.mkdir(parents=True,exist_ok=False)
 binary=PROJECT/'build/native/c1-host'
 assert subprocess.run(['make','-q','build/native/c1-host'],cwd=PROJECT,stdout=subprocess.DEVNULL).returncode==0
 freeze(native_sources(),d/'source-snapshot.json')
 code='''set pagination off
set confirm off
python
import gdb,json
rows=[]
def integer(expr): return int(gdb.parse_and_eval(expr))
class Entry(gdb.Breakpoint):
 def stop(self):
  f=gdb.newest_frame()
  rows.append({'kind':'entry','symbol':f.name(),'args':{k:int(f.read_var(k).cast(gdb.lookup_type('unsigned int'))) for k in ('module','reason','reserved')}})
  return False
class Observe(gdb.Breakpoint):
 def stop(self):
  identity=gdb.parse_and_eval('*c1_host_identity')
  r={'kind':'return','reason':integer('reason'),'status':integer('status'),'module_token':integer('(unsigned int)c1_host_module'),'module_identity':{k:identity[k].string() for k in ('name','path','sha256')}}
  r['module_identity']['flags']=int(identity['flags'])
  for key,expr in [('crt_attached','c1_crt_attached'),('raw_dllmain','(unsigned int)c1_raw_dllmain'),('onexit_begin','(unsigned int)c1_onexit_begin'),('onexit_end','(unsigned int)c1_onexit_end'),('crt_adjust_fdiv','c1_crt_adjust_fdiv'),('dll_instance','(unsigned int)c1_dll_instance'),('win32_last_error','last_error'),('x87_control','$fctrl')]: r[key]=integer(expr)
  r['onexit_first']=integer('(unsigned int)*c1_onexit_begin') if r['onexit_begin'] else None
  r['thread_calls_disabled']=integer('(int)recon_module_thread_calls_disabled(c1_host_module)')
  rows.append(r)
  return False
for name in ('c1_crt_entry','c1_crt_init','DllMain'): Entry(name)
Observe('c1_host_observe')
end
run > %s 2> %s
python
open(%r,'w').write(json.dumps({'events':rows,'exit':int(gdb.parse_and_eval('$_exitcode'))},indent=2)+'\\n')
end
quit
''' % (d/'stdout.bin',d/'stderr.bin',str(d/'state.json'))
 (d/'commands.gdb').write_text(code);start=time.perf_counter()
 r=subprocess.run(['gdb','--batch','-x',str(d/'commands.gdb'),'--args',str(binary)],cwd=PROJECT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=15)
 (d/'gdb.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout.decode()
 state=json.loads((d/'state.json').read_text());assert state['exit']==0
 candidate=json.loads((PROJECT/'cases/lifecycle/candidate.json').read_text())
 for source,target in [('cases/lifecycle/candidate.json','candidate.json'),(candidate['audit_config'],'functions.json'),('build/trial-audit.json','audit.json'),('build/trial-audit-binding.json','audit-binding.json')]: (d/target).write_bytes((PROJECT/source).read_bytes())
 receipt={'schema':1,'kind':'native-gdb-completion','seconds':time.perf_counter()-start,'native_sha256':sha(binary),'rebuilt_sha256':sha(PROJECT/'build/c1.dll'),'candidate_sha256':sha(d/'candidate.json'),'functions_sha256':sha(d/'functions.json'),'audit_binding_sha256':sha(d/'audit-binding.json'),'artifacts':{f.name:{'sha256':sha(f),'size':f.stat().st_size} for f in d.iterdir() if f.is_file()}}
 (d/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'output':str(d),'seconds':receipt['seconds'],'exit':state['exit']}))
if __name__=='__main__':main()
