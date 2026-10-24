"""Actual packing initializer entry before its first allocator call."""
from allocation_state import symbols

def definitions(binary,output,allocation=False,frame=False):
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
 if allocation:code+=allocation_definitions(names,output,frame)
 return code.replace('PACKING_ENTRY',str(names['initialize_packing'][0]))

def final_observation(output,allocation=False,frame=False):
 result="open(%r,'w').write(json.dumps(packing_entries,indent=2)+'\\n')\n"%str(output/'packing-entries.json')
 if allocation:
  for name,value in [('packing-allocation-entries.json','packing_allocation_entries'),('packing-allocation-returns.json','packing_allocation_returns'),('packing-member-entries.json','packing_member_entries')]:result+="open(%r,'w').write(json.dumps(%s,indent=2)+'\\n')\n"%(str(output/name),value)
 if frame:
  result+="packing_frame_ready=packing_frame_snapshot(vector_snapshot(),'ready')\n"
  for name,value in [('packing-frame-returns.json','packing_frame_returns'),('packing-frame-ready.json','packing_frame_ready')]:result+="open(%r,'w').write(json.dumps(%s,indent=2)+'\\n')\n"%(str(output/name),value)
 return result

def allocation_definitions(names,output,frame):
 code=r'''packing_allocation_entries=[]
packing_allocation_returns=[]
packing_member_entries=[]
def packing_allocation_snapshot(row,phase):
    row['locale']=native_locale_snapshot('packing-allocation-'+phase)
    row['memory']=list(gdb.selected_inferior().read_memory(packing_allocation_entries[0]['memory_address'],8).tobytes())
    return row
class PackingAllocationReturn(gdb.Breakpoint):
    def __init__(self,address):super(PackingAllocationReturn,self).__init__('*0x%x'%address)
    def stop(self):
        packing_allocation_returns.append(packing_allocation_snapshot(vector_snapshot(),'return'));self.enabled=False;return False
class PackingAllocationEntry(gdb.Breakpoint):
    def stop(self):
        assert not packing_allocation_entries
        row=vector_abi(vector_snapshot(),2);row['args']={'bytes':row['words'][1]};row['caller']['args']={}
        identity=gdb.parse_and_eval('*c1_host_identity')
        row.update(module_identity=dict({key:identity[key].string()for key in ('name','path','sha256')},flags=int(identity['flags'])),module_token=int(gdb.parse_and_eval('(unsigned int)c1_host_module')))
        row['memory_address']=int(gdb.parse_and_eval('(unsigned int)c1_bss.arenas[0].cursor'))
        packing_allocation_entries.append(row);packing_allocation_snapshot(row,'entry');PackingAllocationReturn(row['words'][0]);return False
PackingAllocationEntry('*0x%x'%PACKING_ALLOCATOR)
class PackingMemberEntry(gdb.Breakpoint):
    def stop(self):
        assert not packing_member_entries
        row=packing_allocation_snapshot(vector_abi(vector_snapshot(),1),'member');row['args']={'this':row['ecx']};row['caller']['args']={}
        identity=gdb.parse_and_eval('*c1_host_identity')
        row.update(module_identity=dict({key:identity[key].string()for key in ('name','path','sha256')},flags=int(identity['flags'])),module_token=int(gdb.parse_and_eval('(unsigned int)c1_host_module')))
        row['object_layout']={'bytes':int(gdb.parse_and_eval('sizeof(CompilerPacking)')),'active':int(gdb.parse_and_eval('(unsigned int)&((CompilerPacking*)0)->active')),'free_frames':int(gdb.parse_and_eval('(unsigned int)&((CompilerPacking*)0)->free_frames'))}
        packing_member_entries.append(row);PACKING_FRAME_ENTER;return False
PackingMemberEntry('*0x%x'%PACKING_MEMBER)
'''
 if frame:
  before=r'''packing_frame_returns=[]
def packing_frame_snapshot(row,phase):
    entry=packing_member_entries[0]
    row['locale']=native_locale_snapshot('packing-frame-'+phase)
    row['object']=list(gdb.selected_inferior().read_memory(entry['args']['this'],8).tobytes())
    row['frame']=list(gdb.selected_inferior().read_memory(entry['new_frame_address'],12).tobytes())
    return row
class PackingFrameReturn(gdb.Breakpoint):
    def __init__(self,address):super(PackingFrameReturn,self).__init__('*0x%x'%address)
    def stop(self):
        packing_frame_returns.append(packing_frame_snapshot(vector_snapshot(),'return'));self.enabled=False;return False
'''
  code=before+code
  code=code.replace('PACKING_FRAME_ENTER',"row['new_frame_address']=int(gdb.parse_and_eval('(unsigned int)c1_bss.arenas[0].cursor'));row['frame_layout']={'bytes':int(gdb.parse_and_eval('sizeof(CompilerPackingFrame)')),'previous':int(gdb.parse_and_eval('(unsigned int)&((CompilerPackingFrame*)0)->previous')),'alignment':int(gdb.parse_and_eval('(unsigned int)&((CompilerPackingFrame*)0)->alignment')),'tag':int(gdb.parse_and_eval('(unsigned int)&((CompilerPackingFrame*)0)->tag'))};packing_frame_snapshot(row,'entry');PackingFrameReturn(row['words'][0])")
 else:code=code.replace('PACKING_FRAME_ENTER','pass')
 return code.replace('PACKING_ALLOCATOR',str(names['allocate_packing_memory'][0])).replace('PACKING_MEMBER',str(names['_ZN15CompilerPacking4PushEv'][0]))
