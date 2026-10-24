"""Four-formal ownership and whole repeated-byte record-chunking controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_write_byte_array@16';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
params=[dict(name=n,location=l,type=t)for n,l,t in [('segment','ECX','Segment pointer'),('bytes','EDX','unsigned32'),('repeat','entryESP+4','unsigned32'),('source','entryESP+8','void pointer')]]
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6b3d0'and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==8 and e['no_hidden_inputs_proof']
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==params
 assert e['original']['return_address']==0x6b07a and e['native']['caller']=='omf_output_relocation'
 for side in ['original','native']:
  d=e[side];assert d['segment']==d['data_segment']and d['source']==d['owner_source']and d['bytes']==d['owner_width']==38 and d['repeat']==d['owner_repeat']==1 and d['combine']==2 and d['position']==0 and d['data_hex']==b'typed OMF relocation and debug scopes\0'.hex()
  assert [f[side]for f in e['formals']]==[d[k]for k in ['segment','bytes','repeat','source']]
validate(e);reject=[]
for label,change in [('missing source',lambda e:e['formals'].pop()),('wrong repeat stack slot',lambda e:e['formals'][2].update(location='entryESP+8')),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('wrong source owner',lambda e:e['original'].update(owner_source=0)),('wrong segment owner',lambda e:e['native'].update(data_segment=0)),('wrong width',lambda e:e['native'].update(bytes=37)),('wrong repeat',lambda e:e['original'].update(repeat=2)),('missing source bytes',lambda e:e['native'].update(data_hex='')),('invented incoming flags',lambda e:e.update(hidden_inputs=['EFLAGS']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b3d0 and row['end']==0x6b47c and original.read(0x6b479,3)==b'\xc2\x08\0'
MASK=0xffffffff

def expected(c):
 left=c['width'];repeat=c['repeat'];limit=(0x7fffffff-c['position'])&MASK;position=c['position'];combine=c['combine'];offset=0;events=[];step=0
 while True:
  assert step<200
  cap=c['capacities'][step%len(c['capacities'])];events.append(['begin',0xc3 if combine==7 else 0xa1,position]);count=min(limit,left,cap)
  # The mocked begin child may mutate both consumed Segment fields.
  if c['mutate']:position=(position+7)&MASK;combine=7 if combine!=7 else 2
  events.append(['append'if c['source']else'pad',count,offset if c['source']else None]);position=(position+(9 if c['mutate']else 0)+count)&MASK
  if c['source']:offset+=count
  left-=count;limit-=count
  if not left:
   if c['source']:offset-=c['width']
   left=c['width'];repeat-=1
  step+=1
  if not repeat:return events,combine

if args.abi_only:
 print(name,'actual ABI/negative/extent/storage checks pass; full semantic grid is explicit');raise SystemExit

def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x5000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 segment=0x400080;record=0x400400;data=0x402000;seg=bytearray((i*17+3)&255 for i in range(72));seg[53]=c['combine'];struct.pack_into('<I',seg,4,c['position']);u.mem_write(segment,bytes(seg));buf=bytearray((i*29+7)&255 for i in range(1004));u.mem_write(record,bytes(buf));source=bytes((i*23+1)&255 for i in range(max(c['width'],1)));u.mem_write(data,source);u.mem_write(0x300800,struct.pack('<4I',0x500000,c['repeat'],data if c['source']else 0,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,segment);u.reg_write(UC_X86_REG_EDX,c['width']);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 specs=[('begin','@omf_begin_record@8',0x6b480),('append','@omf_append_bytes@12',0x6b650),('pad','@omf_pad_record@8',0x6b2a0)];children={names[n]if native else a:k for k,n,a in specs};events=[];steps=0;terminal=False
 def hook(m,a,size,_):
  nonlocal steps,terminal
  if a==0x500000:terminal=True;m.emu_stop();return
  if a not in children:return
  k=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);pop=4;result=0xdeadbeef
  if k=='begin':
   assert edx==segment and ecx==(0xc3 if seg[53]==7 else 0xa1);actual=bytes(m.mem_read(segment,72));position=struct.unpack_from('<I',actual,4)[0];struct.pack_into('<I',seg,4,position);assert actual==seg;events.append([k,ecx,position]);cap=c['capacities'][steps%len(c['capacities'])];struct.pack_into('<5I',buf,0,ecx,segment,position,17,record+998-cap);m.mem_write(record,bytes(buf));result=record;steps+=1
   if c['mutate']:struct.pack_into('<I',seg,4,(position+7)&MASK);seg[53]=7 if seg[53]!=7 else 2;m.mem_write(segment,bytes(seg))
  else:
   assert ecx==record
   if k=='append':
    count=struct.unpack('<I',m.mem_read(esp+4,4))[0];pop=8;offset=edx-data;assert c['source']and 0<=offset<=c['width']and offset+count<=c['width']and bytes(m.mem_read(edx,count))==source[offset:offset+count];events.append([k,count,offset])
   else:assert not c['source'];count=edx;events.append([k,count,None])
   if c['mutate']:struct.pack_into('<I',seg,4,(struct.unpack('<I',m.mem_read(segment+4,4))[0]+9)&MASK);m.mem_write(segment,bytes(seg))
  # Mirror only child-owned mutations; the parent's position update remains real.
  if k=='begin':pass
  elif not c['mutate']:seg[:]=m.mem_read(segment,72)
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+pop);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=15000);want,combine=expected(c);assert terminal and events==want,(c,events,want)
 struct.pack_into('<I',seg,4,c['position']);seg[53]=combine
 assert bytes(u.mem_read(segment,72))==seg and bytes(u.mem_read(record,1004))==buf and bytes(u.mem_read(data,len(source)))==source
 assert u.reg_read(UC_X86_REG_ESP)==0x30080c and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x30080c,4))==struct.pack('<I',0xfeedbeef)
 return dict(events=events,segment_sha256=hashlib.sha256(seg).hexdigest(),record_sha256=hashlib.sha256(buf).hexdigest())
cases=[]
for width,repeat,source,combine,mutate in itertools.product([0,1,38,1000],[1,3],[False,True],[2,7],[False,True]):cases.append(dict(width=width,repeat=repeat,source=source,combine=combine,position=100,capacities=[5,978],mutate=mutate))
for position in [0,0x7fffffff-120,0x80000000,0xffffffff]:cases.append(dict(width=38,repeat=2,source=True,combine=2,position=position,capacities=[38],mutate=False))
rows=[]
for c in cases:
 a=run(original,False,c);b=run(canonical,True,c);assert a==b;rows.append(dict(inputs=c,result=a))
Path('cases/omf-byte-array-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole172-byte original/canonical controls against independent chunk/repeat model:0/1/38/1000-byte patterns, one/three repeats, record capacity split5/978, source rewind or null padding, combine2/7 tags, unsigned min and position wrap, post-child Segment reloads and final entry-position restoration, exact4-formal child ABI/RET8/callee-saved/owned guards. Child begin/append/pad effects mocked; original restore does not erase child combine mutations.64 bounded core states+4 positions, no repeat0 or nonprogress domain success, no invented guard or source copy claim outside owned width. Actual38-byte pattern/repeat1/data Segment independently observed.'),indent=2)+'\n');print(len(rows),'whole byte-array controls and9 actual ABI rejects pass')
