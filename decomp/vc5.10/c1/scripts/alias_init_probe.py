"""Physical native entry of the C-linkage alias initializer, before its body."""
from allocation_state import symbols

def definitions(binary,output):
 address=symbols(binary)['initialize_include_aliases'][0]
 checksum=symbols(binary)['initialize_checksums'][0]
 return '''alias_initializer_entries=[]
alias_memory_boundary=None
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
        global alias_memory_boundary
        assert alias_memory_boundary is None
        pc=int(gdb.parse_and_eval('(unsigned int)$pc'));frame=gdb.newest_frame()
        block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        alias_checksum_entries.append({'pc':pc,'symbol':frame.name(),
          'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),
          'formal_inventory':[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument],
          'preserved':{key:int(gdb.parse_and_eval('(unsigned int)$'+key))for key in ('ebx','esi','edi','ebp')}})
        root=int(gdb.parse_and_eval('(unsigned int)c1_data.options.include_aliases'));base=int(gdb.parse_and_eval('(unsigned int)c1_bss.arenas[0].base'));size=int(gdb.parse_and_eval('c1_bss.arenas[0].committed_bytes'))
        assert size==65536 and base<=root and root-base<=size-4
        raw=gdb.selected_inferior().read_memory(base,size).tobytes();head=gdb.selected_inferior().read_memory(root,4).tobytes()
        open(%r,'xb').write(raw);open(%r,'xb').write(head)
        alias_memory_boundary={'schema':1,'boundary':'initialize_checksums','pc':pc,'caller':gdb.newest_frame().older().name(),'return':struct.unpack('<I',gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('(unsigned int)$esp')),4).tobytes())[0],'owner':'c1_bss.arenas[0]','base':base,'bytes':size,'reads':1,'root':{'address':root,'head':struct.unpack('<I',head)[0],'size':4},'sha256':__import__('hashlib').sha256(raw).hexdigest()}
        open(%r,'x').write(json.dumps(alias_memory_boundary,indent=2)+'\\n')
        return False
ActualAliasChecksumEntry('*0x%x')
'''%(address,str(output/'include-alias-committed-000.bin'),str(output/'include-alias-root-000.bin'),str(output/'include-alias-memory-boundary.json'),checksum)

def final_observation(output):
 return '''assert alias_memory_boundary is not None
value['alias_root']=alias_memory_boundary['root']
assert len(alias_checksum_entries)==1
value['alias_checksum_entry']=alias_checksum_entries[0]
value['alias_esp']=alias_checksum_entries[0]['esp']
value['alias_preserved']=alias_checksum_entries[0]['preserved']
open(%r,'w').write(json.dumps(alias_initializer_entries,indent=2)+'\\n')
'''%(str(output/'alias-initializer-entries.json'),)
