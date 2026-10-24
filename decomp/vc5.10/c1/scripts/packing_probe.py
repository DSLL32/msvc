"""Actual packing initializer entry before its first allocator call."""
from allocation_state import symbols

def definitions(binary,output):
 names=symbols(binary)
 code=r'''packing_entries=[]
class PackingEntry(gdb.Breakpoint):
    def stop(self):
        assert not packing_entries
        row=vector_abi(vector_snapshot(),1);row['args']={}
        caller=gdb.newest_frame().older();assert caller.name()=='parse_command_line'
        row['caller']['args']={key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv')}
        identity=gdb.parse_and_eval('*c1_host_identity')
        row.update(module_identity=dict({key:identity[key].string()for key in ('name','path','sha256')},flags=int(identity['flags'])),module_token=int(gdb.parse_and_eval('(unsigned int)c1_host_module')))
        row['locale']=native_locale_snapshot('packing-entry');packing_entries.append(row);return False
PackingEntry('*0x%x'%PACKING_ENTRY)
'''
 return code.replace('PACKING_ENTRY',str(names['initialize_packing'][0]))

def final_observation(output):
 return "open(%r,'w').write(json.dumps(packing_entries,indent=2)+'\\n')\n"%str(output/'packing-entries.json')
