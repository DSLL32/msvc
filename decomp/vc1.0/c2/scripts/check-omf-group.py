"""Actual GRPDEF inputs and whole group-alias/member writer controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_write_group_definition@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name='segment',location='ECX',type='Segment pointer'),dict(name='group',location='EDX',type='OmfGroup pointer')]
 assert e['native']['segment_size']==72 and e['native']['group_size']==8
 for side in ['original','native']:
  d=e[side];assert d['segment']==d['segment_owner']==e['formals'][0][side]and d['group']==d['group_owner']==e['formals'][1][side]and d['dos']==1
  raw=bytes.fromhex(d['segment_hex']);assert len(raw)==72 and struct.unpack_from('<I',raw,64)[0]==2 and bytes.fromhex(d['group_hex'])==struct.pack('<2I',2,1)
validate(e);reject=[]
for label,change in [('missing group formal',lambda e:e['formals'].pop()),('wrong group owner',lambda e:e['native'].update(group_owner=0)),('wrong segment owner',lambda e:e['original'].update(segment_owner=0)),('changed DOS predicate',lambda e:e['native'].update(dos=0)),('wrong group words',lambda e:e['native'].update(group_hex=bytes(8).hex())),('wrong order',lambda e:e['native'].update(segment_hex=e['native']['segment_hex'][:128]+'03000000'+e['native']['segment_hex'][136:])),('hidden input',lambda e:e.update(hidden_inputs=['EAX']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6a850 and row['end']==0x6a884 and original.read(0x6a883,1)==b'\xc3'

if args.abi_only:
 print(name,'actual ABI/negative/extent/storage checks pass; full semantic grid is explicit');raise SystemExit

def run(image,native,dos,after_dos,order,count,mutate):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 segment=0x400080;group=0x400180;record=0x400280;glob=names['_dos_object']if native else 0x80684;data=bytearray((i*13+5)&255 for i in range(72));struct.pack_into('<I',data,64,order);u.mem_write(segment,bytes(data));grp=struct.pack('<2I',23,7);u.mem_write(group,grp);buf=bytearray((i*17+3)&255 for i in range(1004));struct.pack_into('<5I',buf,0,0x9a,segment,0,count,record+28);u.mem_write(record,bytes(buf));u.mem_write(glob,struct.pack('<I',dos));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,segment);u.reg_write(UC_X86_REG_EDX,group)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 children={names[n]if native else a:k for n,a,k in [('@omf_begin_record@8',0x6b480,'begin'),('@omf_write_index@8',0x6c2b0,'index')]};events=[]
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a not in children:return
  k=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP)
  if k=='begin':
   assert ecx==0x9a and edx==segment and struct.unpack('<I',m.mem_read(segment+24,4))[0]==group;events.append(['begin',ecx,'segment','group-stored']);m.mem_write(glob,struct.pack('<I',after_dos))
   if mutate:m.mem_write(segment+64,struct.pack('<I',0xfedcba98))
   result=record
  else:
   assert ecx==record and edx==(0xfedcba98 if mutate else order)and bytes(m.mem_read(record+28,1))==b'\xff'and struct.unpack('<I',m.mem_read(record+12,4))[0]==(count+1)&0xffffffff and struct.unpack('<I',m.mem_read(record+16,4))[0]==record+29;events.append(['index',edx]);result=0xdeadbeef
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=300)
 expected=bytearray(buf);struct.pack_into('<I',data,24,group)
 if mutate:struct.pack_into('<I',data,64,0xfedcba98)
 if after_dos==0:expected[28]=255;struct.pack_into('<I',expected,12,(count+1)&0xffffffff);struct.pack_into('<I',expected,16,record+29)
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(segment,72))==data and bytes(u.mem_read(group,8))==grp and bytes(u.mem_read(record,1004))==expected and len(events)==(2 if after_dos==0 else 1)
 return events
rows=[]
for dos,after_dos,order,count,mutate in itertools.product([0,1],[0,1],[2,0xffffffff],[0,0xffffffff],[False,True]):
 a=run(original,False,dos,after_dos,order,count,mutate);b=run(canonical,True,dos,after_dos,order,count,mutate);assert a==b;rows.append(dict(dos=dos,after_child_dos=after_dos,order=order,count=count,child_mutates_order=mutate,events=a))
Path('cases/omf-group-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole52-byte original/canonical GRPDEF helper; group alias stored before begin child,9a/Segment arguments, DOS global and order reload after child, explicitFF member prefix/count wrap/cursor before index, all72 Segment/8 group/1004 record bytes and callee-saved/RET0 checked.32 controls mock begin/index only; actual dos1 branch does not execute non-DOS append, cold branch has bounded control proof rather than case execution credit.'),indent=2)+'\n');print(len(rows),'whole OMF group-definition controls and7 actual ABI rejects pass')
