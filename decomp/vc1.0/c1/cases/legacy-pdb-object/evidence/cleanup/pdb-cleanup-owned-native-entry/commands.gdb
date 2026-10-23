set pagination off
set confirm off
set disable-randomization off
break *destroy_pdb_buffer_list
run >/root/.ai-tmp/msvc2/vc1.0/c1/validation/pdb-cleanup-owned-native-entry/stdout 2>&1
python
import gdb,json
p=int(gdb.parse_and_eval('$ecx'))&0xffffffff
raw=bytes(gdb.selected_inferior().read_memory(p,12));head=int.from_bytes(raw[:4],'little')
row=dict(logical='@destroy_pdb_buffer_list@4',ecx=p,esp=int(gdb.parse_and_eval('$esp')),list=raw.hex(),head=head,node=bytes(gdb.selected_inferior().read_memory(head,12)).hex(),object=bytes(gdb.selected_inferior().read_memory(p-72,116)).hex(),registers={r:int(gdb.parse_and_eval('$'+r))&0xffffffff for r in ['ebx','esi','edi','ebp','fctrl']})
with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/pdb-cleanup-owned-native-entry/state.json','w')as out:json.dump(row,out,indent=2)
end
