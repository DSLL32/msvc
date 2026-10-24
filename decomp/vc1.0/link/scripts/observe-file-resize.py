#!/usr/bin/env python3
"""Fresh native GDB resize ABI/state observation; not a cached fixture."""
import argparse,hashlib,json,os,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser();p.add_argument('--phase',choices=['mapped','buffered'],required=True);p.add_argument('--trial');a=p.parse_args();config=json.loads((BINARY/'functions.json').read_text());allowed={n[1:] for n in config['accepted']};allowed.add(a.trial)
    d=Path(tempfile.mkdtemp(prefix='file-resize-gdb-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(d,'S:\\vc1.0\\link\\direct-file-resize-'+a.phase)
    source=r'''
import gdb,json,struct
from pathlib import Path
events=[];terminal=[];out=Path(OUTPUT)
def val(s):return int(gdb.parse_and_eval(s))&0xffffffff
def mem(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def words(p,n):return list(struct.unpack('<'+'I'*n,mem(p,n*4)))
def persist():out.write_text(json.dumps(dict(events=events,terminal=terminal),indent=2)+'\n')
def snap(name,event):
 esp=val('$esp');handle=val('resize_test_handle');phase=val('resize_test_phase');slot=val('files[%d]'%handle);state=words(slot,10);formals=words(esp+4,2) if event!='return' else []
 e=dict(event=event,name=name,phase=phase,formal_words=formals,esp=esp,eax=val('$eax'),ebx=val('$ebx'),esi=val('$esi'),edi=val('$edi'),ebp=val('$ebp'),caller=words(esp,1)[0],slot_address=slot,slot=state,errors=[val('*resize_test_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],read_buffer=mem(val('&resize_test_buffer'),64).hex())
 if state[4] and not state[1]&8 and state[5] and state[7]>=32768:
  e['page_table']=words(state[5],8);e['pages']=[dict(index=i,address=p,head=words(p,6),tail=words(p+4120,4)) for i,p in enumerate(e['page_table']) if p>1]
 return e
class ApiReturn(gdb.Breakpoint):
 def __init__(self,before):self.before=before;super().__init__('*'+str(before['caller']),internal=True)
 def stop(self):
  e=api_snap(self.before['name'],'api_return');e['arguments']=self.before['arguments'];e['caller']=self.before['caller'];events.append(e);persist()
  assert all(e[r]==self.before[r] for r in ['ebx','esi','edi','ebp'])
  assert e['esp']==self.before['esp']+(4 if self.before['name']=='recon_chsize' else (20 if self.before['name']=='recon_SetFilePointer' else 8))
  self.enabled=False;return False
def api_snap(name,event):
 esp=val('$esp');n=4 if name=='recon_SetFilePointer' else (1 if name=='recon_SetEndOfFile' else 2)
 return dict(event=event,name=name,phase=val('resize_test_phase'),esp=esp,eax=val('$eax'),ebx=val('$ebx'),esi=val('$esi'),edi=val('$edi'),ebp=val('$ebp'),caller=words(esp,1)[0],arguments=words(esp+4,n),errors=[val('*resize_test_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],fp_control=val('$fctrl'),fp_status=val('$fstat'),fp_tag=val('$ftag'),fp_raw=gdb.execute('info float',to_string=True))
class ApiEntry(gdb.Breakpoint):
 def __init__(self,name):self.name=name;super().__init__('*'+name,internal=True)
 def stop(self):
  phase=val('resize_test_phase');caller=gdb.newest_frame().older();owner=caller.name() if caller else None
  wanted=(PHASE=='mapped' and phase==8 and owner=='resize_mapped_file' and self.name!='recon_chsize') or (PHASE=='buffered' and phase in [4,5,8] and owner=='resize_buffered_file' and self.name=='recon_chsize')
  if not wanted:return False
  e=api_snap(self.name,'api_entry');e['actual_caller']=owner;events.append(e);persist();ApiReturn(e);return False
class Return(gdb.Breakpoint):
 def __init__(self,before):self.before=before;super().__init__('*'+str(before['caller']),internal=True)
 def stop(self):
  e=snap(self.before['name'],'return');e['formal_words']=self.before['formal_words'];events.append(e);persist()
  assert e['esp']==self.before['esp']+4 and all(e[r]==self.before[r] for r in ['ebx','esi','edi','ebp'])
  self.enabled=False;return False
class Entry(gdb.Breakpoint):
 def __init__(self,name):self.name=name;super().__init__('*'+name,internal=True)
 def stop(self):
  e=snap(self.name,'entry' if self.name in ALLOWED else 'frontier');events.append(e);persist()
  if self.name not in ALLOWED:return True
  caller=gdb.newest_frame().older();e['actual_caller']=caller.name() if caller else None;Return(e);return False
def exited(e):terminal.append(dict(event='exit',code=getattr(e,'exit_code',None)));persist()
gdb.events.exited.connect(exited);gdb.execute('set pagination off');gdb.execute('set confirm off')
for n in ['resize_file','resize_mapped_file','resize_buffered_file']:Entry(n)
for n in ['recon_SetFilePointer','recon_SetEndOfFile','recon_chsize']:ApiEntry(n)
gdb.execute('run '+PHASE+' >'+STDOUT+' 2>'+STDERR);persist();assert events
'''
    for k,v in dict(OUTPUT=str(d/'state.json'),ALLOWED=allowed,PHASE=a.phase,STDOUT=str(d/'stdout.bin'),STDERR=str(d/'stderr.bin')).items():source=source.replace(k,repr(v))
    (d/'observe.py').write_text(source);env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,TZ='PST8PDT',LC_ALL='C')
    for k in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(k,None)
    exe=BINARY/'build/native/file-resize';command=['gdb','-q','-nx','-batch','-ex','file '+str(exe),'-ex','source '+str(d/'observe.py')];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=15);elapsed=time.perf_counter()-t
    (d/'gdb.stdout').write_bytes(r.stdout);(d/'gdb.stderr').write_bytes(r.stderr);(d/'receipt.json').write_text(json.dumps(dict(command=command,seconds=elapsed,exit=r.returncode,phase=a.phase,trial=a.trial,elf_sha256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n');print(d,flush=True);assert r.returncode==0,(r.stdout,r.stderr)
if __name__=='__main__':main()
