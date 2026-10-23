set pagination off
set confirm off
break *write_helper_thunk_member
run -lib /machine:i860 /def:cross.def /name:cross.dll /debugtype:both /out:i860.lib >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/import-archive-i860-write_helper_thunk_member-8r1c0rr3/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/import-archive-i860-write_helper_thunk_member-8r1c0rr3/native.stderr
printf "NATIVE_MATCH write_helper_thunk_member\n"
x/6wx $esp
bt 5
python
import gdb,json,struct
esp=int(gdb.parse_and_eval('$esp'))
stack=struct.unpack('<3I',bytes(gdb.selected_inferior().read_memory(esp,12)))
words=list(struct.unpack('<9I',bytes(gdb.selected_inferior().read_memory(stack[1],36))))
references={n:int(gdb.parse_and_eval('&'+n)) for n in ['import_jump_i860','import_relocations_i860','import_helper_i860']}
state=dict(bytes=int(gdb.parse_and_eval('sizeof(ImportMachine)')),machine_is_i860=stack[1]==int(gdb.parse_and_eval('&import_i860')),image_is_export_image=stack[2]==int(gdb.parse_and_eval('export_image')),words=words,references=references)
open('/root/.ai-tmp/msvc2/vc1.0/link/frontiers/import-archive-i860-write_helper_thunk_member-8r1c0rr3/native-import-machine.json','w').write(json.dumps(state,indent=2)+'\n')
end
quit
