"""Physical checksum entry/return and subsequent public locale entry."""
from allocation_state import symbols

def definitions(binary,output):
 address=symbols(binary)['initialize_checksums'][0]
 return '''checksum_entries=[]
checksum_returns=[]
class ActualChecksumReturn(gdb.Breakpoint):
    def stop(self):
        checksum_returns.append({'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),
          'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),
          'preserved':{key:int(gdb.parse_and_eval('(unsigned int)$'+key))for key in ('ebx','esi','edi','ebp')},
          'fp':capture_x87(),'compiler_state':capture_modeled(),'crt_state':capture_command_line_crt_state()})
        self.enabled=False
        return False
class ActualChecksumEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        pc=int(gdb.parse_and_eval('(unsigned int)$pc'));esp=int(gdb.parse_and_eval('(unsigned int)$esp'))
        returned=struct.unpack('<I',gdb.selected_inferior().read_memory(esp,4).tobytes())[0]
        block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        root=int(gdb.parse_and_eval('(unsigned int)c1_data.options.include_aliases'))
        preserved={key:int(gdb.parse_and_eval('(unsigned int)$'+key))for key in ('ebx','esi','edi','ebp')}
        formals=[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument]
        physical={'pc':pc,'symbol':frame.name(),'esp':esp,'formal_inventory':formals,'preserved':preserved}
        checksum_entries.append({'pc':pc,'esp':esp,'return':returned,'symbol':frame.name(),'boundary_rva':0x12e1c,
          'args':{},'formal_inventory':formals,
          'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv')}},
          'fp':capture_x87(),'preserved':preserved,'alias_checksum_entry':physical,'alias_esp':esp,'alias_preserved':preserved,
          'alias_root':{'address':root,'head':int(gdb.parse_and_eval('(unsigned int)c1_data.options.include_aliases->head')),'size':int(gdb.parse_and_eval('sizeof(*c1_data.options.include_aliases)'))},
          'compiler_state':capture_modeled(),'invocation':capture_invocation(),'crt_state':capture_command_line_crt_state()})
        checksum_entries[-1]['feedback_taps']=list(struct.unpack('<8I',gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('(unsigned int)feedback_taps')),32).tobytes()))
        ActualChecksumReturn('*0x%%x'%%returned)
        return False
ActualChecksumEntry('*0x%x')
'''%address

def final_observation(output):
 return '''value['public_entry']={'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),
 'stack':list(struct.unpack('<3I',gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('(unsigned int)$esp')),12).tobytes())),
 'return':struct.unpack('<I',gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('(unsigned int)$esp')),4).tobytes())[0],
 'locale_bytes':list(gdb.selected_inferior().read_memory(args['locale'],1).tobytes()),
 'preserved':{key:int(gdb.parse_and_eval('(unsigned int)$'+key))for key in ('ebx','esi','edi','ebp')}}
open(%r,'w').write(json.dumps(checksum_entries,indent=2)+'\\n')
open(%r,'w').write(json.dumps(checksum_returns,indent=2)+'\\n')
'''%(str(output/'checksum-entries.json'),str(output/'checksum-returns.json'))
