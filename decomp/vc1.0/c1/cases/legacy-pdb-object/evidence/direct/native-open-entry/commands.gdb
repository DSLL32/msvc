set pagination off
set confirm off
set disable-randomization off
break *open_program_database_object
run 0 /root/.ai-tmp/msvc2/vc1.0/c1/validation/pdb-owned-open-native-entry/report >/root/.ai-tmp/msvc2/vc1.0/c1/validation/pdb-owned-open-native-entry/stdout 2>&1
python
assert int(gdb.parse_and_eval('$eip'))==int(gdb.parse_and_eval('(unsigned)create_program_database'))+1
end
signal SIGTRAP
python
import json,struct
u=lambda x:int(gdb.parse_and_eval(x))&0xffffffff
p=u('$ecx');sp=u('$esp');read=lambda a,n:bytes(gdb.selected_inferior().read_memory(a,n))
args=struct.unpack('<IIII',read(sp,16))
assert args[3]==0x31415926
out=dict(registers={r:u('$'+r)for r in ['eip','esp','ecx','edx','ebx','esi','edi','ebp']},stack=list(args),path=read(args[1],9).hex(),mode=read(args[2],3).hex(),object=read(p,116).hex())
open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/pdb-owned-open-native-entry/state.json','w').write(json.dumps(out,indent=2)+'\n')
end
kill
