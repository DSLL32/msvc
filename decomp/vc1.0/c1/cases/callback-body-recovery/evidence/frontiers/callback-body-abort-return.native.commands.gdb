set pagination off
set confirm off
set disable-randomization off
break *abort_function_definition
run >/root/.ai-tmp/msvc2/vc1.0/c1/validation/callback-body-abort-return/stdout 2>&1
python
import gdb,json,struct
def u(x):return int(gdb.parse_and_eval(x)) & 0xffffffff
fields=['function_parameter_declarations','current_parameter','function_parameter_context','declaration_controls.fields.parameter_context','function_symbol']
def snapshot():return dict(values=[u(n)for n in fields],esp=u('$esp'),flags=[u('scanner_flags'),u('parser_flags')],registers={r:u('$'+r)for r in ['ebx','esi','edi','ebp']})
rows=[]
for index in range(2):
    before=snapshot();ret=struct.unpack('<I',bytes(gdb.selected_inferior().read_memory(before['esp'],4)))[0]
    gdb.Breakpoint('*'+str(ret),temporary=True);gdb.execute('disable 1');gdb.execute('continue')
    after=snapshot();caller=gdb.newest_frame().name()
    assert before['values']==after['values']==[0]*5
    assert after['flags']==[(before['flags'][0]|0x01000000)&~0x00080000,before['flags'][1]]
    assert after['esp']-before['esp']==4 and after['registers']==before['registers'] and caller=='prepare_function_definition'
    rows.append(dict(index=index,before=before,after=after,caller=caller))
    gdb.execute('enable 1')
    if index==0:gdb.execute('continue')
with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/callback-body-abort-return/registers.json','w')as out:json.dump(rows,out,indent=2)
end
