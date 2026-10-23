"""Actual ELF conversion and API observations, with no target emulation."""
import json,re,struct,subprocess,hashlib
from pathlib import Path
import capstone
from capstone.x86_const import X86_REG_EBP


def symbols(binary):
    text=subprocess.check_output(['nm','-S','--defined-only',str(binary)],text=True)
    return {m[3]:(int(m[1],16),int(m[2],16))for m in re.finditer(r'^([0-9a-f]+) ([0-9a-f]+) [TtWw] (\S+)$',text,re.M)}


def describe(binary):
    raw=binary.read_bytes();assert raw[:7]==b'\x7fELF\x01\x01\x01' and struct.unpack_from('<HHI',raw,16)==(2,3,1)
    offset=struct.unpack_from('<I',raw,32)[0];stride,count=struct.unpack_from('<HH',raw,46);assert stride==40 and offset+count*40<=len(raw)
    start,size=symbols(binary)['calculate_arena_reservation'];sections=[struct.unpack_from('<10I',raw,offset+i*40)for i in range(count)]
    matches=[s for s in sections if s[1]==1 and s[2]&4 and s[3]<=start and start+size<=s[3]+s[5]];assert len(matches)==1
    section=matches[0];off=section[4]+start-section[3];assert off+size<=len(raw)
    decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);decoder.detail=True;instructions=list(decoder.disasm(raw[off:off+size],start));assert sum(i.size for i in instructions)==size
    floating=[i for i in instructions if i.mnemonic.startswith('f')];assert [i.mnemonic for i in floating]==['fild','fild','fmulp','fld','fmulp','fnstcw','fldcw','fistp','fldcw']
    converted=floating[7];assert converted.operands[0].size==8 and converted.operands[0].mem.base==X86_REG_EBP and not converted.operands[0].mem.index
    return {'kind':'actual captured ELF x87 conversion instructions','elf_sha256':hashlib.sha256(raw).hexdigest(),'function':'calculate_arena_reservation','start':start,'size':size,'instructions':[dict(pc=i.address,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)for i in floating],'converted_pc':floating[-1].address+floating[-1].size,'qword_ebp_displacement':converted.operands[0].mem.disp}


def setup(output,binary,collector,model):
    (output/'scaler-probe-source.py').write_bytes(Path(__file__).read_bytes())
    metadata=describe(binary);names=symbols(binary);(output/'scaler-probe-metadata.json').write_text(json.dumps(metadata,indent=2)+'\n')
    externals=[(row['name'],row['native_expression'],row['bytes'])for row in model['fields']if row['class']=='external']
    for stem in ('scaler-return','arena-reset-entry'):
        path=output/('collect-'+stem+'.gdb');path.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/(stem+'.json')))).replace(repr(str(output/'native-argv.bin')),repr(str(output/(stem+'-argv.bin')))))
    body='''python
import gdb,json,struct
scaler_fp_records=[]
scaler_function_entries=[]
scaler_function_returns=[]
scaler_initial_entries=[]
scaler_profile_entries=[]
scaler_api_entries=[]
scaler_api_returns=[]
scaler_reset_entries=[]
def scaler_fp():
    cw=int(gdb.parse_and_eval('$fctrl'))&0xffff;sw=int(gdb.parse_and_eval('$fstat'))&0xffff;tag=int(gdb.parse_and_eval('$ftag'))&0xffff;top=(sw>>11)&7
    registers=[gdb.parse_and_eval('$st%d'%i).bytes.hex()for i in range(8)]
    return dict(cw=cw,sw=sw,tag=tag,top=top,registers=registers,live=[dict(logical=i,tag=(tag>>(2*((top+i)&7)))&3,raw80=registers[i])for i in range(8)if ((tag>>(2*((top+i)&7)))&3)!=3])
def scaler_guard():
    result={}
    for name,expression,width in [('data','frontend_data',int(gdb.parse_and_eval('sizeof(FrontendData)'))),('bss','frontend_bss',int(gdb.parse_and_eval('sizeof(FrontendBss)')))]+@@EXTERNAL@@:
        address=int(gdb.parse_and_eval('(unsigned int)&'+expression));result[name]=gdb.selected_inferior().read_memory(address,width).tobytes().hex()
    saved=int(gdb.parse_and_eval('(unsigned int)frontend_bss.saved_state'))
    result['saved']=gdb.selected_inferior().read_memory(saved,int(gdb.parse_and_eval('sizeof(FrontendData)'))).tobytes().hex()if saved else None
    return result
def scaler_context():
    row={r:int(gdb.parse_and_eval('(unsigned int)$'+r))for r in ('pc','esp','eax','edx','ecx','edi','esi','ebx','ebp')}
    row.update(fp=scaler_fp(),last_error=int(gdb.parse_and_eval('last_error')))
    return row
class ScalerInitial(gdb.Breakpoint):
    def stop(self):
        row=scaler_context();words=list(struct.unpack('<4I',gdb.selected_inferior().read_memory(row['esp'],16)))
        row.update(return_pc=words[0],args=words[1:],profile={'reference':gdb.parse_and_eval('native_x87_empty_entry.reference_sha256').string(),'applications':int(gdb.parse_and_eval('native_x87_empty_entry.applications'))})
        scaler_initial_entries.append(row);return False
class ScalerProfile(gdb.Breakpoint):
    def stop(self):
        row=scaler_context();words=list(struct.unpack('<5I',gdb.selected_inferior().read_memory(row['esp'],20)));row.update(args=[gdb.Value(words[1]).cast(gdb.lookup_type('char').pointer()).string()]+words[2:],applications=int(gdb.parse_and_eval('native_x87_empty_entry.applications')))
        scaler_profile_entries.append(row);return False
class ScalerPoint(gdb.Breakpoint):
    def __init__(self,address,label):super(ScalerPoint,self).__init__('*0x%x'%address,internal=True);self.label=label
    def stop(self):
        row=scaler_context();row.update(label=self.label,guard=scaler_guard())
        if self.label=='converted':
            row['qword_address']=row['ebp']+@@DISPLACEMENT@@;row['qword_bytes']=gdb.selected_inferior().read_memory(row['qword_address'],8).tobytes().hex()
        scaler_fp_records.append(row);return False
class ScalerReturn(gdb.Breakpoint):
    def stop(self):
        row=scaler_context();row['guard']=scaler_guard();scaler_function_returns.append(row)
        gdb.execute('source '+@@RETURN_COLLECTOR@@);self.enabled=False;return False
class ScalerEntry(gdb.Breakpoint):
    def stop(self):
        row=scaler_context();words=list(struct.unpack('<2I',gdb.selected_inferior().read_memory(row['esp'],8)));row.update(return_pc=words[0],descriptor=words[1],guard=scaler_guard())
        scaler_function_entries.append(row);ScalerReturn('*0x%x'%words[0],internal=True);return False
class ArenaApiReturn(gdb.Breakpoint):
    def stop(self):
        row=scaler_context();row.update(maps=open('/proc/%d/maps'%gdb.selected_inferior().pid).read(),guard=scaler_guard())
        view=gdb.parse_and_eval('allocations');matches=[];count=0
        while int(view):
            count+=1
            if count>32:raise RuntimeError('unbounded shared allocation list')
            address=int(view.dereference()['address']);length=int(view.dereference()['length'])
            if address==row['eax']:matches.append({'address':address,'bytes':length})
            view=view.dereference()['next']
        row['owned_reservations']=matches;scaler_api_returns.append(row);self.enabled=False;return False
class ArenaApiEntry(gdb.Breakpoint):
    def stop(self):
        caller=gdb.newest_frame().older().name()
        if caller=='reserve_work_buffer':return False
        if caller!='CompilerArena::Reserve':raise RuntimeError('unreviewed arena allocation caller')
        row=scaler_context();words=list(struct.unpack('<5I',gdb.selected_inferior().read_memory(row['esp'],20)));row.update(return_pc=words[0],args=words[1:],guard=scaler_guard())
        scaler_api_entries.append(row);ArenaApiReturn('*0x%x'%words[0],internal=True);return False
class ArenaResetEntry(gdb.Breakpoint):
    def stop(self):
        row=scaler_context();words=list(struct.unpack('<I',gdb.selected_inferior().read_memory(row['esp'],4)));row.update(return_pc=words[0],guard=scaler_guard())
        block=gdb.block_for_pc(row['pc'])
        while block and not block.function:block=block.superblock
        row['formal_inventory']=[{'name':item.name,'type':str(item.type)}for item in block if item.is_argument]
        scaler_reset_entries.append(row);gdb.execute('source '+@@RESET_COLLECTOR@@);return False
ScalerInitial('*0x%x'%@@INITIAL@@,internal=True)
ScalerProfile('*0x%x'%@@PROFILE@@,internal=True)
ScalerEntry('*0x%x'%@@ENTRY@@,internal=True)
ArenaApiEntry('*0x%x'%@@API@@,internal=True)
ArenaResetEntry('*0x%x'%@@RESET@@,internal=True)
end
'''
    replace={'@@EXTERNAL@@':repr(externals),'@@DISPLACEMENT@@':str(metadata['qword_ebp_displacement']),'@@RETURN_COLLECTOR@@':repr(str(output/'collect-scaler-return.gdb')),'@@RESET_COLLECTOR@@':repr(str(output/'collect-arena-reset-entry.gdb')),'@@INITIAL@@':str(names['_DllMainCRTStartup'][0]),'@@PROFILE@@':str(names['recon_apply_x87_empty_entry_profile'][0]),'@@ENTRY@@':str(metadata['start']),'@@API@@':str(names['recon_VirtualAlloc'][0]),'@@RESET@@':str(names['_ZN13CompilerArena5ResetEv'][0])}
    for key,value in replace.items():body=body.replace(key,value)
    script=output/'observe-scaler.gdb';body=body[:-4]
    for i,row in enumerate(metadata['instructions']):body+='ScalerPoint(%d,%r)\n'%(row['pc'],'instruction-%d'%i)
    body+='ScalerPoint(%d,"converted")\nend\n'%metadata['converted_pc'];script.write_text(body)
    finish='python\n'
    for name,variable in [('scaler-fpu','scaler_fp_records'),('scaler-entries','scaler_function_entries'),('scaler-returns','scaler_function_returns'),('scaler-initial','scaler_initial_entries'),('scaler-profile','scaler_profile_entries'),('arena-api-entries','scaler_api_entries'),('arena-api-returns','scaler_api_returns'),('arena-reset-entries','scaler_reset_entries')]:finish+='open('+repr(str(output/(name+'.json')))+',"w").write(json.dumps('+variable+',indent=2)+"\\n")\n'
    finish+='end\n';return 'source '+str(script)+'\n',finish
