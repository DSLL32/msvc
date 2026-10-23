set pagination off
set confirm off
set disable-randomization off
break *clear_packing
run >/root/.ai-tmp/msvc2/vc1.0/c1/validation/hdrstop-packing-return/stdout 2>&1
python
import gdb,json,struct
def u(x):return int(gdb.parse_and_eval(x)) & 0xffffffff
inf=gdb.selected_inferior();sp=u('$esp');ret,arg=struct.unpack('<II',bytes(inf.read_memory(sp,8)))
before=dict(esp=sp,return_address=ret,argument=arg,object=bytes(inf.read_memory(arg,12)).hex(),scanner_flags=u('scanner_flags'),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']})
gdb.Breakpoint('*'+str(ret),temporary=True)
end
delete 1
continue
python
row=dict(before=before,esp=u('$esp'),object=bytes(inf.read_memory(arg,12)).hex(),caller=gdb.newest_frame().name(),scanner_flags=u('scanner_flags'),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']})
assert row['esp']-sp==4 and row['object']=='00'*12
assert row['registers']==before['registers'] and row['caller']=='push_packing' and row['scanner_flags']==before['scanner_flags']
with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/hdrstop-packing-return/registers.json','w')as out:json.dump(row,out,indent=2)
end
