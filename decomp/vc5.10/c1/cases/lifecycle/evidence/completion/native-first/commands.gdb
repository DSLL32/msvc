set pagination off
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
run > /root/.ai-tmp/msvc2/vc5.10/c1/native-completion-002/stdout.bin 2> /root/.ai-tmp/msvc2/vc5.10/c1/native-completion-002/stderr.bin
python
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-completion-002/state.json','w').write(json.dumps({'events':rows,'exit':int(gdb.parse_and_eval('$_exitcode'))},indent=2)+'\n')
end
quit
