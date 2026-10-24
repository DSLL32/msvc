#!/usr/bin/env python3
"""Explicit native GDB four-formal relocation observations; never a fixture."""
import argparse,hashlib,json,os,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/direct-rebase-fallback'
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--trial',action='store_true');a=p.parse_args();cfg=json.loads((BINARY/'functions.json').read_text());allowed=a.trial or '_fallback_process_relocations@16'in cfg['accepted']
    d=Path(tempfile.mkdtemp(prefix='rebase-fallback-gdb-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(d,'S:\\vc1.0\\link\\direct-rebase-fallback')
    source=r'''
import gdb,json,struct
from pathlib import Path
events=[];terminal=[];frame=None;returns={};out=Path(OUTPUT)
def val(x):return int(gdb.parse_and_eval(x))&0xffffffff
def mem(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def words(p,n):return list(struct.unpack('<'+'I'*n,mem(p,n*4)))
def caller(pc):
 b=gdb.block_for_pc(pc-1)
 while b and not b.function:b=b.superblock
 return str(b.function.name)if b else None
def persist():out.write_text(json.dumps(dict(events=events,terminal=terminal),indent=2)+'\n')
def snap(event,before=None):
 esp=val('$esp');page=val('reloc_test_page');records=val('reloc_test_words');args=before['arguments']if before else words(esp+4,4)
 return dict(event=event,name='fallback_process_relocations',phase=val('reloc_test_phase'),eip=val('$eip'),esp=esp,eax=val('$eax'),ebx=val('$ebx'),esi=val('$esi'),edi=val('$edi'),ebp=val('$ebp'),arguments=args,stack_argument_words=words(before['esp']+4 if before else esp+4,4),caller=before['caller']if before else words(esp,1)[0],actual_caller=before['actual_caller']if before else caller(words(esp,1)[0]),page_address=page,word_address=records,page=mem(page,4096).hex(),page_guards=mem(page-16,16).hex(),page_guard_after=mem(page+4096,16).hex(),records=mem(records,16).hex(),record_guard_before=mem(records-16,16).hex(),record_guard_after=mem(records+16,16).hex(),errors=[val('*reloc_test_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],fp_control=val('$fctrl'),fp_status=val('$fstat'),fp_tag=val('$ftag'),fp_raw=gdb.execute('info float',to_string=True))
class Return(gdb.Breakpoint):
 def stop(self):
  global frame
  assert frame and frame['caller']==val('$eip');e=snap('return',frame);assert e['esp']==frame['esp']+20 and all(e[r]==frame[r]for r in ['ebx','esi','edi','ebp']);events.append(e);frame=None;self.enabled=False;persist();return False
class Entry(gdb.Breakpoint):
 def stop(self):
  global frame
  assert frame is None;e=snap('entry'if ALLOWED else'frontier');assert e['actual_caller']=='__wrap_main';events.append(e);persist()
  if not ALLOWED:return True
  frame=e;pc=e['caller']
  if pc not in returns:returns[pc]=Return('*'+str(pc),internal=True)
  else:returns[pc].enabled=True
  return False
def exited(e):terminal.append(dict(event='exit',code=getattr(e,'exit_code',None)));persist()
gdb.events.exited.connect(exited);gdb.execute('set pagination off');gdb.execute('set confirm off');Entry('*fallback_process_relocations',internal=True)
gdb.execute('run '+INPUT+' '+REPORT+' >'+STDOUT+' 2>'+STDERR);persist();assert events
'''
    for k,v in dict(OUTPUT=str(d/'state.json'),ALLOWED=allowed,INPUT=str(CASE/'inputs/payload.bin'),REPORT=str(d/'results.bin'),STDOUT=str(d/'stdout.bin'),STDERR=str(d/'stderr.bin')).items():source=source.replace(k,repr(v))
    (d/'observe.py').write_text(source);env={k:v for k,v in os.environ.items()if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,TZ='PST8PDT',LC_ALL='C')
    for k in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(k,None)
    exe=BINARY/'build/native/rebase-fallback';command=['gdb','-q','-nx','-batch','-ex','file '+str(exe),'-ex','source '+str(d/'observe.py')];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=20);elapsed=time.perf_counter()-t
    (d/'gdb.stdout').write_bytes(r.stdout);(d/'gdb.stderr').write_bytes(r.stderr);(d/'receipt.json').write_text(json.dumps(dict(command=command,seconds=elapsed,status=r.returncode,trial=a.trial,ELF_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),observer_sha256=hashlib.sha256((d/'observe.py').read_bytes()).hexdigest()),indent=2)+'\n');print(d,flush=True);assert r.returncode==0,(r.stdout,r.stderr)
if __name__=='__main__':main()
