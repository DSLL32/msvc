set pagination off
set confirm off
set disable-randomization off
break synchronize_pch_pragma
run >/root/.ai-tmp/msvc2/vc1.0/c1/validation/hdrstop-strdup-return-1000/stdout 2>&1
delete 1
break *strdup
continue
python
import gdb,json,struct
def u(x):return int(gdb.parse_and_eval(x)) & 0xffffffff
def text(p):return gdb.Value(p).cast(gdb.lookup_type('char').pointer()).string(encoding='latin1')
inf=gdb.selected_inferior();sp=u('$esp');ret,arg=struct.unpack('<II',bytes(inf.read_memory(sp,8)))
assert gdb.newest_frame().older().name()=='synchronize_pch_pragma'
assert arg==u('text_work_cursor')
before=dict(esp=sp,return_address=ret,argument=arg,text=text(arg),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']})
gdb.Breakpoint('*'+str(ret),temporary=True)
end
delete 2
continue
python
row=dict(before=before,esp=u('$esp'),eax=u('$eax'),text=text(u('$eax')),caller=gdb.newest_frame().name(),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']})
assert row['esp']-sp==4 and row['eax']!=arg and row['text']==before['text']=='hdrstop.pch'
assert row['registers']==before['registers'] and row['caller']=='synchronize_pch_pragma'
with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/hdrstop-strdup-return-1000/registers.json','w')as out:json.dump(row,out,indent=2)
end
