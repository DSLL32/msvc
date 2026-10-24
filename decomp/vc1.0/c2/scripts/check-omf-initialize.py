"""Actual OMF initializer pointer/stack ABI and full prefix construction controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');ap.add_argument('--eax-contract',action='store_true');args=ap.parse_args();start=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_initialize_record@12';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);actual=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def abi(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==4
 assert [{k:a[k]for k in ['name','location','type']}for a in e['formals']]==row['abi']['parameters']==[dict(name='record',location='ECX',type='omf_record_slot32'),dict(name='type',location='EDX',type='unsigned32'),dict(name='context',location='entryESP+4',type='null32')]
 assert [a['value']for a in e['formals']]==e['original_values']==e['native']['values']==[0,128,0]
 a=e['pool_input']['original'];b=e['pool_input']['native'];assert a['stride']==1004 and a['count']==4 and a['current']==a['base']and b['current']==b['base']and a['headers']==b['headers']==[bytes(20).hex()]*4
 f=e['record_pointer_formals'];assert len(f)==1 and f[0]['name']=='record'and f[0]['slot']==0 and f[0]['defined_pointee']and f[0]['original_pointer']==e['original_raw_values'][0]==a['base']and f[0]['native_pointer']==e['native_raw_values'][0]==b['base']
abi(actual);reject=[]
for label,change in [('missing context',lambda e:e['formals'].pop()),('wrong stack position',lambda e:e['formals'][2].update(location='entryESP+8')),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=0)),('unknown hidden input',lambda e:e.update(hidden_inputs=['?'])),('nonnull context',lambda e:e['native']['values'].__setitem__(2,1)),('lost record identity',lambda e:e['record_pointer_formals'][0].update(original_pointer=0)),('wrong native alias',lambda e:e['record_pointer_formals'][0].update(native_pointer=0)),('wrong header bytes',lambda e:e['pool_input']['native']['headers'].__setitem__(0,'01'+bytes(19).hex()))]:
 e=copy.deepcopy(actual);change(e)
 try:abi(e)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6c060 and row['end']==0x6c243 and original.read(0x6c240,3)==b'\xc2\x04\0'
# BSS proof for the newly encountered cached name, not a read through a zero
# raw file offset. The PE virtual BSS owns7e8b0 and initializes it to zero.
section=next(s for s in original.sections if original.base+s['rva']<=0x7e8b0<original.base+s['rva']+s['virtual_size']);assert section['flags']&0x80
if args.abi_only:print('OMF initializer actual pointer/stack ABI,8 rejects and483-byte extent pass');raise SystemExit

def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 pool=names['_omf_records']if native else 0x7a8dc;flags=names['_omf_comdat_flags']if native else 0x85090;cached=names['_omf_last_comdat_name']if native else 0x7e8b0
 children={names['@omf_flush_record@4']if native else 0x6c330:'flush',names['@omf_write_index@8']if native else 0x6c2b0:'index',names['@omf_comdat_selection@4']if native else 0x6c2f0:'selection',names['@omf_alignment@4']if native else 0x6a310:'alignment'}
 context=0 if c['null']else 0x400000;parent=0x400080;group=0x400200;owner=0x400300
 segment=bytearray(72);struct.pack_into('<I',segment,4,c['position']);struct.pack_into('<I',segment,24,group);struct.pack_into('<I',segment,48,c['alignment']);struct.pack_into('<I',segment,56,c['name']);struct.pack_into('<I',segment,60,parent);struct.pack_into('<I',segment,64,owner if c['type']==0xc3 else c['order']);segment[68]=c['selection'];segment[69]=c['flags'];u.mem_write(0x400000,bytes(segment))
 parent_data=bytearray(72);struct.pack_into('<I',parent_data,24,group);struct.pack_into('<I',parent_data,64,c['order']);u.mem_write(parent,bytes(parent_data));u.mem_write(group,struct.pack('<II',c['group_name'],c['group_number']));u.mem_write(owner+32,struct.pack('<I',c['owner_type']))
 before=bytearray((i*31+11)&255 for i in range(4016))
 for i in range(4):struct.pack_into('<5I',before,i*1004,0,0,0,c['count'],pool+i*1004+40)
 selected=c['slot'];target=(selected+1)%4 if c['occupied']else selected;struct.pack_into('<I',before,selected*1004,c['occupied']);u.mem_write(pool,bytes(before));u.mem_write(flags,b'\x55');u.mem_write(cached,struct.pack('<I',c['old_name']));u.mem_write(0x300800,struct.pack('<II',0x500000,context));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,pool+selected*1004);u.reg_write(UC_X86_REG_EDX,c['type'])
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 events=[]
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a not in children:return
  kind=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);r=pool+target*1004;count=struct.unpack('<I',u.mem_read(r+12,4))[0];cursor=struct.unpack('<I',u.mem_read(r+16,4))[0];ev=[kind]
  if kind=='flush':
   assert ecx==pool+selected*1004;ev+=[selected];result=r
  elif kind=='index':
   assert ecx==r;ev+=[edx,count,cursor-r,bytes(u.mem_read(r+20,cursor-r-20)).hex()];u.mem_write(cursor,bytes([0x60+(edx&15)]));u.mem_write(r+12,struct.pack('<I',(count+1)&0xffffffff));u.mem_write(r+16,struct.pack('<I',cursor+1));result=0xabcdef98
  else:
   ev+=[ecx&255 if kind=='selection'else ecx,count,cursor-r,bytes(u.mem_read(r+20,cursor-r-20)).hex()];result=({1:0x10,2:0,3:0x20,4:0x30}[c['selection']]if args.eax_contract else 0xabcdef5c)if kind=='selection'else 0xc0d0e07b
   if args.eax_contract and kind=='alignment':assert m.reg_read(UC_X86_REG_EAX)=={1:0x10,2:0,3:0x20,4:0x30}[c['selection']]
  events.append(ev);m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0x123456);m.reg_write(UC_X86_REG_EDX,0x234567);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=10000)
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300808 and u.reg_read(UC_X86_REG_EAX)==pool+target*1004 and all(u.reg_read(r)==v for r,v in saved)
 output=bytearray(u.mem_read(pool,4016));typ,ctx,offset,count,cursor=struct.unpack_from('<5I',output,target*1004);length={0x91:2,0x95:2,0xb7:2,0x9a:1,0xa1:5,0xa3:5,0xc3:11,0xc5:2}.get(c['type'],0)
 assert typ==c['type']and ctx==context and offset==(c['position']if context else 0)and count==0 and cursor==pool+target*1004+23+length and output[target*1004+20]==c['type']&255
 assert output[target*1004+21:target*1004+23]==before[target*1004+21:target*1004+23]
 for i in range(4):struct.pack_into('<I',output,i*1004+16,struct.unpack_from('<I',output,i*1004+16)[0]-pool)
 return dict(slot=target,events=events,pool_sha256=hashlib.sha256(output).hexdigest(),cached_flags=bytes(u.mem_read(flags,1)).hex(),cached_name=struct.unpack('<I',u.mem_read(cached,4))[0])
rows=[]
if args.eax_contract:
 for selection,alignment in itertools.product([1,2,3,4],[8,16]):
  c=dict(type=0xc3,occupied=0,slot=0,count=0,null=False,position=0x89abcdef,alignment=alignment,name=0x123,old_name=0x123,order=0x135,selection=selection,flags=2,group_name=0x127,group_number=0x234,owner_type=0x1234ffff)
  a=run(original,False,c);b=run(canonical,True,c);assert a==b,(c,a,b);rows.append(dict(inputs=c,result=a))
 Path('cases/omf-initialize-eax-validation.json').write_text(json.dumps(dict(states=rows,seconds=time.perf_counter()-start,scope='Original/canonical whole initializer with valid selection1..4 full EAX results as independently proved mapper outputs, alignments8/16. Alignment child hook requires exact full EAX producer as well as existing ECX/count/cursor/output/stack/nonvolatile controls. Mocked child results; not evidence actual case executes COMDAT8.'),indent=2)+'\n');print('8 original/canonical COMDAT caller full-EAX producer controls pass');raise SystemExit
for type,occupied,slot,count in itertools.product([0,0x80,0x91,0x95,0xb7,0x9a,0xa1,0xa3,0xc3,0xc5,0xffffffff],[0,0x99],[0,3],[0,7,0xffffffff]):
 c=dict(type=type,occupied=occupied,slot=slot,count=count,null=False,position=0x89abcdef,alignment=4,name=0x123,old_name=0x123,order=0x135,selection=3,flags=2,group_name=0x127,group_number=0x234,owner_type=0x1234ffff)
 a=run(original,False,c);b=run(canonical,True,c);assert a==b,(c,a,b);rows.append(dict(inputs=c,result=a))
for type,value in itertools.product([0x80,0xc3,0xc5],[0,1,0x80000000,0xffffffff]):
 c=dict(type=type,occupied=0,slot=0,count=19,null=type==0x80,position=value,alignment=16,name=value,old_name=0,order=0xffffffff,selection=4,flags=0x80,group_name=0xff,group_number=0xffffffff,owner_type=0x12348000)
 a=run(original,False,c);b=run(canonical,True,c);assert a==b,(c,a,b);rows.append(dict(inputs=c,result=a))
Path('cases/omf-initialize-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-start,scope='Whole483-byte original/canonical initializer: all8 prefix categories and default, every header/prefix/payload byte in4016-byte cache, occupied flush returning another slot, pointer/ESP/RET4/GPR preservation, counter overflow/reset and every child args/count/cursor/prefix at call boundary. Index/selection/alignment/flush bodies mocked, not accepted or claimed actual cold execution. Actual slot0/type80/NULL path has no children and defined zero header.'),indent=2)+'\n');print(len(rows),'whole original/canonical OMF prefix controls and8 ABI rejects pass')
