"""Actual callback ABI, eight-slot dispatch and whole initializer-emission controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_output_relocation@12';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6afc0'and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==4
 expected=[dict(name='initializer',location='ECX',type='Initializer pointer'),dict(name='symbol',location='EDX',type='SymbolHeader pointer'),dict(name='segment',location='entryESP+4',type='Segment pointer')];assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==expected
 payload=dict(datum_hex='06002600',repeat_hex='01000000',data_hex=b'typed OMF relocation and debug scopes\0'.hex());assert e['kind']==1 and e['normalized_payload']==payload and e['original']['return_address']==0x693f8 and e['native']['caller']=='emit_initializer'and e['native']['initializer_size']==28
 for side in ['original','native']:
  d=e[side];assert d['kind_hex']=='01000000'and d['combine']==2 and d['position']==0 and d['segment_aliases']==['data']and d['payload']['data_pointer']and d['payload']['data_pointer']!=d['initializer']+12
  for i,(key,owner)in enumerate([('initializer','loop_owner'),('symbol','symbol_owner'),('segment','segment_owner')]):assert d[key]==d[owner]==e['formals'][i][side]and d[key]
  assert {k:v for k,v in d['payload'].items()if k!='data_pointer'}==payload
validate(e);reject=[]
for label,change in [('missing unused Symbol formal',lambda e:e['formals'].pop(1)),('wrong Segment stack slot',lambda e:e['formals'][2].update(location='entryESP+8')),('wrong RET cleanup',lambda e:e.update(callee_stack_cleanup=0)),('wrong initializer owner',lambda e:e['native'].update(loop_owner=0)),('wrong unused Symbol owner',lambda e:e['original'].update(symbol_owner=0)),('wrong Segment owner',lambda e:e['native'].update(segment_owner=0)),('wrong combine',lambda e:e['native'].update(combine=7)),('missing active bytes',lambda e:e['native']['payload'].update(data_hex='')),('wrong repeat',lambda e:e['native']['payload'].update(repeat_hex='02000000')),('unproved hidden local input',lambda e:e.update(hidden_inputs=['value']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6afc0 and row['end']==0x6b1a9 and row['shared']==[[0x6b1ac,0x6b1cc]]and original.read(0x6b1a6,3)==b'\xc2\x04\0'
targets=list(struct.unpack('<8I',original.read(0x6b1ac,32)));assert targets==[0x6b059,0x6b0b0,0x6b19d,0x6b147,0x6b19d,0x6b044,0x6b044,0x6b170]
assert original.read(0x6b037,13)==bytes.fromhex('4d83fd077707ff24adacb10600')
MASK=0xffffffff
def signed(v,bits=32):return v-(1<<bits)if v&(1<<(bits-1))else v
def remainder(a,b):
 assert b and not(a==-2147483648 and b==-1)
 q=abs(a)//abs(b);q=-q if(a<0)!=(b<0)else q
 return a-q*b

if args.abi_only:
 print(name,'actual ABI/negative/extent/storage checks pass; full semantic grid is explicit');raise SystemExit

def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x3000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 node=0x400080;segment=0x400280;record=0x400400;reference=0x400900;data=0x400a00;d=bytearray((i*13+5)&255 for i in range(64));struct.pack_into('<i',d,0,c['kind']);struct.pack_into('<I',d,24,c['repeat'])
 if c['kind']==1:
  struct.pack_into('<2h',d,8,c['type'],c['width']);struct.pack_into('<I',d,12,data if c['type']==6 else 0x89abcdef)
 elif c['kind']==2:struct.pack_into('<2I',d,8,7,c['addend']);d[16]=c['flag']&255
 elif c['kind']==4:d[8]=c['divisor']&255
 elif c['kind']==8:struct.pack_into('<I',d,8,c['padding']&MASK)
 u.mem_write(node,bytes(d));seg=bytearray((i*17+3)&255 for i in range(72));seg[53]=c['combine'];u.mem_write(segment,bytes(seg));buf=bytearray((i*29+7)&255 for i in range(1004));struct.pack_into('<5I',buf,0,0xa1,segment,c['position'],c['count'],record+23);u.mem_write(record,bytes(buf));ref=bytearray((i*19+9)&255 for i in range(84));ref[12]=c['referenced_kind']&255;ref[48]=c['label_kind'];u.mem_write(reference,bytes(ref));source=bytes((i*23+1)&255 for i in range(64));u.mem_write(data,source)
 # Guard bytes make the actually initialized kind8 DWORD zero observable.
 # They are not supplied as a defined original uninitialized-local parameter.
 u.mem_write(0x300000,b'\xa5'*0x10000);u.mem_write(0x300800,struct.pack('<3I',0x500000,segment,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,node);u.reg_write(UC_X86_REG_EDX,c['unused_symbol'])
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 specs=[('begin','@omf_begin_record@8',0x6b480),('lookup','@lookup_global_symbol@4',0x4dce0),('byte-array','@omf_write_byte_array@16',0x6b3d0),('iterated','@omf_write_iterated_data@16',0x6b1d0),('datum','@omf_write_datum@8',0x6b2d0),('fixup','@omf_output_bytes@8',0x6b2c0),('pad','@omf_pad_record@8',0x6b2a0),('error','@compiler_internal_error@8',0x6fb70)];children={names[n]if native else a:k for k,n,a in specs};events=[];terminal=None;record_created=False;expected_result=MASK
 def hook(m,a,size,_):
  nonlocal terminal,record_created,expected_result
  if a==0x500000:terminal='return';m.emu_stop();return
  if a not in children:return
  k=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);result=0xdeadbeef;pop=4
  if k=='error':
   text=b''
   for i in range(32):
    b=bytes(m.mem_read(ecx+i,1));text+=b
    if b==b'\0':break
   expect=0x1c8 if c['kind']==2 and ref[12]==3 and ref[48]!=4 else 0x1da if c['kind']==2 else 0x1fe;assert text==b'@(#)omf.c:1.10\0'and edx==expect;events.append(['error',edx]);terminal='error';m.emu_stop();return
  if k=='begin':
   assert not record_created and ecx==(0xc3 if c['combine']==7 else 0xa1)and edx==segment;events.append(['begin',ecx]);record_created=True;result=record
   if c['mutation'].startswith('begin'):
    struct.pack_into('<i',d,0,9)
    if c['mutation']=='begin-byte-array':struct.pack_into('<2h',d,8,6,3);struct.pack_into('<I',d,12,data);struct.pack_into('<I',d,24,2)
    elif c['mutation']=='begin-iterated':struct.pack_into('<2h',d,8,1,2);struct.pack_into('<I',d,24,2)
    elif c['mutation']=='begin-reference':struct.pack_into('<I',d,8,8);d[16]=0x80
    elif c['mutation']=='begin-alignment':d[8]=0xfe;struct.pack_into('<2I',buf,8,0xfffffffd,0);m.mem_write(record,bytes(buf))
    elif c['mutation']=='begin-padding':struct.pack_into('<I',d,8,25)
    m.mem_write(node,bytes(d))
  elif k=='lookup':
   assert c['kind']==2 and ecx==struct.unpack_from('<I',d,8)[0];events.append(['lookup',ecx]);result=reference;expected_result=signed(d[16],8)&MASK
   if c['mutation']=='lookup':
    d[16]=0x80;struct.pack_into('<I',d,12,0x10203040);ref[12]=1;m.mem_write(node,bytes(d));m.mem_write(reference,bytes(ref))
  elif k in ['byte-array','iterated']:
   third,pointer=struct.unpack('<2I',m.mem_read(esp+4,8));assert ecx==segment;pop=12
   if c['kind']==1:
    typ,width=struct.unpack_from('<2h',d,8);repeat=struct.unpack_from('<I',d,24)[0];expected_result=(width*repeat)&MASK
    if k=='byte-array':assert typ==6 and edx==width&MASK and third==repeat and pointer==data;events.append([k,width,repeat,'owned-data'])
    else:assert typ!=6 and signed(repeat)>1 and edx==repeat and third==width&MASK and pointer==node+12;events.append([k,signed(repeat),width,'inline-value'])
   else:
    padding=struct.unpack_from('<I',d,8)[0];assert c['kind']==8 and signed(padding)>=20 and k=='iterated'and edx==padding and third==1 and 0x300000<=pointer<=0x30fffc and bytes(m.mem_read(pointer,4))==bytes(4);expected_result=padding;events.append([k,signed(padding),1,'initialized-zero4'])
  elif k=='datum':
   typ,width=struct.unpack_from('<2h',d,8);repeat=struct.unpack_from('<I',d,24)[0];assert c['kind']==1 and typ!=6 and signed(repeat)<=1 and ecx==(record if record_created else 0)and edx==node;events.append([k,'record'if record_created else'NULL']);result=c['scalar_result'];expected_result=result
  elif k=='pad':
   padding=struct.unpack_from('<I',d,8)[0];assert c['kind']==8 and signed(padding)<20 and ecx==record and record_created and edx==padding;expected_result=padding;events.append([k,signed(padding)])
  else:
   assert k=='fixup'and c['kind']==2 and ecx==0xfffffffc and edx==reference
   rk=signed(ref[12],8);assert rk in [1,3,4]and(rk!=3 or ref[48]==4);value=struct.unpack_from('<I',d,12)[0]if rk==1 else 0;count,cursor=struct.unpack_from('<2I',buf,12);buf[cursor-record:cursor-record+4]=struct.pack('<I',value);struct.pack_into('<2I',buf,12,(count+4)&MASK,cursor+4);assert bytes(m.mem_read(record,1004))==buf;events.append([k,-4,'reference',value])
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+pop);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=3000)
 should_begin=(c['kind']!=1 or c['repeat']==1)and(c['kind']!=1 or c['type']!=6)and(c['kind']!=8 or c['padding']<20);assert record_created==should_begin
 if terminal=='return':
  if c['kind']==4:
   count=struct.unpack_from('<I',buf,12)[0];position=struct.unpack_from('<I',buf,8)[0];divisor=signed(d[8],8);rem=remainder(signed((position+count)&MASK),divisor);expected_result=rem&MASK
   if rem:
    cursor=struct.unpack_from('<I',buf,16)[0];buf[cursor-record]=0;struct.pack_into('<2I',buf,12,(count+1)&MASK,cursor+1)
  assert u.reg_read(UC_X86_REG_EAX)==expected_result and u.reg_read(UC_X86_REG_ESP)==0x300808 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x300808,4))==struct.pack('<I',0xfeedbeef)
 else:assert terminal=='error'
 assert bytes(u.mem_read(node,64))==d and bytes(u.mem_read(segment,72))==seg and bytes(u.mem_read(reference,84))==ref and bytes(u.mem_read(data,64))==source and bytes(u.mem_read(record,1004))==buf
 return dict(terminal=terminal,returned=expected_result if terminal=='return'else None,events=events,node_sha256=hashlib.sha256(d).hexdigest(),record_sha256=hashlib.sha256(buf).hexdigest())
def case(kind,**kw):
 c=dict(kind=kind,type=1,width=4,repeat=1,addend=0x89abcdef,flag=4,divisor=2,padding=1,combine=2,count=0,position=0,referenced_kind=4,label_kind=4,unused_symbol=0xdeadbeef,scalar_result=0x12345678,mutation='none');c.update(kw);return c
cases=[]
for width in [1,2,4,8,10]:cases.append(case(1,width=width))
for typ,width,repeat in [(5,4,1),(1,2,2),(1,10,3),(6,38,1),(6,3,2),(6,3,0),(6,-1,2),(1,4,0),(1,4,0xffffffff),(1,4,0x80000000)]:cases.append(case(1,type=typ,width=width,repeat=repeat))
for mutation in ['begin-byte-array','begin-iterated']:cases.append(case(1,mutation=mutation))
for rk,flag,combine,count in itertools.product([1,3,4],[4,-1],[2,7],[0,MASK]):cases.append(case(2,referenced_kind=rk,flag=flag,combine=combine,count=count))
cases.extend([case(2,mutation='lookup'),case(2,mutation='begin-reference'),case(2,referenced_kind=3,label_kind=0),case(2,referenced_kind=7)])
for divisor,position,count in [(2,0,0),(2,0,1),(4,0,3),(8,0,7),(-2,0xfffffffd,0),(4,0x7fffffff,2),(-128,0,10),(2,0xffffffff,MASK)]:cases.append(case(4,divisor=divisor,position=position,count=count))
cases.append(case(4,mutation='begin-alignment'))
for padding in [0,1,19,20,21,0x7fffffff,-1]:cases.append(case(8,padding=padding))
cases.append(case(8,padding=19,mutation='begin-padding'))
for kind in [3,5,0,6,7,9,-1]:cases.append(case(kind))
cases.append(case(3,mutation='begin-tag'));cases.append(case(2,referenced_kind=1,unused_symbol=0))
rows=[]
for c in cases:
 a=run(original,False,c);b=run(canonical,True,c);assert a==b,(c,a,b);rows.append(dict(inputs=c,result=a))
Path('cases/omf-relocation-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,dispatch_targets=[hex(v)for v in targets],seconds=time.perf_counter()-started,scope='Whole489-byte original/canonical dispatcher plus32-byte8-slot bounds/target proof; all kinds1..8/outside, preselection conditions, cached kind/post-begin operand reloads, signed short multiplication low32, repeated/byte/scalar exact child ABI, lookup flags cached before mutation and addend/kind reloaded, LE32/fixup(-4), signed remainder ONE-byte padding behavior and FFFFFFFF sentinel, initialized zero4 local under poison guards, RET4/callee-saved and all owned node/Segment/reference/data/record bytes checked. Children mocked; zero/negative/huge repeat/width/padding cases are request-shape/arithmetic controls only, no physical oversized copy success or valid frontend claim. Invalid divisor0/INT_MIN%-1 not executed or guarded; invalid tags/references/labels stop actual1fe/1da/1c8 diagnostics, no returning-fatal/local seed claim. Actual first type6/38B/repeat1 and3 caller-owned formals observed separately, unused Symbol formal still captured.'),indent=2)+'\n');print(len(rows),'whole OMF relocation controls and10 actual ABI rejects pass')
