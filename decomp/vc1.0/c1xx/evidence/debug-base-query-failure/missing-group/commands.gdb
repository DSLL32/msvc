set pagination off
break emit_debug_base_fields
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/debug-base-query-native-policy-w0o97hei/stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/debug-base-query-native-policy-w0o97hei/stderr.bin
python
import gdb,json
assert gdb.newest_frame().name()=="emit_debug_base_fields"
assert int(gdb.parse_and_eval("*member_index"))==0
assert int(gdb.parse_and_eval("debug_field_bytes"))==0
end
set $owner = tag
set $counter = member_index
set $base = (BaseClassLink *)calloc(1,sizeof(BaseClassLink))
set $base->kind = 15
set $base->flags = 1
set $base->state = 1
set $base->context_index = tag->type->field08
set tag->auxiliary.scope_members = (CppSymbol *)$base
set $groups = (Vector *)calloc(1,sizeof(Vector))
set tag->detail.class_info->virtual_table_groups = $groups
disable 1
break pad_debug_field
continue
python
assert gdb.newest_frame().name()=="pad_debug_field"
end
return (unsigned)0
disable 2
continue
python
frame=gdb.newest_frame()
assert int(gdb.parse_and_eval("*$counter"))==1
assert frame.name()=="emit_debug_base_fields"
assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5
pc=frame.pc(); assert bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\xcc"
assert int(gdb.parse_and_eval("$groups->count"))==0
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/debug-base-query-native-policy-w0o97hei/observation.json',"w").write(json.dumps({'status': 'pass', 'mode': 'missing group rejected before output reads', 'synthetic_native_only': True, 'unaccepted_padding': 'forced zero return only for focused control', 'public_reachability': 'UNKNOWN'},indent=2)+"\n")
end
