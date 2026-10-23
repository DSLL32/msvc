set pagination off
set confirm off
set disable-randomization off
python
import gdb,json,struct
calls=[]
u=lambda expression:int(gdb.parse_and_eval(expression))&0xffffffff
read=lambda address,count:bytes(gdb.selected_inferior().read_memory(address,count))
class Returned(gdb.Breakpoint):
 def __init__(self,entry):
  super().__init__('*'+str(entry['return_address']),temporary=True)
  self.entry=entry
 def stop(self):
  entry=self.entry
  after={name:u('$'+name)for name in ['esp','ebx','esi','edi','ebp','fctrl','eax']}
  assert after['esp']==entry['registers']['esp']+4+entry['callee_pop']
  for name in ['ebx','esi','edi','ebp','fctrl']:assert after[name]==entry['registers'][name]
  entry['returned_registers']=after
  entry['object_after']=read(entry['registers']['ecx'],116).hex()
  return False
class Entry(gdb.Breakpoint):
 def __init__(self,name,pop):
  super().__init__('*'+name)
  self.name=name;self.pop=pop
 def stop(self):
  registers={name:u('$'+name)for name in ['eip','esp','ecx','edx','ebx','esi','edi','ebp','fctrl']}
  frame=struct.unpack('<IIII',read(registers['esp'],16))
  entry=dict(function=self.name,registers=registers,return_address=frame[0],callee_pop=self.pop,
             object_before=read(registers['ecx'],116).hex())
  if self.pop:
   entry.update(path_pointer=frame[1],path=read(frame[1],9).hex(),mode_pointer=frame[2],
                mode=read(frame[2],3).hex(),signature=frame[3])
   assert entry['path']==b'unit.pdb\0'.hex() and entry['signature']==0x31415926
  calls.append(entry);Returned(entry)
  return False
Entry('open_program_database_object',12)
Entry('reload_program_database',0)
end
run 5 /root/.ai-tmp/msvc2/vc1.0/c1/validation/pdb-owned-ABI-unit-5/report >/root/.ai-tmp/msvc2/vc1.0/c1/validation/pdb-owned-ABI-unit-5/stdout 2>&1
python
assert u('$eip')==u('(unsigned)create_program_database')+1
end
signal SIGTRAP
python
assert [entry['function']for entry in calls]==['open_program_database_object', 'reload_program_database']
assert all('returned_registers'in entry for entry in calls)
open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/pdb-owned-ABI-unit-5/state.json','w').write(json.dumps(calls,indent=2)+'\n')
end
