set pagination off
set confirm off
set disable-randomization off
break *reload_program_database
run 4 /root/.ai-tmp/msvc2/vc1.0/c1/validation/pdb-owned-reload-native-entry/report >/root/.ai-tmp/msvc2/vc1.0/c1/validation/pdb-owned-reload-native-entry/stdout 2>&1
python
assert int(gdb.parse_and_eval('$eip'))==int(gdb.parse_and_eval('(unsigned)create_program_database'))+1
end
signal SIGTRAP
python
import json
u=lambda x:int(gdb.parse_and_eval(x))&0xffffffff
p=u('$ecx');sp=u('$esp');read=lambda a,n:bytes(gdb.selected_inferior().read_memory(a,n))
out=dict(registers={r:u('$'+r)for r in ['eip','esp','ecx','edx','ebx','esi','edi','ebp']},stack=read(sp,16).hex(),object=read(p,116).hex())
open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/pdb-owned-reload-native-entry/state.json','w').write(json.dumps(out,indent=2)+'\n')
end
kill
