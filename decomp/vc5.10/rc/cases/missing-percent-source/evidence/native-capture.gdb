
set pagination off
set confirm off
set disable-randomization off
start /r missing%source.rc >/root/.ai-tmp/vc510-rc-error-y0wpguso/controls/stdout.bin 2>/root/.ai-tmp/vc510-rc-error-y0wpguso/controls/stderr.bin
python
import gdb,json
proof={'allocations':[],'escaped':[]}
class Returned(gdb.FinishBreakpoint):
 def __init__(self,frame,entry):
  super().__init__(frame,internal=True);self.entry=entry
 def stop(self):
  self.entry['returned_nonnull']=bool(int(gdb.parse_and_eval('$eax')))
  return False
class Allocation(gdb.Breakpoint):
 def stop(self):
  frame=gdb.newest_frame();caller=frame.older()
  if caller and caller.name()=='rc_diagnostic':
   entry={'bytes':int(gdb.parse_and_eval('*(unsigned int *)($esp+4)')),'caller':caller.name()}
   proof['allocations'].append(entry);Returned(frame,entry)
  return False
class Escaped(gdb.Breakpoint):
 def stop(self):
  value=gdb.parse_and_eval('escaped')
  data=value.string().encode('ascii')
  proof['escaped'].append({'bytes':data.hex(),'percent_count':data.count(b'%')})
  return False
Allocation('malloc',internal=True)
Escaped('src/rc.c:42',internal=True)
end
continue
python
assert int(gdb.parse_and_eval('$_exitcode'))==1,'Wrong native error-case exit'
gdb.write('RC_PERCENT_PROOF='+json.dumps(proof,sort_keys=True)+'\n')
end
quit
