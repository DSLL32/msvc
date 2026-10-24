"""Actual fixup formal/fields and whole FIXUPP32 bit/datum/child-order controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_write_fixup@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
params=[dict(name='fixup',location='ECX',type='OmfFixup pointer')]
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6c680'and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and e['no_hidden_inputs_proof']
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==params
 shape=dict(slot=0,count=4,type=9,location=120,flags=7,frame=0,target=1,payload=dict(addend=40));assert e['shape']==shape
 for side in ['original','native']:
  d=e[side];s=d['state'];assert {k:s[k]for k in shape if k!='payload'}=={k:v for k,v in shape.items()if k!='payload'}and d['payload']==shape['payload']and s['pointer']==s['pool']and s['pointer']==e['formals'][0][side]
 assert e['original']['state']['pool']==0x79154 and e['original']['state']['return_address']==0x6c41e and e['native']['state']['owner']==e['native']['state']['pointer']and e['native']['state']['loop_index']==0 and e['native']['state']['caller']=='omf_write_fixups'
validate(e);reject=[]
for label,change in [('missing formal',lambda e:e.update(formals=[])),('wrong register',lambda e:e['formals'][0].update(location='EDX')),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('wrong closed pool',lambda e:e['original']['state'].update(pool=0)),('wrong caller owner',lambda e:e['native']['state'].update(owner=0)),('wrong flags',lambda e:e['native']['state'].update(flags=15)),('missing conditional addend',lambda e:e['original'].update(payload={})),('invented padding input',lambda e:e.update(hidden_inputs=['padding']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6c680 and row['end']==0x6c7b2 and original.read(0x6c7b1,1)==b'\xc3'
assert original.read(0x6c686,5)==bytes.fromhex('8b4608a808')and original.read(0x6c735,6)==bytes.fromhex('f7c502000000')and original.read(0x6c75c,6)==bytes.fromhex('f7c504000000')and original.read(0x6c771,6)==bytes.fromhex('f7c508000000')
assert 'sizeof(OmfFixup) == 24'in Path('src/omf-output.h').read_text()and'offsetof(OmfFixup, addend) == 20'in Path('src/omf-output.h').read_text()
if args.abi_only:print('Fixup serializer actual pointer/conditional fields/8 rejects,306-byte extent/bit guards and typed24-byte layout pass');raise SystemExit

node=0x400080;record=0x400400
pack=lambda fmt,*v:struct.pack(fmt,*v)
def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in[(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x3000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 d=bytearray((i*13+5)&255 for i in range(24));d[4]=c['type'];struct.pack_into('<HI',d,6,c['location'],c['flags']);d[12:14]=bytes([c['frame'],c['target']]);struct.pack_into('<HH',d,14,c['frame_datum'],c['target_datum']);struct.pack_into('<I',d,20,c['addend']);u.mem_write(node,bytes(d));buf=bytearray((i*29+7)&255 for i in range(1004));struct.pack_into('<2I',buf,12,c['count'],record+c['offset']);u.mem_write(record,bytes(buf));u.mem_write(0x300800,pack('<2I',0x500000,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,node);u.reg_write(UC_X86_REG_EDX,0xabcdef98);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 # Independent wire specification, including request boundaries and explicitly
 # synthetic child mutations. Untouched padding/owned tails are guards only.
 plans=[]
 def stage(kind,value,change):
  before=(bytes(d),bytes(buf));change();plans.append((kind,value,before,(bytes(d),bytes(buf))))
 def put(data):
  count,cursor=struct.unpack_from('<2I',buf,12);off=cursor-record;assert 20<=off and off+len(data)<=1004;buf[off:off+len(data)]=data;struct.pack_into('<2I',buf,12,(count+len(data))&0xffffffff,cursor+len(data))
 initial_flags=struct.unpack_from('<I',d,8)[0]
 if not initial_flags&8 and not c['addend']:struct.pack_into('<I',d,8,initial_flags|8)
 def begin_change():
  if c['begin_mutation']:
   d[4]=0x85;struct.pack_into('<HI',d,6,0x312,0xdeadbeef);d[12:14]=bytes([2,6]);struct.pack_into('<HH',d,14,0x81,0x8123);struct.pack_into('<I',d,20,0x12345678)
 stage('begin',0x9d,begin_change)
 flags=struct.unpack_from('<I',d,8)[0];location=struct.unpack_from('<H',d,6)[0];typ=struct.unpack_from('<b',d,4)[0]
 locat=((location&255)<<8)|((location>>8)&3)|0x80|(0x40 if flags&1 else 0)|(typ*4)
 fixdat=(0x80 if flags&2 else 0)|((d[12]*16)&255)|(0x08 if flags&4 else 0)|(0x04 if flags&8 else 0)
 put(pack('<H',locat&0xffff)+bytes([(fixdat+(d[13]&3))&255]))
 def index_change(value):
  encoded=bytes([value])if value<128 else bytes([0x80|((value>>8)&255),value&255]);put(encoded)
  if c['index_mutation']:
   # Flags are cached, while later target datum/addend and record state reload.
   f=struct.unpack_from('<I',d,8)[0];struct.pack_into('<I',d,8,f^15);struct.pack_into('<H',d,16,0x8123);struct.pack_into('<I',d,20,0xaabbccdd);count,cursor=struct.unpack_from('<2I',buf,12);struct.pack_into('<2I',buf,12,(count+9)&0xffffffff,cursor+3)
 if not flags&2 and d[12]<=2:
  value=struct.unpack_from('<H',d,14)[0];stage('index',value,lambda:index_change(value))
 if not flags&4:
  value=struct.unpack_from('<H',d,16)[0];stage('index',value,lambda:index_change(value))
 if not flags&8:put(d[20:24])
 begin=names['@omf_begin_record@8']if native else 0x6b480;index=names['@omf_write_index@8']if native else 0x6c2b0;events=[];terminal=False
 def hook(m,a,size,_):
  nonlocal terminal
  if a==0x500000:terminal=True;m.emu_stop();return
  if a not in[begin,index]:return
  step=len(events);assert step<len(plans);kind,value,before,after=plans[step];assert kind==('begin'if a==begin else'index');ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);assert (ecx,edx)==((0x9d,0)if kind=='begin'else(record,value));assert bytes(m.mem_read(node,24))==before[0]and bytes(m.mem_read(record,1004))==before[1];events.append([kind,value]);m.mem_write(node,after[0]);m.mem_write(record,after[1]);esp=m.reg_read(UC_X86_REG_ESP);m.reg_write(UC_X86_REG_EAX,record if kind=='begin'else 0xdeadbeef);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=1000)
 assert terminal and len(events)==len(plans)and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x300804,4))==pack('<I',0xfeedbeef)and bytes(u.mem_read(node,24))==d and bytes(u.mem_read(record,1004))==buf
 return dict(events=events,record_sha256=hashlib.sha256(buf).hexdigest(),fixup_sha256=hashlib.sha256(d).hexdigest())
base=dict(type=9,location=120,flags=7,frame=0,target=1,frame_datum=128,target_datum=65535,addend=40,count=124,offset=68,begin_mutation=False,index_mutation=False);cases=[]
for flags,frame,target,addend in itertools.product(range(16),[0,2,3,5,8,15],[0,3,4,6],[0,0x89abcdef]):cases.append(dict(base,flags=flags,frame=frame,target=target,addend=addend))
for typ,location in itertools.product([0,1,2,5,9,13,0x80,0xff],[0,255,256,1023,65535]):cases.append(dict(base,type=typ,location=location))
for fd,td,count,offset,mutate in itertools.product([0,127,128,65535],[0,127,128,65535],[0xffffffff],[20,950],[False,True]):cases.append(dict(base,flags=0,frame_datum=fd,target_datum=td,count=count,offset=offset,index_mutation=mutate))
for flags,addend,index_mutation in itertools.product([0,7,8,15],[0,40],[False,True]):cases.append(dict(base,flags=flags,addend=addend,begin_mutation=True,index_mutation=index_mutation))
rows=[]
for c in cases:
 a=run(original,False,c);b=run(canonical,True,c);assert a==b;rows.append(dict(inputs=c,result=a))
Path('cases/omf-fixup-entry-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole306-byte original/canonical FIXUPP32 against independent wire/request model. All16 low flags x6 frame methods x4 target shapes xzero/nonzero addend, signed byte types/location truncation,0/127/128/65535 unsigned indices, count wrap/two cursor positions, begin changes and index child changes verify cached flags but later target/addend/record reloads, exact conditional index edges, field/padding/record-tail guards and RET0/saved registers. Begin/index children mocked with explicit bounded state transformations, not cold real-child execution/output claim. Actual pool slot0/count4/type9/location120/flags7/frame0/target1/addend40 separately observed, no padding/raw pool/old destination seed. Full exact OMF artifact remains pending.'),indent=2)+'\n');print(len(rows),'whole FIXUPP32 controls/8 actual ABI rejects pass')
