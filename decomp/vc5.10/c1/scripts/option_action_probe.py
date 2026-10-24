"""Per-call action and matcher returns; all mutable state read at actual stops."""
from allocation_state import symbols

def definitions(binary,output):
    names=symbols(binary)
    code=r'''option_action_entries=[]
option_action_returns=[]
option_match_returns=[]
class ActionReturn(gdb.Breakpoint):
    def __init__(self,address,index):
        super(ActionReturn,self).__init__('*0x%x'%address);self.index=index
    def stop(self):
        row=vector_snapshot();row['index']=self.index
        option_action_returns.append(row);self.enabled=False;return False
class ActionEntry(gdb.Breakpoint):
    def stop(self):
        row=vector_abi(vector_snapshot(),4);caller=gdb.newest_frame().older()
        row['args']=dict(zip(('rule','value','copy'),row['words'][1:]))
        row['caller']['args']={key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('rules','argument','next','copy')}
        identity=gdb.parse_and_eval('*c1_host_identity')
        row.update(module_identity=dict({key:identity[key].string()for key in ('name','path','sha256')},flags=int(identity['flags'])),module_token=int(gdb.parse_and_eval('(unsigned int)c1_host_module')))
        row['index']=len(option_action_entries)
        row['value_bytes']=vector_c_string(row['args']['value'])if row['args']['value']else[]
        option_action_entries.append(row);ActionReturn(row['words'][0],row['index']);return False
ActionEntry('*0x%x'%ACTION_ENTRY)
class MatchReturn(gdb.Breakpoint):
    def __init__(self,address,index):
        super(MatchReturn,self).__init__('*0x%x'%address);self.index=index
    def stop(self):
        row=vector_snapshot();row['index']=self.index
        option_match_returns.append(row);self.enabled=False;return False
class MatchReturnEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('(unsigned int)$esp'))
        address=struct.unpack('<I',gdb.selected_inferior().read_memory(esp,4).tobytes())[0]
        MatchReturn(address,len(option_match_returns));return False
MatchReturnEntry('*0x%x'%MATCH_ENTRY)
'''
    return code.replace('ACTION_ENTRY',str(names['apply_option_action'][0])).replace('MATCH_ENTRY',str(names['match_option'][0]))

def final_observation(output):
    return '\n'.join("open(%r,'w').write(json.dumps(%s,indent=2)+'\\n')"%(str(output/file),value)for file,value in [('option-action-entries.json','option_action_entries'),('option-action-returns.json','option_action_returns'),('option-match-returns.json','option_match_returns')])+'\n'
