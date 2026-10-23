set pagination off
set confirm off
set disable-randomization off
break *enclosing_declaration_scope
run >/root/.ai-tmp/msvc2/vc1.0/c1/validation/registry-scope-return/stdout 2>&1
python
import gdb,json,struct
def u(x):return int(gdb.parse_and_eval(x)) & 0xffffffff
inf=gdb.selected_inferior();sp=u('$esp');ret=struct.unpack('<I',bytes(inf.read_memory(sp,4)))[0]
links=[];scope=gdb.parse_and_eval('current_scope')
while int(scope):
    item=scope.dereference();links.append([int(scope),int(item['next']),int(item['flags']) & 255])
    if links[-1][2]!=1:break
    assert len(links)<16
    scope=item['next']
before=dict(esp=sp,return_address=ret,links=links,selected=u('scope_directory[scope_depth]'),depth=u('scope_depth'),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']},scanner=u('scanner_flags'),parser=u('parser_flags'))
gdb.Breakpoint('*'+str(ret),temporary=True)
end
delete 1
continue
python
row=dict(before=before,esp=u('$esp'),result=u('$eax'),caller=gdb.newest_frame().name(),registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']},scanner=u('scanner_flags'),parser=u('parser_flags'))
assert row['esp']-sp==4 and row['result']==links[-1][0] and before['selected']==row['result']
assert row['registers']==before['registers'] and row['caller']=='create_tag'
assert row['scanner']==before['scanner'] and row['parser']==before['parser']
with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/registry-scope-return/registers.json','w')as out:json.dump(row,out,indent=2)
end
