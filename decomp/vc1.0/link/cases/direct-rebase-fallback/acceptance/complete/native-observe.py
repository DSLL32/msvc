
import gdb,json,struct
from pathlib import Path
events=[];terminal=[];frame=None;returns={};out=Path('/root/.ai-tmp/msvc2/vc1.0/link/rebase-fallback-gdb-44eur864/state.json')
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
  assert frame is None;e=snap('entry'if True else'frontier');assert e['actual_caller']=='__wrap_main';events.append(e);persist()
  if not True:return True
  frame=e;pc=e['caller']
  if pc not in returns:returns[pc]=Return('*'+str(pc),internal=True)
  else:returns[pc].enabled=True
  return False
def exited(e):terminal.append(dict(event='exit',code=getattr(e,'exit_code',None)));persist()
gdb.events.exited.connect(exited);gdb.execute('set pagination off');gdb.execute('set confirm off');Entry('*fallback_process_relocations',internal=True)
gdb.execute('run '+'/root/msvc2/decomp/vc1.0/link/cases/direct-rebase-fallback/inputs/payload.bin'+' '+'/root/.ai-tmp/msvc2/vc1.0/link/rebase-fallback-gdb-44eur864/results.bin'+' >'+'/root/.ai-tmp/msvc2/vc1.0/link/rebase-fallback-gdb-44eur864/stdout.bin'+' 2>'+'/root/.ai-tmp/msvc2/vc1.0/link/rebase-fallback-gdb-44eur864/stderr.bin');persist();assert events
