"""No-formal finalizer ABI, conditional flush order, segment rewrites and close."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='_omf_finish_module';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
expected_file=dict(flags=10,count=2102,fd=6,size=4096,null=0,used=1994,position=0,osfile=1,file_bytes=0);prefix_sha='d6dacbb9da965a55d2343f8d62ee47130bee6569b7cfb8c59df44828ca3dee1c'
expected_segments=[dict(index=i,combine=k,header_end=202+12*i,order=i+1)for i,k in enumerate([2,2,2,2,0,0,2])]
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6a9d0'and e['formals']==row['abi']['parameters']==[]and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and e['no_hidden_inputs_proof']
 assert e['original']['file']['return_address']==0x4c25c and e['native']['caller']=='finish_debugging'
 for side in ['original','native']:
  d=e[side];f=d['file'];assert f['stream']and {k:f[k]for k in expected_file}==expected_file and hashlib.sha256(bytes.fromhex(f['buffer_hex'])).hexdigest()==prefix_sha
  nodes=d['segments'];assert len(nodes)==7 and all({k:n[k]for k in expected_segments[i]}==expected_segments[i]and n['pointer']and n['next']==(nodes[i+1]['pointer']if i+1<len(nodes)else 0)for i,n in enumerate(nodes))
validate(e);reject=[]
for label,change in [('invented file formal',lambda e:e.update(formals=[dict(name='file',location='ECX')])),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('missing file owner',lambda e:e['native']['file'].update(stream=0)),('wrong buffered count',lambda e:e['original']['file'].update(count=2100)),('truncated prefix',lambda e:e['native']['file'].update(buffer_hex='')),('wrong descriptor position',lambda e:e['native']['file'].update(position=1)),('wrong segment link',lambda e:e['original']['segments'][0].update(next=0)),('wrong segment header offset',lambda e:e['native']['segments'][0].update(header_end=0)),('wrong segment order',lambda e:e['native']['segments'][1].update(order=1)),('invented old started input',lambda e:e.update(hidden_inputs=['old_object_generation_started']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6a9d0 and row['end']==0x6aacd and original.read(0x6aacc,1)==b'\xc3'and original.read(0x6aa67,3)==bytes.fromhex('385e35')
assert 'void omf_finish_module(void);'in Path('src/output.h').read_text()
if args.abi_only:print('OMF finalizer actual no formals/FILE1994-byte prefix/7 segment owners/10 rejects and253-byte extent pass');raise SystemExit

def run(image,native,combines,winner,end,mode):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in[(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 glob={k:names['_'+k]if native else a for k,a in [('object_file',0x84fb0),('segment_list',0x84fac),('omf_next_segment',0x85080),('object_generation_started',0x83158)]};put=lambda a,v:u.mem_write(a,struct.pack('<I',v&0xffffffff));word=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
 pointers=[0x400080+0x100*i for i in range(len(combines))];ds={}
 for i,p in enumerate(pointers):
  d=bytearray((j*13+5)&255 for j in range(72));d[53]=combines[i];struct.pack_into('<I',d,36,pointers[i+1]if i+1<len(pointers)else 0);struct.pack_into('<I',d,60,0x80000000 if i else 202);struct.pack_into('<I',d,64,i+1);ds[p]=d;u.mem_write(p,bytes(d))
 file=0x400c00;put(glob['object_file'],file);put(glob['segment_list'],pointers[0]if pointers else 0);put(glob['omf_next_segment'],0xfedcba98);put(glob['object_generation_started'],0x12345678);u.mem_write(0x300800,struct.pack('<2I',0x500000,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800)
 for reg in[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX]:u.reg_write(reg,0x89abcdef)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for reg,value in saved:u.reg_write(reg,value)
 children={names[n]if native else a:k for n,a,k in [('@omf_flush_record_type@4',0x6aaf0,'type'),('_ftell',0x71d50,'tell'),('_fseek',0x71f00,'seek'),('@omf_write_segment_definition@4',0x6a890,'definition'),('_omf_flush_records',0x6a7f0,'flush'),('_omf_write_module_end',0x6aad0,'end'),('_fclose',0x71b80,'close')]}
 initial=[0x91,0xb0]+([0x8c]if winner!=1 else[])+([0xb4]if winner not in[1,2]else[])+([0xbc]if winner==0 else[])+[0xa3,0xa1,0xc3,0xc5,0x95];phase='initial';initial_index=0;current=0;expected_head=pointers[0]if pointers else 0;last_index=0xfedcba98;events=[];terminal=False
 def advance(pointer):
  while pointer and ds[pointer][53]==7:pointer=struct.unpack_from('<I',ds[pointer],36)[0]
  return pointer
 def access(m,kind,address,size,value,_):
  assert address+size<=glob['object_generation_started']or address>=glob['object_generation_started']+4,'Old started flag is write-only'
  for p,d in ds.items():
   if d[53]==7:assert address+size<=p+60 or address>=p+68,'Skipped segment header/order are not input'
 def hook(m,address,size,_):
  nonlocal terminal,phase,initial_index,current,file,expected_head,last_index
  if address==0x500000:assert phase=='done';terminal=True;m.emu_stop();return
  if address not in children:return
  kind=children[address];esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);result=0xdeadbeef
  if phase=='initial':
   assert kind=='type'and initial_index<len(initial)and ecx==initial[initial_index];initial_index+=1;events.append(['type',ecx]);result=0x80000000 if ecx=={1:0xb0,2:0x8c,3:0xb4}.get(winner)else 0
   if initial_index==len(initial):phase='tell'
  elif phase=='tell':
   assert kind=='tell'and word(esp+4)==file;events.append(['tell',end]);result=end&0xffffffff
   if mode=='head'and len(pointers)>1:expected_head=pointers[1];put(glob['segment_list'],expected_head)
   current=advance(expected_head);phase='segment_seek'if current else'restore'
  elif phase=='segment_seek':
   target=struct.unpack_from('<I',ds[current],60)[0];assert kind=='seek'and[word(esp+i)for i in[4,8,12]]==[file,target,0];events.append(['seek-header',pointers.index(current),target]);result=0xffffffff if mode!='unchanged'else 0
   if mode=='fields':struct.pack_into('<I',ds[current],64,struct.unpack_from('<I',ds[current],64)[0]^0x4000);m.mem_write(current,bytes(ds[current]))
   last_index=struct.unpack_from('<I',ds[current],64)[0];phase='definition'
  elif phase=='definition':
   assert kind=='definition'and ecx==current and word(glob['omf_next_segment'])==last_index;events.append(['definition',pointers.index(current),last_index]);phase='segment_flush'
  elif phase=='segment_flush':
   assert kind=='flush';events.append(['flush-header',pointers.index(current)])
   if mode=='truncate':struct.pack_into('<I',ds[current],36,0);m.mem_write(current,bytes(ds[current]))
   current=advance(struct.unpack_from('<I',ds[current],36)[0]);phase='segment_seek'if current else'restore'
  elif phase=='restore':assert kind=='seek'and[word(esp+i)for i in[4,8,12]]==[file,end&0xffffffff,0];events.append(['restore',end]);phase='end';result=0xffffffff if mode!='unchanged'else 0
  elif phase=='end':assert kind=='end';events.append(['end']);phase='final_flush'
  elif phase=='final_flush':assert kind=='flush';events.append(['flush-final']);phase='close'
  elif phase=='close':assert kind=='close'and word(esp+4)==file;events.append(['close']);phase='done';result=0xffffffff if mode!='unchanged'else 0
  else:raise AssertionError((phase,kind))
  if mode!='unchanged':file=0x400d00 if file==0x400c00 else 0x400c00;put(glob['object_file'],file)
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,word(esp))
 u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_READ,access);u.emu_start(names[name]if native else row['start'],0,count=5000)
 assert terminal and phase=='done'and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and word(0x300804)==0xfeedbeef and word(glob['object_generation_started'])==0 and word(glob['segment_list'])==expected_head and word(glob['omf_next_segment'])==last_index and all(bytes(u.mem_read(p,72))==d for p,d in ds.items())
 return dict(events=events,started=0,last_segment_index=last_index)
rows=[]
for combines,winner,end,mode in itertools.product([[],[7],[2],[7,2],[2,7],[2,0],[7,7]],range(4),[-1,0,2594],['unchanged','fields','head','truncate']):
 a=run(original,False,combines,winner,end,mode);b=run(canonical,True,combines,winner,end,mode);assert a==b;rows.append(dict(combines=combines,first_nonzero_group_flush=winner,tell_result=end,child_mutation=mode,result=a))
Path('cases/omf-module-end-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole253-byte finalizer versus independent callback/seek/list state machine.336 original/canonical controls cover four short-circuit flush outcomes,seven empty/valid/combine7 lists,ftell-1/0/2594,ignored fseek/fclose errors and child mutations of file global/list head/current next/order. Exact rewrite offsets/post-seek order/restore end/MODEND/finalflush/close requests and started0,RET0/saved registers/owned guards. Memory reads of old write-only started and skipped header/order rejected. Public CRT/application children mocked, no real failure success or cold child execution claim. Actual no-formal FILE/full1994-byte prefix and7 segment rows separately matched; MODEND remains named unaccepted trap. Full exact OMF output still pending.'),indent=2)+'\n');print(len(rows),'whole finalizer controls/10 actual ABI rejects pass')
