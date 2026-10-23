
import gdb,json,struct
from pathlib import Path
rows=[];owned=[]
def v(s):return int(gdb.parse_and_eval(s))&0xffffffff
def w(a):return struct.unpack('<I',bytes(gdb.selected_inferior().read_memory(a,4)))[0]
def save():Path('/root/.ai-tmp/msvc2/vc1.0/link/dispatcher-allocation-ris3va0w/state.json').write_text(json.dumps(rows,indent=2)+'\n')
class AllocReturn(gdb.FinishBreakpoint):
 def stop(self):
  base=v('$eax');before=v('*dispatcher_test_errno');usable=v('(unsigned int)malloc_usable_size((void*)%d)'%base);after=v('*dispatcher_test_errno')
  assert base and before==after and usable>=1
  owned.append(base);rows.append(dict(event='allocation_return',base=base,requested=0,provider='native glibc malloc(0)',usable=usable,errno_before=before,errno_after=after,scope='Provider allocation extent observation, not an application-requested zero-size payload contract'));save();return False
class Alloc(gdb.Breakpoint):
 def stop(self):
  older=gdb.newest_frame().older()
  if older and older.name()=='recon_setvbuf':
   esp=v('$esp');assert w(esp+4)==0;rows.append(dict(event='allocation_entry',caller=older.name(),size=w(esp+4)));save();AllocReturn(gdb.newest_frame(),internal=True)
  return False
class Done(gdb.Breakpoint):
 def stop(self):
  assert len(owned)==1;stream=gdb.parse_and_eval('recon_iob[1]');base=int(stream['_base']);assert base==owned[0]
  rows.append(dict(event='actual_FILE_owner',base=base,ptr=int(stream['_ptr']),buffer_size=int(stream['_bufsiz']),flags=int(stream['_flag']),allocated_by='actual recon_setvbuf malloc(0) observed return'));save();return False
class Start(gdb.Breakpoint):
 def stop(self):Done('*'+str(w(v('$esp'))),internal=True);self.enabled=False;return False
gdb.execute('set pagination off');gdb.execute('set confirm off');Alloc('malloc',internal=True);Start('*driver_usage',internal=True)
gdb.execute('run --direct >/root/.ai-tmp/msvc2/vc1.0/link/dispatcher-allocation-ris3va0w/stdout.bin 2>/root/.ai-tmp/msvc2/vc1.0/link/dispatcher-allocation-ris3va0w/stderr.bin');save()
