"""Native-only machine-bound x87 observations; no code or provider emulation."""
import json,struct
from pathlib import Path
import capstone
from capstone.x86_const import X86_REG_EBP
from allocation_state import symbols
from provenance import sha

def describe(binary):
 raw=binary.read_bytes();assert len(raw)>=52 and raw[:7]==b'\x7fELF\x01\x01\x01'
 assert struct.unpack_from('<HHI',raw,16)==(2,3,1) and struct.unpack_from('<H',raw,40)[0]==52
 section_table=struct.unpack_from('<I',raw,32)[0];entry_size,section_count=struct.unpack_from('<HH',raw,46);assert entry_size==40
 assert section_count>0 and section_table>=52 and section_table+section_count*40<=len(raw)
 sections=[struct.unpack_from('<10I',raw,section_table+i*40)for i in range(section_count)]
 assert all(s[3]+s[5]<=0x100000000 and (s[1]==8 or s[4]+s[5]<=len(raw))for s in sections)
 names=symbols(binary);start,size=names['arena_reservation_bytes']
 assert 0<start<0x100000000 and 0<size and start+size<=0x100000000
 matches=[s for s in sections if s[1]==1 and s[2]&4 and s[3]<=start and start+size<=s[3]+s[5]];assert len(matches)==1;section=matches[0];offset=section[4]+start-section[3];data=raw[offset:offset+size]
 decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);decoder.detail=True
 instructions=list(decoder.disasm(data,start));assert sum(i.size for i in instructions)==size;floating=[i for i in instructions if i.mnemonic.startswith('f')]
 assert [i.mnemonic for i in floating]==['fild','fild','fmulp','fld','fmulp','fnstcw','fldcw','fistp','fldcw']
 cast=floating[7];assert cast.operands[0].size==8 and cast.operands[0].mem.base==X86_REG_EBP and cast.operands[0].mem.index==0
 converted_pc=floating[-1].address+floating[-1].size
 metadata=dict(kind='actual ELF x87 conversion observer points',elf_sha256=sha(binary),function='arena_reservation_bytes',start=start,size=size,
  instructions=[dict(pc=i.address,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)for i in floating],converted_pc=converted_pc,result_ebp_displacement=cast.operands[0].mem.disp)
 return metadata

def probes(binary,output,reset_seed=False):
 metadata=describe(binary);names=symbols(binary);start=metadata['start']
 floating=metadata['instructions'];converted_pc=metadata['converted_pc']
 (output/'scaler-probe-metadata.json').write_text(json.dumps(metadata,indent=2)+'\n')
 code='''scaler_fp_records=[]
scaler_entries=[]
scaler_returns=[]
def capture_x87():
    cw=int(gdb.parse_and_eval('$fctrl'))&0xffff;sw=int(gdb.parse_and_eval('$fstat'))&0xffff;tag=int(gdb.parse_and_eval('$ftag'))&0xffff
    top=(sw>>11)&7
    registers=[gdb.parse_and_eval('$st%d'%i).bytes.hex()for i in range(8)]
    return {'cw':cw,'sw':sw,'tag':tag,'top':top,'registers':registers,'live':[{'logical':i,'tag':(tag>>(2*((top+i)&7)))&3,'raw80':registers[i]}for i in range(8)if ((tag>>(2*((top+i)&7)))&3)!=3]}
class ScalerFloatPoint(gdb.Breakpoint):
    def __init__(self,address,label):
        super(ScalerFloatPoint,self).__init__('*0x%x'%address);self.label=label
    def stop(self):
        value={'label':self.label,'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'ebp':int(gdb.parse_and_eval('(unsigned int)$ebp')),
          'fp':capture_x87(),'compiler_state':capture_modeled()}
        if self.label=='converted':
            address=value['ebp']+RESULT_DISPLACEMENT
            value['qword_address']=address;value['qword_bytes']=gdb.selected_inferior().read_memory(address,8).tobytes().hex()
        scaler_fp_records.append(value)
        return False
class ScalerReturn(gdb.Breakpoint):
    def __init__(self,address):super(ScalerReturn,self).__init__('*0x%x'%address)
    def stop(self):
        scaler_returns.append({'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'eax':int(gdb.parse_and_eval('(unsigned int)$eax')),
          'fp':capture_x87(),'compiler_state':capture_modeled()});self.enabled=False;return False
class ScalerEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('(unsigned int)$esp'));words=list(struct.unpack('<2I',gdb.selected_inferior().read_memory(esp,8).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        frame=gdb.newest_frame();caller=frame.older();ancestor=frame
        while ancestor and ancestor.name()!='InvokeCompilerPass':ancestor=ancestor.older()
        assert ancestor
        actual={key:int(ancestor.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv','mode')}
        invocation={'symbol':ancestor.name(),'args':actual,'argv':[ancestor.read_var('argv')[i].string()for i in range(actual['argc'])],
          'argv_pointers':[int(ancestor.read_var('argv')[i].cast(gdb.lookup_type('unsigned int')))for i in range(actual['argc'])],
          'argv_terminator':int(ancestor.read_var('argv')[actual['argc']].cast(gdb.lookup_type('unsigned int')))}
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        caller_args={key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('this','descriptor','address')}
        scaler_entries.append({'symbol':frame.name(),'invocation':invocation,'crt_state':crt_state,'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':caller_args},
          'args':{'descriptor':words[1]},'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':esp,'return':words[0],'descriptor':words[1],
          'formal_inventory':[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument],'fp':capture_x87(),'compiler_state':capture_modeled()})
        ScalerReturn(words[0]);return False
'''.replace('RESULT_DISPLACEMENT',str(metadata['result_ebp_displacement']))
 code+="""initial_x87_records=[]
class InitialX87(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('(unsigned int)$esp'));words=list(struct.unpack('<4I',gdb.selected_inferior().read_memory(esp,16).tobytes()))
        initial_x87_records.append({'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':esp,'return':words[0],'args':words[1:],'fp':capture_x87(),
          'profile':{'reference':gdb.parse_and_eval('native_x87_empty_entry.reference_sha256').string(),'applications':int(gdb.parse_and_eval('native_x87_empty_entry.applications'))}});return False
"""
 code+='InitialX87("*0x%x")\n'%names['_DllMainCRTStartup'][0]
 code+='ScalerEntry("*0x%x")\n'%start
 for index,instruction in enumerate(floating):code+='ScalerFloatPoint(%d,%r)\n'%(instruction['pc'],'instruction-%d'%index)
 code+='ScalerFloatPoint(%d,"converted")\n'%converted_pc
 code+='''arena_reset_entries=[]
arena_reset_returns=[]
class ArenaResetReturn(gdb.Breakpoint):
    def __init__(self,address,index):
        super(ArenaResetReturn,self).__init__('*0x%x'%address);self.index=index
    def stop(self):
        arena_reset_returns.append({'index':self.index,'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'fp':capture_x87(),'compiler_state':capture_modeled()});self.enabled=False;return False
class ArenaResetEntry(gdb.Breakpoint):
    def stop(self):
        pc=int(gdb.parse_and_eval('(unsigned int)$pc'));esp=int(gdb.parse_and_eval('(unsigned int)$esp'))
        words=list(struct.unpack('<2I',gdb.selected_inferior().read_memory(esp,8).tobytes()))
        block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        assert block and block.function
        before=capture_modeled();index=len(arena_reset_entries);ecx=int(gdb.parse_and_eval('(unsigned int)$ecx'))
        if RESET_SEED:
            assert index<6 and ecx==before['live_addresses']['bss']+804+index*32
            base=before['bss']['arenas'][index]['base'];assert base
            for offset,value in [(0,base+0x1000),(4,base+0x2000),(12,117+index),(24,37+index)]:
                gdb.selected_inferior().write_memory(ecx+offset,struct.pack('<I',value))
        arena_reset_entries.append({'index':index,'pc':pc,'esp':esp,'return':words[0],'ecx':ecx,'seeded':RESET_SEED,'pre_seed':before,
          'formal_inventory':[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument],
          'fp':capture_x87(),'compiler_state':capture_modeled()})
        ArenaResetReturn(words[0],index);return False
'''
 code=code.replace('RESET_SEED',repr(reset_seed))
 code+='ArenaResetEntry("*0x%x")\n'%names['_ZN13CompilerArena5ResetEv'][0]
 return code
