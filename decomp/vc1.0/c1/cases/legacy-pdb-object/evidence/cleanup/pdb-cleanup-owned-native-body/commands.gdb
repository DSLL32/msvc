set pagination off
set confirm off
set disable-randomization off
break *destroy_pdb_buffer_list
run >/root/.ai-tmp/msvc2/vc1.0/c1/validation/pdb-cleanup-owned-native-body/stdout 2>&1
python
import gdb,struct
report=open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/pdb-cleanup-owned-native-body/events','w')
def u(x):return int(gdb.parse_and_eval(x))&0xffffffff
def mem(label,address,size):report.write('pdb-memory %s %08x %d %s\n'%(label,address,size,bytes(gdb.selected_inferior().read_memory(address,size)).hex()))
p=u('$ecx');sp=u('$esp');ret=int.from_bytes(bytes(gdb.selected_inferior().read_memory(sp,4)),'little')
before={r:u('$'+r)for r in ['ebx','esi','edi','ebp']}
mem('object-before',p-72,116);mem('list-before',p,12)
head=int.from_bytes(bytes(gdb.selected_inferior().read_memory(p,4)),'little');mem('node-before-0',head,12)
report.write('pdb-registers entry %08x %08x %08x %08x %08x %08x\n'%tuple([sp,p]+[before[r]for r in ['ebx','esi','edi','ebp']]))
class Free(gdb.Breakpoint):
 def stop(self):
  stack=u('$esp');pointer=int.from_bytes(bytes(gdb.selected_inferior().read_memory(stack+4,4)),'little');report.write('pdb-free %08x\n'%pointer);return False
free=Free('*recon_free');gdb.Breakpoint('*'+str(ret),temporary=True)
end
delete 1
continue
python
free.delete();mem('object-after',p-72,116);mem('list-after',p,12)
after={r:u('$'+r)for r in ['ebx','esi','edi','ebp']}
report.write('pdb-registers return %08x %08x %08x %08x %08x %08x\n'%tuple([u('$esp'),u('$eax')]+[after[r]for r in ['ebx','esi','edi','ebp']]))
report.close();assert before==after and u('$esp')==sp+4
end
continue
