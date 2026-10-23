set pagination off
set confirm off
set disable-randomization off
break *require_complete_type
run >/root/.ai-tmp/msvc2/vc1.0/c1/validation/dispatch-tag-return-1007/stdout 2>&1
python
import gdb,json,struct
def u(x):return int(gdb.parse_and_eval(x)) & 0xffffffff
inf=gdb.selected_inferior();sp=u('$esp');ret,arg=struct.unpack('<II',bytes(inf.read_memory(sp,8)));tag=struct.unpack('<I',bytes(inf.read_memory(arg+8,4)))[0]
before=dict(esp=sp,return_address=ret,argument=arg,tag=tag,flags=bytes(inf.read_memory(tag+1,1))[0],errors=u('error_count'),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']})
gdb.Breakpoint('*'+str(ret),temporary=True)
end
delete 1
continue
python
row=dict(before=before,esp=u('$esp'),tag=struct.unpack('<I',bytes(inf.read_memory(arg+8,4)))[0],flags=bytes(inf.read_memory(tag+1,1))[0],errors=u('error_count'),caller=gdb.newest_frame().name(),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']})
assert row['esp']-sp==8 and row['tag']==tag and row['flags']==(before['flags']|128) and row['errors']==before['errors']+1
assert row['registers']==before['registers'] and row['caller']=='complete_type_size'
with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/dispatch-tag-return-1007/registers.json','w')as out:json.dump(row,out,indent=2)
end
