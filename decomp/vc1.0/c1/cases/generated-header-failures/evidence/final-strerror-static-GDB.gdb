set pagination off
set confirm off
set disable-randomization off
python
import gdb
native_exit = None
def record_native_exit(event):
    global native_exit
    native_exit = getattr(event,'exit_code',None)
gdb.events.exited.connect(record_native_exit)
end
run >/root/.ai-tmp/msvc2/vc1.0/c1/validation/generated-strerror-return-987/stdout 2>&1
python
import gdb,json
reason=gdb.parse_and_eval('*(char **)((char *)&code+12)')
static=gdb.parse_and_eval('recon_strerror::text')
row=dict(reason=int(reason),static=int(static.address),bytes=bytes(gdb.selected_inferior().read_memory(int(reason),128)).hex(),errno=int(gdb.parse_and_eval('(int)errno')))
assert row['reason']==row['static'] and row['errno']==2
with open('/root/.ai-tmp/msvc2/vc1.0/c1/generated-strerror-result.json','w') as out:json.dump(row,out,indent=2)
end
python
import gdb,json,struct
state={'buffers':{},'frames':[],'exit':native_exit}
if gdb.selected_inferior().pid:
    frame=gdb.selected_frame()
    state['frontier']=frame.name()
    state['pc']=int(gdb.parse_and_eval('$pc'))
    if state['frontier']=='recon_qsort':
        state['qsort_contract']={
            'count':int(gdb.parse_and_eval('count')),
            'width':int(gdb.parse_and_eval('width')),
            'array_nonnull':bool(int(gdb.parse_and_eval('array'))),
            'callback_is_legacy_enumerator':int(gdb.parse_and_eval('compare'))==int(gdb.parse_and_eval('&compare_legacy_enumerators'))}

    while frame and len(state['frames'])<12:
        state['frames'].append(frame.name())
        frame=frame.older()
    for name in ['expression_file', 'symbol_file', 'line_file', 'global_file', 'debug_file', 'pch_file', 'browser_output']:
        address=int(gdb.parse_and_eval(name))
        if not address:
            state['buffers'][name]=None
            continue
        ptr,count,base=struct.unpack('<III',bytes(gdb.selected_inferior().read_memory(address,12)))
        assert 0<=ptr-base<=65536,(name,ptr,base)
        state['buffers'][name]=(bytes(gdb.selected_inferior().read_memory(base,ptr-base)).hex() if ptr>base else '')
    state['label_stack_count']=int(gdb.parse_and_eval('label_stack_count'))
    state['scanner_flags']=int(gdb.parse_and_eval('scanner_flags'))
    state['exception_scope_depth']=[int(gdb.parse_and_eval('exception_scope_depth[%d]'%i)) for i in range(3)]
else:
    state['frontier']=None
with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/generated-strerror-return-987/state.json','w') as out:json.dump(state,out,indent=2)
end
