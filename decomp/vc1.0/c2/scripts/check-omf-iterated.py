"""Actual LIDATA four-formal ABI and full original/canonical record controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_write_iterated_data@16';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6b1d0'and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==8
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name=n,location=l,type=t)for n,l,t in [('segment','ECX','Segment pointer'),('repeat','EDX','signed32'),('bytes','entryESP+4','unsigned32'),('source','entryESP+8','void pointer')]]
 assert e['shape']==dict(repeat=1016,bytes=1,kind=8,combine=2,flags=None,data_hex='00')
 for side in ['original','native']:
  d=e[side];assert all(d[n]==f[side]for n,f in zip(['segment','repeat','bytes','source'],e['formals']))and d['segment']==d['data_segment']and all(d[k]==v for k,v in e['shape'].items())and d['owner']['padding']==d['repeat']
 assert e['original']['return_address']==0x6b19d and e['original']['source_eax']==e['original']['source']and e['native']['source']==e['native']['owner']['source']and e['native']['caller']=='omf_output_relocation'
validate(e);reject=[]
for label,change in [('missing source',lambda e:e.update(formals=e['formals'][:3])),('wrong repeat register',lambda e:e['formals'][1].update(location='ECX')),('wrong source owner',lambda e:e['native']['owner'].update(source=0)),('wrong original zero owner',lambda e:e['original'].update(source_eax=0)),('wrong bytes',lambda e:e['native'].update(bytes=4)),('wrong source data',lambda e:e['native'].update(data_hex='01')),('wrong combine',lambda e:e['native'].update(combine=7)),('invented record input',lambda e:e.update(hidden_inputs=['returned_record']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b1d0 and row['end']==0x6b293 and original.read(0x6b290,3)==b'\xc2\x08\0'
assert original.read(0x6b183,17).hex()=='c7442410000000008d442410506a018bd6'
# Segment field offsets are independently bound by the common data-layout audit.
if args.abi_only:print(name,'actual ABI/8 negatives/195B extent/caller-zero storage checks pass; full controls explicit');raise SystemExit

def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x3000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 segment=0x400080;record=0x400400;source=0x401000;d=bytearray((i*13+5)&255 for i in range(72));d[53]=c['combine'];d[69]=c['flags'];u.mem_write(segment,bytes(d));buf=bytearray((i*29+7)&255 for i in range(1004));struct.pack_into('<2I',buf,12,c['count'],record+23);u.mem_write(record,bytes(buf));data=bytes((i*11+3)&255 for i in range(c['bytes']));u.mem_write(source,data or b'X');u.mem_write(0x300800,struct.pack('<3I',0x500000,c['bytes'],source));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,segment);u.reg_write(UC_X86_REG_EDX,c['repeat']&0xffffffff)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 children={names['@omf_begin_record@8']if native else 0x6b480:'begin',names['@omf_append_bytes@12']if native else 0x6b650:'append'};events=[];expected_flags=c['flags']|2 if c['combine']==7 else c['flags'];d[69]=expected_flags;shift=0 if c['repeat']&1 else 1;repeat=c['repeat']>>shift;header=struct.pack('<I',repeat&0xffffffff)+b'\0\0'+bytes([(c['bytes']<<shift)&255]);appends=0
 def hook(m,a,size,_):
  nonlocal appends
  if a==0x500000:m.emu_stop();return
  if a not in children:return
  k=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',m.mem_read(esp,4))[0];cleanup=0;result=0xdeadbeef
  if k=='begin':
   assert ecx==(0xc3 if c['combine']==7 else 0xa3)and edx==segment and bytes(m.mem_read(segment,72))==d;events.append(['begin',ecx]);result=record
   if c['mutation']:
    d[69]=0xa0;m.mem_write(segment+69,b'\xa0');struct.pack_into('<2I',buf,12,7,record+64);m.mem_write(record+12,bytes(buf[12:20]))
   count,cursor=struct.unpack('<2I',buf[12:20]);buf[cursor-record:cursor-record+7]=header;struct.pack_into('<2I',buf,12,(count+7)&0xffffffff,cursor+7)
  else:
   n=struct.unpack('<I',m.mem_read(esp+4,4))[0];assert ecx==record and edx==source and n==c['bytes']and bytes(m.mem_read(record,1004))==buf;events.append(['append',n]);appends+=1;count,cursor=struct.unpack('<2I',buf[12:20]);buf[cursor-record:cursor-record+n]=data;struct.pack_into('<2I',buf,12,(count+n)&0xffffffff,cursor+n);m.mem_write(record,bytes(buf));cleanup=4
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4+cleanup);m.reg_write(UC_X86_REG_EIP,ret)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=1000)
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x30080c and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(segment,72))==d and bytes(u.mem_read(record,1004))==buf and bytes(u.mem_read(source,len(data)or 1))==(data or b'X')and appends==1+shift
 return dict(events=events,header=header.hex(),flags=d[69],record_sha256=hashlib.sha256(buf).hexdigest())
rows=[]
for combine,flags,repeat,width,count,mutation in itertools.product([2,7],[0,0xa5],[-2147483648,-3,-2,0,1,2,3,1016,0x7fffffff],[0,1,4,128],[0,0xffffffff],[False,True]):
 c=dict(combine=combine,flags=flags,repeat=repeat,bytes=width,count=count,mutation=mutation);a=run(original,False,c);b=run(canonical,True,c);assert a==b;rows.append(dict(inputs=c,result=a))
Path('cases/omf-iterated-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole195B original/canonical LIDATA writer. Signed even-repeat shift, low-byte doubledwidth, four repeat bytes/zero16/header,COMDAT flagOR2 before begin, exact1or2 append calls with same originalwidth/source, begin mutation/caller-saved clobber/unsignedcountwrap, all72segment/1004record/source bytes and callee-saved/RET8 checked. Children mocked; actual kind8/combine2/repeat1016/width1/zero source only observed; coldCOMDAT/synthetic negative widths/counts not claimed compiler execution.'),indent=2)+'\n');print(len(rows),'whole iterated-data controls and8 actual ABI rejects pass')
