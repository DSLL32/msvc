"""Actual matcher/callback and ordinary public signature call observations."""
from allocation_state import symbols

def definitions(binary,output):
 names=symbols(binary)
 code=r'''option_match_entries=[]
option_next_entries=[]
option_next_returns=[]
signature_call_entries=[]
signature_call_returns=[]
def signature_owned_bytes(phase):
    arena=gdb.parse_and_eval('c1_bss.arenas[1]');base=int(arena['base']);size=int(arena['committed_bytes']);assert base and size==32768
    open(OPTION_OUTPUT+'/signature-call-'+phase+'-vector.bin','wb').write(gdb.selected_inferior().read_memory(base,size).tobytes())
class OptionMatchEntry(gdb.Breakpoint):
    def stop(self):
        row=vector_abi(vector_snapshot(),5);caller=gdb.newest_frame().older()
        row['args']=dict(zip(('rules','argument','next','copy'),row['words'][1:]));row['caller']['args']={key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv')}
        identity=gdb.parse_and_eval('*c1_host_identity')
        row.update(module_identity=dict({key:identity[key].string()for key in ('name','path','sha256')},flags=int(identity['flags'])),module_token=int(gdb.parse_and_eval('(unsigned int)c1_host_module')))
        option_match_entries.append(row);return False
OptionMatchEntry('*0x%x'%OPTION_MATCH_ENTRY)
class OptionNextReturn(gdb.Breakpoint):
    def stop(self):
        row=vector_snapshot();row['result_bytes']=list(gdb.selected_inferior().read_memory(row['eax'],10).tobytes())
        option_next_returns.append(row);self.enabled=False;return False
class OptionNextEntry(gdb.Breakpoint):
    def stop(self):
        caller=gdb.newest_frame().older()
        if caller.name()!='match_option':return False
        row=vector_abi(vector_snapshot(),1);row['args']={};row['caller']['args']={key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('rules','argument','next','copy')}
        option_next_entries.append(row);OptionNextReturn('*0x%x'%row['words'][0]);return False
OptionNextEntry('*0x%x'%OPTION_NEXT_ENTRY)
class SignatureCallReturn(gdb.Breakpoint):
    def stop(self):
        row=vector_snapshot();signature_owned_bytes('return');row['argument_slots_after']=list(struct.unpack('<3I',gdb.selected_inferior().read_memory(row['esp'],12).tobytes()));row['locale']=native_locale_snapshot('signature-return')
        signature_call_returns.append(row);self.enabled=False;return False
class SignatureCallEntry(gdb.Breakpoint):
    def stop(self):
        row=vector_abi(vector_snapshot(),4);caller=gdb.newest_frame().older()
        assert caller.name()=='parse_command_line'
        row['args']=dict(zip(('bytes','byte_count','signature'),row['words'][1:]));row['caller']['args']={key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv')}
        row['input']=list(gdb.selected_inferior().read_memory(row['words'][1],row['words'][2]+1).tobytes())
        row['locale']=native_locale_snapshot('signature-entry')
        row['dependency_entry']=dict(stack=row['words'],esp=row['esp'],preserved=row['preserved'],input=row['input'][:-1],errors=row['errors'])
        identity=gdb.parse_and_eval('*c1_host_identity')
        row.update(module_identity=dict({key:identity[key].string()for key in ('name','path','sha256')},flags=int(identity['flags'])),module_token=int(gdb.parse_and_eval('(unsigned int)c1_host_module')))
        signature_owned_bytes('entry')
        signature_call_entries.append(row);SignatureCallReturn('*0x%x'%row['words'][0]);return False
SignatureCallEntry('*0x%x'%SIGNATURE_CALL_ENTRY)
'''
 return code.replace('OPTION_OUTPUT',repr(str(output))).replace('OPTION_MATCH_ENTRY',str(names['match_option'][0])).replace('OPTION_NEXT_ENTRY',str(names['next_argument'][0])).replace('SIGNATURE_CALL_ENTRY',str(names['recon_mspdb_SigForPbCb'][0]))

def final_observation(output):
 return '\n'.join("open(%r,'w').write(json.dumps(%s,indent=2)+'\\n')"%(str(output/file),value)for file,value in [('option-match-entries.json','option_match_entries'),('option-next-entries.json','option_next_entries'),('option-next-returns.json','option_next_returns'),('signature-call-entries.json','signature_call_entries'),('signature-call-returns.json','signature_call_returns')])+'\n'
