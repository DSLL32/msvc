set pagination off
set confirm off
set disable-randomization off
python
import gdb,json
flag_count = 0
def record_flag_words(count,boundary):
    row=[count,boundary,int(gdb.parse_and_eval('scanner_flags')),int(gdb.parse_and_eval('parser_flags'))]
    with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/asm-parser-finish-final/flag-words.jsonl','a') as log:log.write(json.dumps(row)+'\n')
    if True:
        symbol=gdb.parse_and_eval('function_symbol')
        if not int(symbol):row=[count,boundary,None]
        else:
            value=symbol.dereference()
            row=[count,boundary,int(value['kind']),int(value['flags']),int(value['index']),int(value['attributes']),value['name'].string()]
        with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/asm-parser-finish-final/function-symbols.jsonl','a') as log:log.write(json.dumps(row)+'\n')
class FlagFinish(gdb.FinishBreakpoint):
    def __init__(self,count):
        super().__init__(gdb.newest_frame(),internal=True)
        self.count=count
        if False:
            self.handle=int(gdb.parse_and_eval('handle'))
            self.timestamp=gdb.parse_and_eval('timestamp')
    def stop(self):
        if False:
            row=[self.count,self.handle,int(self.return_value),int(self.timestamp.dereference())]
            with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/asm-parser-finish-final/source-timestamps.jsonl','a') as log: log.write(json.dumps(row)+'\n')
        record_flag_words(self.count,'finish')
        return False
class FlagEntry(gdb.Breakpoint):
    def stop(self):
        global flag_count
        record_flag_words(flag_count,'entry')
        FlagFinish(flag_count)
        flag_count+=1
        return False
FlagEntry('finish_function_definition',internal=True)
end
python
import gdb
native_exit = None
def record_native_exit(event):
    global native_exit
    native_exit = getattr(event,'exit_code',None)
gdb.events.exited.connect(record_native_exit)
end
run >/root/.ai-tmp/msvc2/vc1.0/c1/validation/asm-parser-finish-final/stdout 2>&1
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
    for item in []:
        slot=item['stack_slot']
        value=struct.unpack('<I',bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('$ebp'))+4*(slot+1),4)))[0]
        row=dict(name=item['name'],stack_slot=slot,value=value,kind=item.get('kind','u32'))
        if item.get('formal'):
            row['formal_value']=int(gdb.parse_and_eval(item['formal'])) & 0xffffffff
            assert row['formal_value']==value,('native formal/ABI slot differs',row)
        state['abi_inputs'].append(row)
    state['abi_globals']=[]
    for item in []:
        value=int(gdb.parse_and_eval(item['name'])) & ((1 << (8*item['bytes']))-1)
        state['abi_globals'].append(dict(name=item['name'],original=item['original'],bytes=item['bytes'],value=value))
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
    if False:
        operand=gdb.parse_and_eval('asm_operand').dereference()
        address=operand['address'].dereference()
        symbol=address['symbol']
        row=[int(gdb.parse_and_eval('asm_operand_count')),int(operand['size']),int(operand['flags']),int(address['displacement']),int(address['address_flags']),int(address['flags']),int(address['first_register']),int(address['second_register']),int(bool(int(symbol))),0,0]
        name=''
        if int(symbol):
            row[9]=int(symbol.dereference()['storage'][0]);row[10]=int(symbol.dereference()['index'])
            name=symbol.dereference()['name'].string()
        state['asm_operand_state']=row
        state['asm_operand_symbol_name']=name
    state['label_stack_count']=int(gdb.parse_and_eval('label_stack_count'))
    state['scanner_flags']=int(gdb.parse_and_eval('scanner_flags'))
    state['exception_scope_depth']=[int(gdb.parse_and_eval('exception_scope_depth[%d]'%i)) for i in range(3)]
else:
    state['frontier']=None
with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/asm-parser-finish-final/state.json','w') as out:json.dump(state,out,indent=2)
end
