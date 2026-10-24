"""Physical native entry of the C-linkage alias initializer, before its body."""
from allocation_state import symbols

def definitions(binary,output):
 address=symbols(binary)['initialize_include_aliases'][0]
 checksum=symbols(binary)['initialize_checksums'][0]
 return '''alias_initializer_entries=[]
alias_checksum_entries=[]
class ActualAliasInitializerEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        pc=int(gdb.parse_and_eval('(unsigned int)$pc'));esp=int(gdb.parse_and_eval('(unsigned int)$esp'))
        returned=struct.unpack('<I',gdb.selected_inferior().read_memory(esp,4).tobytes())[0]
        block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        alias_initializer_entries.append({'pc':pc,'esp':esp,'return':returned,'symbol':frame.name(),'boundary_rva':0x12df6,
          'args':{},'formal_inventory':[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument],
          'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv')}},
          'fp':capture_x87(),'preserved':{key:int(gdb.parse_and_eval('(unsigned int)$'+key))for key in ('ebx','esi','edi','ebp')},
          'compiler_state':capture_modeled(),'invocation':capture_invocation(),'crt_state':capture_command_line_crt_state()})
        return False
ActualAliasInitializerEntry('*0x%x')
class ActualAliasChecksumEntry(gdb.Breakpoint):
    def stop(self):
        pc=int(gdb.parse_and_eval('(unsigned int)$pc'));frame=gdb.newest_frame()
        block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        alias_checksum_entries.append({'pc':pc,'symbol':frame.name(),
          'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),
          'formal_inventory':[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument],
          'preserved':{key:int(gdb.parse_and_eval('(unsigned int)$'+key))for key in ('ebx','esi','edi','ebp')}})
        return False
ActualAliasChecksumEntry('*0x%x')
'''%(address,checksum)

def final_observation(output):
 return '''alias_root=int(gdb.parse_and_eval('(unsigned int)c1_data.options.include_aliases'))
alias_base=int(gdb.parse_and_eval('(unsigned int)c1_bss.arenas[0].base'))
alias_committed=int(gdb.parse_and_eval('c1_bss.arenas[0].committed_bytes'))
assert alias_root>=alias_base and alias_root-alias_base<=alias_committed-4
value['alias_root']={'address':alias_root,'head':int(gdb.parse_and_eval('(unsigned int)c1_data.options.include_aliases->head')),'size':int(gdb.parse_and_eval('sizeof(*c1_data.options.include_aliases)'))}
assert len(alias_checksum_entries)==1
value['alias_checksum_entry']=alias_checksum_entries[0]
value['alias_esp']=alias_checksum_entries[0]['esp']
value['alias_preserved']=alias_checksum_entries[0]['preserved']
open(%r,'wb').write(gdb.selected_inferior().read_memory(alias_root,4).tobytes())
open(%r,'wb').write(gdb.selected_inferior().read_memory(alias_base,alias_committed).tobytes())
open(%r,'w').write(json.dumps(alias_initializer_entries,indent=2)+'\\n')
'''%(str(output/'include-alias-root-000.bin'),str(output/'include-alias-committed-000.bin'),str(output/'alias-initializer-entries.json'))
