#!/usr/bin/env python3
"""Actual native entry/return/continuation state; explicit debugger analysis only."""
import argparse,hashlib,json,os,shutil,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/direct-module-diagnostics'
def main():
    p=argparse.ArgumentParser();p.add_argument('--trial');p.add_argument('--candidate',action='append',default=[]);p.add_argument('--clock-from',type=Path);a=p.parse_args();config=json.loads((BINARY/'functions.json').read_text());allowed={n[1:] for n in config['accepted']};allowed.add(a.trial);allowed.update(a.candidate);manifest=json.loads((CASE/'manifest.json').read_text())
    clock_value=manifest['fixed_time'];clock_origin=None
    if a.clock_from:
        raw=a.clock_from.read_bytes();clock=[e for e in map(json.loads,raw.decode().splitlines()) if e['event']=='clock_boundary'];assert len(clock)==1 and clock[0]['result']==clock[0]['output'];clock_value=clock[0]['result'];clock_origin=dict(path=str(a.clock_from),sha256=hashlib.sha256(raw).hexdigest())
    d=Path(tempfile.mkdtemp(prefix='module-diagnostics-gdb-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(d,manifest['cwd'])
    for name in ['helpers.lib','modules.rsp']:shutil.copyfile(CASE/'inputs'/name,work/name)
    source=r'''
import gdb,json,struct
from pathlib import Path
events=[];terminal=[];out=Path(OUTPUT)
def val(s):return int(gdb.parse_and_eval(s))&0xffffffff
def mem(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def words(p,n):return list(struct.unpack('<'+'I'*n,mem(p,n*4))) if n else []
def string(p):
 assert p
 b=bytearray()
 for i in range(512):
  c=mem(p+i,1);b+=c
  if c==b'\0':return b.hex()
 raise AssertionError('unbounded string')
def persist():out.write_text(json.dumps(dict(events=events,terminal=terminal),indent=2)+'\n')
def modules():
 owner=val('module_debug_info');count=val('output_module_index');assert count<=64
 rows=[]
 for i in range(count):
  address=owner+i*76;raw=words(address,19);module=words(raw[0],14);library=words(module[12],12);assert raw[2]==module[1] and raw[1]==library[1]
  rows.append(dict(address=address,raw=mem(address,76).hex(),names=[string(p) for p in raw[1:4]],owner_edges=dict(module=raw[0],path=module[1],library=module[12],library_name=library[1],source_is_path=int(raw[3]==module[1]))))
 return dict(owner=owner,count=count,rows=rows)
def libraries():
 image=val('module_test_image')
 if not image:return None
 head=words(image+0x144,4);rows=[];next=head[3]
 while next:
  assert len(rows)<16;raw=words(next,3);rows.append(dict(address=next,raw=mem(next,12).hex(),name=string(raw[1])));next=raw[0]
 return dict(image=image,list=image+0x144,header=mem(image+0x144,16).hex(),rows=rows)
def snap(name,event):
 esp=val('$esp');phase=val('module_test_phase');n=2 if name in ['main','add_default_libraries'] else 0
 stream=words(val('&recon_iob[1]'),8);used=stream[0]-stream[2];assert 0<=used<=4096
 e=dict(event=event,name=name,phase=phase,arguments=words(esp+4,n) if event!='return' else [],caller=words(esp,1)[0],esp=esp,eax=val('$eax'),ebx=val('$ebx'),esi=val('$esi'),edi=val('$edi'),ebp=val('$ebp'),errors=[val('*module_test_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],fp_control=val('$fctrl'),fp_status=val('$fstat'),fp_tag=val('$ftag'),fp_raw=gdb.execute('info float',to_string=True),stdout_file=stream,pending=mem(stream[2],used).hex() if used else '',input=mem(val('&module_test_input'),256).hex(),guards=mem(val('&module_test_guards'),16).hex(),modules=modules(),libraries=libraries())
 if phase==0 and event!='return':
  argc,argv=e['arguments'];assert argc==3;e['argv']=words(argv,4);e['argv_names']=[string(p) for p in e['argv'][:3]]
 return e
class ErrorWatch(gdb.Breakpoint):
 def stop(self):
  frame=gdb.newest_frame();events.append(dict(event='last_error_access',phase=val('module_test_phase'),eip=val('$eip'),caller=frame.name(),backtrace=gdb.execute('bt',to_string=True)));persist();return True
class ErrorEntry(gdb.Breakpoint):
 def stop(self):
  if val('module_test_phase')==0:return False
  frame=gdb.newest_frame().older();events.append(dict(event='last_error_entry',phase=val('module_test_phase'),eip=val('$eip'),caller=frame.name() if frame else None,backtrace=gdb.execute('bt',to_string=True)));persist();return True
class Return(gdb.Breakpoint):
 def __init__(self,before):self.before=before;super().__init__('*'+str(before['caller']),internal=True)
 def stop(self):
  e=snap(self.before['name'],'return');e['arguments']=self.before['arguments'];events.append(e);persist()
  if self.before['name']=='main':
   address=val("&'win32.c'::last_error");ErrorWatch('*(unsigned long *)'+str(address),type=gdb.BP_WATCHPOINT,wp_class=gdb.WP_ACCESS,internal=True);events.append(dict(event='last_error_watch_armed',address=address,width=4,access='readwrite'));persist()
  assert e['esp']==self.before['esp']+4 and all(e[k]==self.before[k] for k in ['ebx','esi','edi','ebp']);self.enabled=False;return False
class Entry(gdb.Breakpoint):
 def __init__(self,name):self.name=name;super().__init__('*'+name,internal=True)
 def stop(self):
  e=snap(self.name,'entry' if self.name in ALLOWED else 'frontier');events.append(e);persist()
  if self.name not in ALLOWED:return True
  frame=gdb.newest_frame().older();e['actual_caller']=frame.name() if frame else None;Return(e);return False
class Continuation(gdb.Breakpoint):
 def stop(self):
  e=snap('external_continuation','before_crt');e['main_result']=val('module_test_result');events.append(e);persist();return False
def exited(e):terminal.append(dict(event='exit',code=getattr(e,'exit_code',None)));persist()
gdb.events.exited.connect(exited);gdb.execute('set pagination off');gdb.execute('set confirm off')
for name in ['main','add_default_libraries','print_module_debug_info']:Entry(name)
Continuation('*module_test_before_crt',internal=True)
ErrorEntry('*recon_GetLastError',internal=True)
gdb.execute('run '+INPUT_PATH+' '+PROGRAM+' -link @modules.rsp >'+STDOUT+' 2>'+STDERR);persist();assert events
'''
    # Program argument is escaped for GDB's shell command parsing, preserving backslashes.
    import shlex
    for key,value in dict(OUTPUT=str(d/'state.json'),ALLOWED=allowed,INPUT_PATH=str(CASE/'inputs/libraries.bin'),PROGRAM=shlex.quote(PROGRAM),STDOUT=str(d/'stdout.bin'),STDERR=str(d/'stderr.bin')).items():source=source.replace(key,repr(value))
    (d/'observe.py').write_text(source);env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,RECON_FIXED_TIME=str(clock_value),LC_ALL='C',TZ='PST8PDT')
    for k in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(k,None)
    exe=BINARY/'build/native/module-diagnostics';command=['gdb','-q','-nx','-batch','-ex','file '+str(exe),'-ex','source '+str(d/'observe.py')];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=20);elapsed=time.perf_counter()-t
    (d/'gdb.stdout').write_bytes(r.stdout);(d/'gdb.stderr').write_bytes(r.stderr);(d/'receipt.json').write_text(json.dumps(dict(command=command,seconds=elapsed,status=r.returncode,trial=a.trial,clock_value=clock_value,clock_origin=clock_origin,elf_sha256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n');print(d,flush=True);assert r.returncode==0,(r.stdout,r.stderr)
if __name__=='__main__':main()
