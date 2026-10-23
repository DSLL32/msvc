"""Actual SEGDEF inputs and whole cached-prefix/index-write controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();start=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_write_segment_definition@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def fields(raw):
 b=bytes.fromhex(raw);assert len(b)==72
 return dict(position=struct.unpack_from('<I',b,4)[0],alignment=struct.unpack_from('<I',b,48)[0],word_size=b[53],name_index=struct.unpack_from('<I',b,56)[0],order=struct.unpack_from('<I',b,64)[0],class_index=struct.unpack_from('<I',b,68)[0])
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:a[k]for k in ['name','location','type']}for a in e['formals']]==row['abi']['parameters']==[dict(name='segment',location='ECX',type='Segment pointer')]
 expected=dict(position=0,alignment=4,word_size=2,name_index=3,order=1,class_index=4)
 assert e['consumed_fields']==expected and e['native']['size']==72
 for side in ['original','native']:
  d=e[side];assert d['pointer']==d['owner']==e['formals'][0][side]and d['pointer']and d['next_index']==d['empty_name']==1 and fields(d['raw_hex'])==expected
 assert e['field_name_refinement']['legacy_json_key']=='word_size'and e['field_name_refinement']['source_field']=='Segment.combine'and e['field_name_refinement']['offset']==53
validate(e);rejected=[]
for label,change in [('missing descriptor',lambda e:e.update(formals=[])),('wrong register',lambda e:e['formals'][0].update(location='EDX')),('hidden input',lambda e:e.update(hidden_inputs=['?'])),('wrong owner',lambda e:e['native'].update(owner=0)),('wrong counter',lambda e:e['native'].update(next_index=2)),('wrong empty-name',lambda e:e['original'].update(empty_name=0)),('changed descriptor scalar',lambda e:e['native'].update(raw_hex=e['native']['raw_hex'][:106]+'00'+e['native']['raw_hex'][108:])),('wrong descriptor extent',lambda e:e['native'].update(size=68))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):rejected.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6a890 and row['end']==0x6a940 and original.read(0x6a93f,1)==b'\xc3'
assert 'format_flags, combine, section_number'in Path('src/segments.h').read_text()and 'segment->combine'in Path('src/omf-output.c').read_text()
if args.abi_only:print('OMF SEGDEF actual descriptor/scalar ABI,8 rejects and byte53 combine provenance pass');raise SystemExit
MASK=0xffffffff

def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 segment=0x400000;record=0x400200;counter=names['_omf_next_segment']if native else 0x85080;empty=names['_omf_empty_name']if native else 0x7a8d8;d=bytearray((i*13+9)&255 for i in range(72))
 for offset,value in [(4,c['position']),(48,c['alignment']),(56,c['name_index']),(64,c['order']),(68,c['class_index'])]:struct.pack_into('<I',d,offset,value)
 d[53]=c['combine'];u.mem_write(segment,bytes(d));buf=bytearray((i*17+7)&255 for i in range(1004));struct.pack_into('<5I',buf,0,153,0,0,c['count'],record+c['cursor']);u.mem_write(record,bytes(buf));u.mem_write(counter,struct.pack('<I',c['next_index']));u.mem_write(empty,struct.pack('<I',c['empty']));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,segment)
 children={names[n]if native else a:k for k,n,a in [('begin','@omf_begin_record@8',0x6b480),('alignment','@omf_alignment@4',0x6a310),('index','@omf_write_index@8',0x6c2b0),('error','@compiler_internal_error@8',0x6fb70)]};events=[];nindex=0;terminal=None
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 def hook(m,a,size,_):
  nonlocal nindex,terminal
  if a==0x500000:terminal='return';m.emu_stop();return
  if a not in children:return
  kind=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);result=0xdeadbeef
  if kind=='error':
   raw=b''
   for i in range(64):
    b=bytes(m.mem_read(ecx+i,1));raw+=b
    if b==b'\0':break
   assert edx==0x5b2 and raw==b'@(#)omf.c:1.10\0';events.append(['error',edx]);terminal='internal-error';m.emu_stop();return
  assert struct.unpack('<I',m.mem_read(counter,4))[0]==(c['next_index']+1)&MASK
  if kind=='begin':assert ecx==0x99 and edx==0;events.append(['begin',ecx,edx]);result=record
  elif kind=='alignment':
   assert ecx==c['alignment'];assert struct.unpack('<I',m.mem_read(record+12,4))[0]==(c['count']+1)&MASK and struct.unpack('<I',m.mem_read(record+16,4))[0]==record+c['cursor'];events.append(['alignment',ecx,(c['count']+1)&MASK,c['cursor']]);result=c['alignment_result']
   if c['mutate']:
    m.mem_write(record+12,struct.pack('<I',77));m.mem_write(record+16,struct.pack('<I',record+700));m.mem_write(segment+4,struct.pack('<I',0x89abcdef));d[4:8]=struct.pack('<I',0x89abcdef);m.mem_write(segment+53,b'\xa5');d[53]=0xa5
  else:
   assert ecx==record;cursor=struct.unpack('<I',m.mem_read(record+16,4))[0];count=struct.unpack('<I',m.mem_read(record+12,4))[0];expected=[c['name_index'],0x2468ace0 if c['mutate']else c['class_index'],0x13579bdf if c['mutate']else c['empty']][nindex];assert edx==expected;events.append(['index',edx,count,cursor-record,bytes(m.mem_read(record+c['cursor'],5)).hex()]);m.mem_write(cursor,bytes([0x40+(edx&15)]));m.mem_write(record+12,struct.pack('<I',(count+1)&MASK));m.mem_write(record+16,struct.pack('<I',cursor+1));nindex+=1
   if c['mutate']and nindex==1:m.mem_write(segment+68,struct.pack('<I',0x2468ace0));d[68:72]=struct.pack('<I',0x2468ace0)
   if c['mutate']and nindex==2:m.mem_write(empty,struct.pack('<I',0x13579bdf))
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=20000)
 if c['order']!=c['next_index']:
  assert terminal=='internal-error'and events==[['error',0x5b2]]and bytes(u.mem_read(record,1004))==buf and struct.unpack('<I',u.mem_read(counter,4))[0]==c['next_index'];return dict(terminal=terminal,events=events)
 assert terminal=='return'and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and nindex==3
 combine=0xa5 if c['mutate']else c['combine'];position=0x89abcdef if c['mutate']else c['position'];attr=(((c['alignment_result']&255)<<3)|combine)&255;attr=((attr<<2)|1)&255;expected=bytearray(buf);prefix=bytes([attr])+struct.pack('<I',position);expected[c['cursor']:c['cursor']+5]=prefix
 values=[c['name_index'],0x2468ace0 if c['mutate']else c['class_index'],0x13579bdf if c['mutate']else c['empty']]
 for i,v in enumerate(values):expected[c['cursor']+5+i]=0x40+(v&15)
 struct.pack_into('<I',expected,12,(c['count']+8)&MASK);struct.pack_into('<I',expected,16,record+c['cursor']+8)
 assert bytes(u.mem_read(record,1004))==expected and bytes(u.mem_read(segment,72))==d and struct.unpack('<I',u.mem_read(empty,4))[0]==(0x13579bdf if c['mutate']else c['empty'])
 return dict(terminal=terminal,events=events,prefix_hex=prefix.hex(),record_sha256=hashlib.sha256(expected).hexdigest())
rows=[]
for ret,combine,pos,mutate in itertools.product([0,5,0xabcdef05,0xffffffff],[0,2,6,7,255],[0,0x89abcdef,0x80000000,0xffffffff],[False,True]):
 c=dict(alignment=[1,2,4,16][len(rows)%4],alignment_result=ret,combine=combine,position=pos,mutate=mutate,count=0xffffffff if len(rows)%3 else 0,cursor=23 if len(rows)%2 else 80,name_index=pos,class_index=pos^0x80808080,empty=(pos+1)&MASK,next_index=0xffffffff if len(rows)%5==0 else 1,order=0xffffffff if len(rows)%5==0 else 1)
 a=run(original,False,c);b=run(canonical,True,c);assert a==b,(c,a,b);rows.append(dict(inputs=c,result=a))
for order,next in [(0,1),(2,1),(0xffffffff,1),(1,0xffffffff)]:
 c=dict(alignment=4,alignment_result=5,combine=2,position=0,mutate=False,count=0,cursor=23,name_index=3,class_index=4,empty=1,next_index=next,order=order);a=run(original,False,c);b=run(canonical,True,c);assert a==b;rows.append(dict(inputs=c,result=a))
Path('cases/omf-segment-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,states=len(rows),cases=rows,seconds=time.perf_counter()-start,scope='Whole176-byte original/canonical SEGDEF writer: counter guard/wrap,99/NULL begin, count increment before alignment, cached cursor/count surviving mocked child mutation, lowAL and byte53 combine truncation,32-bit LE position, three reloaded index fields and exact unchanged other bytes. Alignment/index/begin bodies mocked; invalid-order controls stop at actual5b2 diagnostic, no returning-error claim. Actual code_segment six fields and two scalars independently observed; legacy JSON word_size key is source Segment.combine, no historical raw rewrite.'),indent=2)+'\n');print(len(rows),'whole OMF SEGDEF controls and8 actual ABI rejects pass')
