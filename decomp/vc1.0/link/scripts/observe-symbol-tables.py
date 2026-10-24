#!/usr/bin/env python3
"""Native GDB actual symbol-table entry/state/return observations, not a fixture."""
import argparse,hashlib,json,os,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser();p.add_argument('--trial');a=p.parse_args();config=json.loads((BINARY/'functions.json').read_text());allowed={n[1:] for n in config['accepted']};allowed.add(a.trial)
    d=Path(tempfile.mkdtemp(prefix='symbol-tables-gdb-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(d,'S:\\vc1.0\\link\\direct-symbol-tables')
    source=r'''
import gdb,json,struct,hashlib
from pathlib import Path
events=[];terminal=[];out=Path(OUTPUT)
def val(s):return int(gdb.parse_and_eval(s))&0xffffffff
def mem(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def words(p,n):return list(struct.unpack('<'+'I'*n,mem(p,n*4)))
def persist():out.write_text(json.dumps(dict(events=events,terminal=terminal),indent=2)+'\n')
def table_state(address):
 if not address:return None
 header=words(address,10);h=words(header[0],11);assert h[2]<=4096 and h[3]<=64 and header[2]<=4096
 result=dict(address=address,header=mem(address,40).hex(),hash_header=mem(header[0],44).hex(),strings=mem(header[1],header[2]).hex() if header[2] else '',entries=[],key_owner=h[8]==val('&symbol_key'))
 seen=0;heads=[]
 for bucket in range(h[2]):
  if not bucket%h[6]:
   segment=words(h[10]+bucket//h[6]*4,1)[0];bp=words(segment,1)[0];heads=words(bp,min(h[6],h[2]-bucket))
  entry=heads[bucket%h[6]]
  while entry:
   seen+=1;assert seen<=64;node=words(entry,2);raw=mem(node[0],56);short,long=struct.unpack_from('<II',raw)
   if short:name=raw[:8].split(b'\0')[0]+b'\0'
   else:
    assert 4<=long<header[2];remaining=mem(header[1]+long,min(64,header[2]-long));assert b'\0' in remaining;name=remaining.split(b'\0')[0]+b'\0'
   result['entries'].append(dict(bucket=bucket,node=entry,next=node[1],external=node[0],raw=raw.hex(),name=name.hex()));entry=node[1]
 assert seen==h[3];return result
def snap(name,event):
 esp=val('$esp');phase=val('symbol_test_phase');addresses=words(val('&symbol_test_tables'),4)
 if name=='copy_defined_names' and event=='return':addresses[1 if phase==4 else 3]=val('$eax')
 stream=words(val('&recon_iob[1]'),8);used=stream[0]-stream[2];assert 0<=used<=4096
 return dict(event=event,name=name,phase=phase,arguments=words(esp+4,1) if event!='return' else [],caller=words(esp,1)[0],esp=esp,eax=val('$eax'),ebx=val('$ebx'),esi=val('$esi'),edi=val('$edi'),ebp=val('$ebp'),errors=[val('*symbol_test_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],fp_control=val('$fctrl'),fp_status=val('$fstat'),fp_tag=val('$ftag'),fp_raw=gdb.execute('info float',to_string=True),stdout_file=stream,pending=mem(stream[2],used).hex() if used else '',short_name=mem(val('&short_symbol_name'),9).hex(),guards=mem(val('&symbol_test_slots'),64).hex(),inputs_unchanged=mem(val('&symbol_test_inputs'),4096)==Path(INPUT_PATH).read_bytes(),tables=[table_state(p) for p in addresses])
class Return(gdb.Breakpoint):
 def __init__(self,before):self.before=before;super().__init__('*'+str(before['caller']),internal=True)
 def stop(self):
  e=snap(self.before['name'],'return');e['arguments']=self.before['arguments'];events.append(e);persist();assert e['esp']==self.before['esp']+4 and all(e[r]==self.before[r] for r in ['ebx','esi','edi','ebp']);self.enabled=False;return False
class Entry(gdb.Breakpoint):
 def __init__(self,name):self.name=name;super().__init__('*'+name,internal=True)
 def stop(self):
  e=snap(self.name,'entry' if self.name in ALLOWED else 'frontier');events.append(e);persist()
  if self.name not in ALLOWED:return True
  f=gdb.newest_frame().older();e['actual_caller']=f.name() if f else None;Return(e);return False
def exited(e):terminal.append(dict(event='exit',code=getattr(e,'exit_code',None)));persist()
gdb.events.exited.connect(exited);gdb.execute('set pagination off');gdb.execute('set confirm off')
for name in ['print_external_table','copy_defined_names']:Entry(name)
gdb.execute('run '+INPUT_PATH+' >'+STDOUT+' 2>'+STDERR);persist();assert events
'''
    for key,value in dict(OUTPUT=str(d/'state.json'),ALLOWED=allowed,INPUT_PATH=str(BINARY/'cases/direct-symbol-tables/inputs/names.bin'),STDOUT=str(d/'stdout.bin'),STDERR=str(d/'stderr.bin')).items():source=source.replace(key,repr(value))
    (d/'observe.py').write_text(source);env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LC_ALL='C',TZ='PST8PDT')
    for k in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(k,None)
    exe=BINARY/'build/native/symbol-tables';command=['gdb','-q','-nx','-batch','-ex','file '+str(exe),'-ex','source '+str(d/'observe.py')];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=15);elapsed=time.perf_counter()-t
    (d/'gdb.stdout').write_bytes(r.stdout);(d/'gdb.stderr').write_bytes(r.stderr);(d/'receipt.json').write_text(json.dumps(dict(command=command,seconds=elapsed,status=r.returncode,trial=a.trial,elf_sha256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n');print(d,flush=True);assert r.returncode==0,(r.stdout,r.stderr)
if __name__=='__main__':main()
