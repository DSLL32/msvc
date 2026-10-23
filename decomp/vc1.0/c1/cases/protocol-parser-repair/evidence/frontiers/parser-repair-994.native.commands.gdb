set pagination off
set confirm off
set disable-randomization off
break recover_parser_token
python
import gdb
native_exit = None
def record_native_exit(event):
    global native_exit
    native_exit = getattr(event,'exit_code',None)
gdb.events.exited.connect(record_native_exit)
end
run >/root/.ai-tmp/msvc2/vc1.0/c1/validation/parser-repair-994/stdout 2>&1
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

    state['abi_inputs']=[]
    for item in [{'name': 'token', 'formal': 'token', 'stack_slot': 1}, {'name': 'stop_semicolon', 'stack_slot': 2}, {'name': 'stop_brace', 'stack_slot': 3}, {'name': 'sentinel', 'stack_slot': 4}]:
        slot=item['stack_slot']
        value=struct.unpack('<I',bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('$ebp'))+4*(slot+1),4)))[0]
        row=dict(name=item['name'],stack_slot=slot,value=value)
        if item.get('formal'):
            row['formal_value']=int(gdb.parse_and_eval(item['formal'])) & 0xffffffff
            assert row['formal_value']==value,('native formal/ABI slot differs',row)
        state['abi_inputs'].append(row)
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
with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/parser-repair-994/state.json','w') as out:json.dump(state,out,indent=2)
end
