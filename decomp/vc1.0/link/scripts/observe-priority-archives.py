#!/usr/bin/env python3
"""Explicit native GDB constructor/main observation; never a fixture."""
import argparse,hashlib,json,os,shutil,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/direct-priority-archives'
def main():
    p=argparse.ArgumentParser();p.add_argument('--trial',action='append',default=[]);a=p.parse_args();cfg=json.loads((BINARY/'functions.json').read_text());allowed={n[1:] for n in cfg['accepted']}|set(a.trial)
    d=Path(tempfile.mkdtemp(prefix='priority-archives-gdb-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(d,'S:\\vc1.0\\link\\direct-priority-archives')
    for name in ['o1.lib','p1.lib','o2.lib','p2.lib']:shutil.copyfile(CASE/'inputs'/name,work/name)
    source=r'''
import gdb,json,struct
from pathlib import Path
events=[];terminal=[];out=Path(OUTPUT)
def val(s):return int(gdb.parse_and_eval(s))&0xffffffff
def mem(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def words(p,n):return list(struct.unpack('<'+'I'*n,mem(p,n*4)))if n else []
def persist():out.write_text(json.dumps(dict(events=events,terminal=terminal),indent=2)+'\n')
def snap(name,event,before=None):
 esp=val('$esp');phase=val('priority_test_phase');n=3 if name=='insert_other_input' else 2;names=val('&priority_test_names');count=min(phase,4);nodes=words(val('&priority_test_nodes'),count)
 if name=='insert_other_input'and event=='return':nodes.append(val('$eax'))
 e=dict(event=event,name=name,phase=phase,esp=esp,eax=val('$eax'),ebx=val('$ebx'),esi=val('$esi'),edi=val('$edi'),ebp=val('$ebp'),caller=words(esp,1)[0],arguments=before['arguments']if before else words(esp+4,n),stack_argument_words=words(esp if before else esp+4,n),names_address=names,names=mem(names,272).hex(),list=words(val('&other_inputs'),3),cursors=[val('first_ordinary_input'),val('last_priority_input')],nodes=[dict(address=p,raw=words(p,4))for p in nodes],errors=[val('*priority_test_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],fp_control=val('$fctrl'),fp_status=val('$fstat'),fp_tag=val('$ftag'),fp_raw=gdb.execute('info float',to_string=True))
 if name=='main':
  argv=e['arguments'][1];e['argv']=words(argv,5);e['argv_names']=[gdb.selected_inferior().read_memory(p,160).tobytes().split(b'\0')[0].decode()if p else ''for p in e['argv']]
 return e
class Return(gdb.Breakpoint):
 def __init__(self,before):self.before=before;super().__init__('*'+str(before['caller']),internal=True)
 def stop(self):
  e=snap(self.before['name'],'return',self.before);events.append(e);persist();assert e['esp']==self.before['esp']+4 and all(e[r]==self.before[r]for r in ['ebx','esi','edi','ebp']);self.enabled=False;return False
class Entry(gdb.Breakpoint):
 def __init__(self,name):self.name=name;super().__init__('*'+name,internal=True)
 def stop(self):
  e=snap(self.name,'entry'if self.name in ALLOWED else'frontier');events.append(e);persist()
  if e['event']=='frontier':return True
  Return(e);return False
def exited(e):terminal.append(dict(event='exit',code=getattr(e,'exit_code',None)));persist()
gdb.events.exited.connect(exited);gdb.execute('set pagination off');gdb.execute('set confirm off')
for n in ['insert_other_input','main']:Entry(n)
gdb.execute('run '+PAYLOAD+' '+REPORT+' '+PROGRAMARG+' -dump /nologo /headers >'+STDOUT+' 2>'+STDERR);persist();assert events
'''
    for k,v in dict(OUTPUT=str(d/'state.json'),ALLOWED=allowed,PAYLOAD=str(CASE/'inputs/names.bin'),REPORT=str(d/'results.bin'),PROGRAMARG=PROGRAM.replace('\\','\\\\'),STDOUT=str(d/'stdout.bin'),STDERR=str(d/'stderr.bin')).items():source=source.replace(k,repr(v))
    (d/'observe.py').write_text(source);env={k:v for k,v in os.environ.items()if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,TZ='PST8PDT',LC_ALL='C')
    for k in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(k,None)
    exe=BINARY/'build/native/priority-archives';command=['gdb','-q','-nx','-batch','-ex','file '+str(exe),'-ex','source '+str(d/'observe.py')];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=15);elapsed=time.perf_counter()-t
    (d/'gdb.stdout').write_bytes(r.stdout);(d/'gdb.stderr').write_bytes(r.stderr);(d/'receipt.json').write_text(json.dumps(dict(command=command,seconds=elapsed,status=r.returncode,trial=a.trial,elf_sha256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n');print(d,flush=True);assert r.returncode==0,(r.stdout,r.stderr)
if __name__=='__main__':main()
