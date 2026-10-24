"""Physical vector and arena1 commit observations, separate from pool prefix."""
from allocation_state import symbols

def definitions(binary,output):
 names=symbols(binary)
 code=r'''vector_entries=[]
vector_returns=[]
vector_commit_entries=[]
vector_commit_returns=[]
vector_api_entries=[]
vector_api_returns=[]
def vector_snapshot():
    return {'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),
      'eax':int(gdb.parse_and_eval('(unsigned int)$eax')),'ecx':int(gdb.parse_and_eval('(unsigned int)$ecx')),
      'preserved':pool_cycle_preserved(),'fp':capture_x87(),'errors':[int(gdb.parse_and_eval('(int)errno')),int(gdb.parse_and_eval('recon_doserrno')),int(gdb.parse_and_eval('last_error'))],
      'compiler_state':capture_modeled(),'invocation':capture_invocation(),'crt_state':capture_command_line_crt_state()}
def vector_abi(row,count):
    frame=gdb.newest_frame();caller=frame.older();block=gdb.block_for_pc(row['pc'])
    while block and not block.function:block=block.superblock
    row.update(symbol=frame.name(),formal_inventory=[{'name':s.name,'type':str(s.type)}for s in block if s.is_argument],
      words=list(struct.unpack('<%dI'%count,gdb.selected_inferior().read_memory(row['esp'],count*4).tobytes())),
      caller={'symbol':caller.name(),'pc':int(caller.pc())})
    return row
class VectorReturn(gdb.Breakpoint):
    def stop(self):
        row=vector_snapshot();address=row['eax'];arena=gdb.parse_and_eval('c1_bss.arenas[1]');base=int(arena['base']);size=int(arena['committed_bytes'])
        raw=gdb.selected_inferior().read_memory(address,12).tobytes();count,capacity,kind,items=struct.unpack('<hhII',raw)
        row.update(header=[count,capacity,kind,items],address=address,raw_header=raw.hex(),items=list(gdb.selected_inferior().read_memory(items,capacity*4).tobytes()))
        open(VECTOR_OUTPUT+'/vector-committed-%03d.bin'%len(vector_returns),'wb').write(gdb.selected_inferior().read_memory(base,size).tobytes())
        vector_returns.append(row);self.enabled=False;return False
class VectorEntry(gdb.Breakpoint):
    def stop(self):
        row=vector_abi(vector_snapshot(),3);frame=gdb.newest_frame();caller=frame.older()
        row['args']={'count':row['words'][1],'mode':row['words'][2]};row['caller']['args']={key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv')}
        identity=gdb.parse_and_eval('*c1_host_identity')
        row.update(module_identity=dict({key:identity[key].string()for key in ('name','path','sha256')},flags=int(identity['flags'])),module_token=int(gdb.parse_and_eval('(unsigned int)c1_host_module')))
        row['layout']={'size':int(gdb.parse_and_eval('sizeof(CompilerVector)')),'offsets':{field:int(gdb.parse_and_eval('(unsigned int)&((CompilerVector*)0)->'+field))for field in ('count','capacity','arena_kind','items')}}
        vector_entries.append(row);VectorReturn('*0x%x'%row['words'][0]);return False
VectorEntry('*0x%x'%VECTOR_ENTRY)
class VectorCommitReturn(gdb.Breakpoint):
    def stop(self):
        vector_commit_returns.append(vector_snapshot());self.enabled=False;return False
class VectorCommitEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller.name()!='new_option_vector':return False
        row=vector_abi(vector_snapshot(),1);row['args']={'this':row['ecx']}
        row['caller']['args']={key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('count','mode')}
        vector_commit_entries.append(row);VectorCommitReturn('*0x%x'%row['words'][0]);return False
VectorCommitEntry('*0x%x'%VECTOR_COMMIT)
class VectorApiReturn(gdb.Breakpoint):
    def __init__(self,address,args):
        super(VectorApiReturn,self).__init__('*0x%x'%address);self.args=args
    def stop(self):
        row=vector_snapshot();row['maps']=open('/proc/%d/maps'%gdb.selected_inferior().pid).read();view=gdb.parse_and_eval('allocations');views=[]
        while int(view):
            item=view.dereference();views.append({'address':int(item['address']),'length':int(item['length'])});view=item['next']
        row['allocations']=views;vector_api_returns.append(row)
        open(VECTOR_OUTPUT+'/vector-commit-zero-000.bin','wb').write(gdb.selected_inferior().read_memory(self.args[0],self.args[1]).tobytes())
        self.enabled=False;return False
class VectorApiEntry(gdb.Breakpoint):
    def stop(self):
        caller=gdb.newest_frame().older()
        if caller.name()!='CompilerArena::CommitExtent' or int(caller.read_var('this').cast(gdb.lookup_type('unsigned int')))!=int(gdb.parse_and_eval('(unsigned int)&c1_bss.arenas[1]')):return False
        row=vector_abi(vector_snapshot(),5);row['args']=row['words'][1:];row['caller']['args']={'this':int(caller.read_var('this').cast(gdb.lookup_type('unsigned int')))}
        vector_api_entries.append(row);VectorApiReturn(row['words'][0],row['args']);return False
VectorApiEntry('*0x%x'%VECTOR_API)
argument_cursor_entries=[]
argument_cursor_returns=[]
class ArgumentCursorReturn(gdb.Breakpoint):
    def stop(self):
        row=vector_snapshot();row['result_bytes']=list(gdb.selected_inferior().read_memory(row['eax'],4).tobytes())
        argument_cursor_returns.append(row);self.enabled=False;return False
class ArgumentCursorEntry(gdb.Breakpoint):
    def stop(self):
        row=vector_abi(vector_snapshot(),1);caller=gdb.newest_frame().older()
        row['args']={};row['caller']['args']={key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv')}
        identity=gdb.parse_and_eval('*c1_host_identity')
        row.update(module_identity=dict({key:identity[key].string()for key in ('name','path','sha256')},flags=int(identity['flags'])),module_token=int(gdb.parse_and_eval('(unsigned int)c1_host_module')))
        argument_cursor_entries.append(row);ArgumentCursorReturn('*0x%x'%row['words'][0]);return False
ArgumentCursorEntry('*0x%x'%ARGUMENT_CURSOR_ENTRY)
'''
 return code.replace('VECTOR_OUTPUT',repr(str(output))).replace('VECTOR_ENTRY',str(names['new_option_vector'][0])).replace('VECTOR_COMMIT',str(names['_ZN13CompilerArena12CommitExtentEv'][0])).replace('VECTOR_API',str(names['recon_VirtualAlloc'][0])).replace('ARGUMENT_CURSOR_ENTRY',str(names['next_argument'][0]))

def final_observation(output):
 return '\n'.join("open(%r,'w').write(json.dumps(%s,indent=2)+'\\n')"%(str(output/file),value)for file,value in [('vector-entries.json','vector_entries'),('vector-returns.json','vector_returns'),('vector-commit-entries.json','vector_commit_entries'),('vector-commit-returns.json','vector_commit_returns'),('vector-api-entries.json','vector_api_entries'),('vector-api-returns.json','vector_api_returns'),('argument-cursor-entries.json','argument_cursor_entries'),('argument-cursor-returns.json','argument_cursor_returns')])+'\n'
