"""Defined OMF alignments and witnessed conditional hidden-EAX ABI."""
from pathlib import Path
import argparse,copy,hashlib,json,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_INTR
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_alignment@4';row=next(r for r in cfg['driver']if r['name']==name)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def bound(directory):
 p=Path(directory);binding=json.loads((p/'original-binding.json').read_text());assert all(sha(p/n)==h for n,h in binding['files'].items());assert binding['target_sha256']==cfg['target']['sha256'];return (p/'original.txt').read_text()
old=json.loads(Path(row['abi']['entry_observation']).read_text());bound('cases/omf-alignment-abi')
# Keep the prior4 observation in its original, defined-branch scope. It did
# not claim absence of the newly encountered conditional default-register input.
assert old['matched']and old['formals']==[dict(name='alignment',location='ECX',type='unsigned32',value=4)]and old['hidden_inputs']==[]and old['native']['values']==old['original_values']==[4]
assert row['abi']['parameters']==[dict(name='alignment',location='ECX',type='unsigned32')]and row['abi']['callee_stack_cleanup']==0
hidden=[dict(name='incoming_eax',location='EAX',type='unsigned32',condition='alignment outside1/2/4/16',native_supported_condition='alignment==8')]
assert row['abi']['hidden_inputs']==hidden and row['abi']['native_support']==dict(defined_alignments=[1,2,4,16],hidden_eax_passthrough=[8],other_alignments='explicit native INT3; original missing-return EAX behavior retained in Win32')
a=bound('cases/comdat-alignment8-original');b=bound('cases/comdat-alignment-selection-original');native=json.loads(Path('cases/comdat-alignment8-native/validation.json').read_text());assert native['gdb_sha256']==sha('cases/comdat-alignment8-native/native.txt')and len(native['executable_sha256'])==len(native['source_sha256'])==64
def fields(text):
 words=next(v for v in text.splitlines()if v.startswith('alignment8 ')).split()[1:];return {words[i]:int(words[i+1],10 if words[i]in ['alignment','count']else 16)for i in range(0,len(words),2)}
left=fields(a);other=fields(b);assert left['eax']==left['record']==left['current']==left['base']==0x7a8dc and left['alignment']==8 and left['count']==1 and left['cursor']-left['base']==23 and left['return_address']==0x6a8db
assert 'return @omf_alignment@4 eax 0007a8dc args 00000008'in a
assert other['eax']==0 and other['alignment']==16 and other['return_address']==0x6c1a3 and 'return @omf_alignment@4 eax 00000003 args 00000010'in b

def actual(d):
 assert d['matched']and d['object_sha256']=='2dcb42c74d07c84c7aa7f25a1724dd70b43ea06b1dca747a5e257ff544557783'
 assert {r['caller']for r in d['rows']}=={'omf_write_segment_definition','omf_initialize_record'}
 for r in d['rows']:
  assert r['eax']==r['producer']==r['incoming_eax']and r['ecx']==r['alignment']and r['return_esp']==r['esp']+4 and r['nonvol']==r['return_nonvol']
  assert r['slots']==[r['pool']+i*1004 for i in range(4)]and r['pool']%8==4
  assert r['return_eax']=={1:1,2:2,4:5,16:3}.get(r['alignment'],r['eax'])
  if r['caller']=='omf_initialize_record':assert r['eax']==0 and r['alignment']==16 and r['count']==3 and r['cursor_offset']==25
  else:assert r['eax']in r['slots']and r['count']==1 and r['cursor_offset']==23
 assert any(r['alignment']==8 and r['eax']==r['pool']for r in d['rows'])
actual(native);reject=[]
for label,edit in [('wrong hidden EAX',lambda d:d['rows'][0].update(incoming_eax=0)),('wrong producer identity',lambda d:d['rows'][0].update(producer=0)),('hook lost ECX',lambda d:d['rows'][0].update(alignment=8)),('wrong cleanup',lambda d:d['rows'][0].update(return_esp=0)),('changed nonvolatile',lambda d:d['rows'][0]['return_nonvol'].update(ebx=0)),('lost pointer modulo',lambda d:d['rows'][0].update(pool=d['rows'][0]['pool']+4)),('constant default return',lambda d:next(r for r in d['rows']if r['alignment']==8).update(return_eax=8)),('missing caller',lambda d:d.update(rows=[r for r in d['rows']if r['caller']!='omf_initialize_record']))]:
 d=copy.deepcopy(native);edit(d)
 try:actual(d)
 except AssertionError:reject.append(label)
 else:raise AssertionError('accepted '+label)
assert row['start']==0x6a310 and row['end']==0x6a346 and original.read(0x6a324,1)==b'\xc3'and original.read(0x6a345,1)==b'\xc3'
# Exact original producer chains: begin_record result survives ESI/count/cursor
# work to SEGDEF; COMDAT selection result survives only stores/increments.
assert original.read(0x6a8c2,5)==b'\xe8'+struct.pack('<i',0x6b480-0x6a8c7)
assert original.read(0x6a8c7,20).hex()=='8bf08b5e0c43895e0c8b7e108b4d30e835faffff'
assert original.read(0x6c18c,23).hex()=='43895e0c88450045896e1043895e0c8b4f30e86de1ffff'
source=Path('src/omf-output.c').read_text();body=source[source.index('/* Original [0006a310,0006a346).'):].split('/* Original [0006c2b0,',1)[0]
assert 'unsigned __attribute__((regcall)) omf_alignment(unsigned incoming_eax,'in body and 'if (alignment == 8) return incoming_eax;\n    __asm int 3'in body
assert '__asm mov incoming_eax, eax'in body and '__asm mov eax, incoming_eax'in body and body.count('return ')==5
assert 'value = omf_alignment((unsigned)record, segment->alignment);'in source and 'value = omf_alignment(value, segment->alignment);'in source
class Elf:
 def __init__(self,path):
  self.path=path;self.data=Path(path).read_bytes();d=self.data;assert d[:5]==b'\x7fELF\x01';ph=struct.unpack_from('<I',d,28)[0];size,count=struct.unpack_from('<HH',d,42);self.loads=[]
  for i in range(count):
   typ,off,va,_,fs,ms,_,_=struct.unpack_from('<8I',d,ph+i*size)
   if typ==1:self.loads.append((va,d[off:off+fs]))
  self.symbols={line.split()[-1]:int(line.split()[0],16)for line in subprocess.check_output(['nm','--defined-only',path],text=True).splitlines()if len(line.split())==3}
  self.sizes={line.split()[-1]:int(line.split()[1],16)for line in subprocess.check_output(['nm','-S','--defined-only',path],text=True).splitlines()if len(line.split())==4}
elfs=[Elf(p)for p in ['build/native/C23232.EXE','build/native-trace/C23232.EXE']]

# Current-build structural ABI proof remains cheap when later application
# additions move symbols. Historical GDB snapshots keep their own hashes;
# they are not silently rebound to a newer whole executable/source file.
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
md=Cs(CS_ARCH_X86,CS_MODE_32)
native_machine=[]
for image in elfs:
 entry=image.symbols['omf_alignment'];size=image.sizes['omf_alignment'];code=next(data[entry-va:entry-va+size]for va,data in image.loads if va<=entry<va+len(data));ins=list(md.disasm(code,entry));rets=[i for i in ins if i.mnemonic=='ret'];assert len(rets)==1 and not rets[0].op_str
 if 'native-trace'in image.path:
  hook_positions={kind:next(n for n,i in enumerate(ins)if i.mnemonic=='call'and i.op_str==hex(image.symbols['__cyg_profile_func_'+kind]))for kind in ['enter','exit']}
  stores={reg:next(i.op_str.split(',')[0]for i in ins[:hook_positions['enter']]if i.mnemonic=='mov'and i.op_str.endswith(', '+reg)and i.op_str.startswith('dword ptr [ebp -'))for reg in ['eax','ecx']}
  for reg,slot in stores.items():assert any(i.mnemonic=='mov'and i.op_str==reg+', '+slot for i in ins[hook_positions['enter']+1:hook_positions['enter']+5])
  result_slot=next(i.op_str.split(',')[0]for i in reversed(ins[:hook_positions['exit']])if i.mnemonic=='mov'and i.op_str.endswith(', eax')and i.op_str.startswith('dword ptr [ebp -'))
  assert ins[hook_positions['exit']+1].mnemonic=='mov'and ins[hook_positions['exit']+1].op_str=='eax, '+result_slot
 native_machine.append(dict(path=image.path,entry=entry,size=size,ret0=rets[0].address))
if args.abi_only:print('OMF alignment actual conditionalEAX/ECX ABI,2caller producers/automatic hooks/RET0/8 rejects pass');raise SystemExit


def run(image,entry,alignment,eax):
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,0x200000);u.mem_map(0x300000,0x10000);u.mem_map(0x400000,0x1000)
 if isinstance(image,Elf):
  for va,data in image.loads:u.mem_write(va,data)
 else:
  for s in image.sections:
   if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 u.mem_write(0x300800,struct.pack('<I',0x400000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,alignment);u.reg_write(UC_X86_REG_EAX,eax);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for reg,value in saved:u.reg_write(reg,value)
 calls=[];terminal=[]
 def step(m,a,size,_):
  if a==0x400000:terminal.append('return');m.emu_stop();return
  if isinstance(image,Elf)and a in [image.symbols.get('__cyg_profile_func_enter'),image.symbols.get('__cyg_profile_func_exit')]:
   esp=m.reg_read(UC_X86_REG_ESP);calls.append(a);m.reg_write(UC_X86_REG_EAX,0xaabbccdd);m.reg_write(UC_X86_REG_ECX,0xbbccddee);m.reg_write(UC_X86_REG_EDX,0xccddeeff);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+4)
 def intr(m,n,_):assert n==3;terminal.append('unsupported-trap');m.emu_stop()
 u.hook_add(UC_HOOK_CODE,step);u.hook_add(UC_HOOK_INTR,intr);u.emu_start(entry,0,count=1000);assert len(terminal)==1
 if terminal==['return']:assert u.reg_read(UC_X86_REG_EIP)==0x400000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)
 if isinstance(image,Elf)and 'native-trace'in image.path:assert len(calls)==(2 if terminal==['return']else 1)
 return terminal[0],u.reg_read(UC_X86_REG_EAX)
controls=[]
for alignment in [1,2,4,16,8,0,3,32,0xffffffff]:
 for eax in [0,0x7a8dc,0xa1004,0x12345678,0xffffffff]:
  want={1:1,2:2,4:5,16:3}.get(alignment,eax)
  assert run(original,row['start'],alignment,eax)==run(canonical,names[name],alignment,eax)==('return',want)
  for image in elfs:
   result=run(image,image.symbols['omf_alignment'],alignment,eax)
   assert result==('return',want)if alignment in [1,2,4,16,8]else result[0]=='unsupported-trap'
  controls.append(dict(alignment=alignment,incoming_eax=eax,result=want,native='return'if alignment in [1,2,4,16,8]else'unsupported-trap'))
report=dict(actual_conditional_abi=True,hidden_inputs=hidden,rejected=reject,controls=controls,original_segdef=left,original_comdat=other,native_actual_evidence='cases/comdat-alignment8-native/validation.json',native_executable_sha256=native['executable_sha256'],current_machine=native_machine,seconds=time.perf_counter()-started,scope='Whole54B original/canonical and bothnativeELF machine bodies; arbitrary fullEAX transport, explicit1/2/4/16 mappings, witnessed8 passthrough onlynative, othernative INT3 retained. Traced controls deliberately clobber EAX/ECX/EDX in BOTH automatic hooks, proving compiler spills/restores fullinputs/result; stack/nonvol preservation/RET0. Original default remains missing-C-return with narrow EAX capture/restore, callerblocks onlytransport. Actual14 tracedcalls frombothproducers, originalSEGDEF8/COMDAT16 entries+returns separatelycaptured. COMDAT8 not executed; no universal unsupportedvalue claim. Storage modulo/alias proof separate, zero newapplicationcredit.')
Path('cases/comdat-alignment8-validation.json').write_text(json.dumps(report,indent=2)+'\n');print('45 original/canonical/bothELF alignment controls (tracing-hook clobbers),8 ABI rejects pass')
