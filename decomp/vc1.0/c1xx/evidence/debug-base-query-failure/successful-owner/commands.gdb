set pagination off
break emit_debug_base_fields
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/debug-base-query-native-policy-4m9bbme7/stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/debug-base-query-native-policy-4m9bbme7/stderr.bin
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
set $group = (VirtualTableGroup *)calloc(1,sizeof(VirtualTableGroup))
set $group->tag = $owner
set $group->virtual_pointer_offset = 4
set $group->virtual_base_table_offset = 8
set $pointer = (CppSymbol *)calloc(1,sizeof(CppSymbol))
set $ptype = (CType *)calloc(1,sizeof(CType))
set $ptype->flags = 4
set $ptype->debug_index = 0x1234
set $pointer->type = $ptype
set $group->virtual_base_pointer = $pointer
set $groups->items = (void **)calloc(1,sizeof(void *))
set $groups->items[0] = $group
set $groups->count = 1
disable 1
break pad_debug_field
continue
python
assert gdb.newest_frame().name()=="pad_debug_field"
end
return (unsigned)0
disable 2
finish
python
frame=gdb.newest_frame()
assert int(gdb.parse_and_eval("*$counter"))==1
assert frame.name()=="emit_debug_field_list"
assert int(gdb.parse_and_eval("$eax"))==1
assert int(gdb.parse_and_eval("debug_field_bytes"))==12
assert int(gdb.parse_and_eval("*(unsigned short *)(debug_record_buffer+4)"))==0x401
assert int(gdb.parse_and_eval("*(unsigned short *)(debug_record_buffer+8)"))==0x1234
assert int(gdb.parse_and_eval("*(unsigned short *)(debug_record_buffer+12)"))==4
assert int(gdb.parse_and_eval("*(unsigned short *)(debug_record_buffer+14)"))==2
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/debug-base-query-native-policy-4m9bbme7/observation.json',"w").write(json.dumps({'status': 'pass', 'mode': 'successful actual owner control', 'synthetic_native_only': True, 'unaccepted_padding': 'forced zero return only for focused control', 'public_reachability': 'UNKNOWN'},indent=2)+"\n")
end
