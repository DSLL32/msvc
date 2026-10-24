"""Signed OMF communal-size encoding, bounded storage and actual two-formal ABI."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_write_communal_size@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6c600'and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name='record',location='ECX',type='OmfRecord pointer'),dict(name='size',location='EDX',type='signed32')]
 for side in ['original','native']:
  d=e[side];slot,rem=divmod(d['record']-d['base'],1004);assert not rem and slot==e['shape']['slot']and 0<=slot<4 and d['record']==d['current']==e['formals'][0][side]and d['size']==68==e['formals'][1][side]and d['count']==e['shape']['count']and d['cursor']-d['record']==e['shape']['cursor_offset']
 assert e['original']['return_address']==0x6aee2 and e['native']['record']==e['native']['record_owner']and e['native']['size']==e['native']['caller_size']and e['native']['caller']=='omf_emit_communal'
validate(e);reject=[]
for label,change in [('missing size',lambda e:e.update(formals=e['formals'][:1])),('wrong EDX',lambda e:e['formals'][1].update(location='ECX')),('wrong record owner',lambda e:e['native'].update(record_owner=0)),('wrong size',lambda e:e['native'].update(size=70000)),('wrong cursor',lambda e:e['native'].update(cursor=e['native']['cursor']+1)),('invented output snapshot',lambda e:e.update(hidden_inputs=['destination_bytes']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6c600 and row['end']==0x6c674 and original.read(0x6c673,1)==b'\xc3'
if args.abi_only:print(name,'actual ABI/6 negatives/116B extent checks pass; full controls explicit');raise SystemExit

def run(image,native,size,count,offset):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x1000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 record=0x400080;buf=bytearray((i*37+11)&255 for i in range(1004));struct.pack_into('<2I',buf,12,count,record+offset);u.mem_write(record,bytes(buf));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,record);u.reg_write(UC_X86_REG_EDX,size&0xffffffff)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 def hook(m,a,n,_):
  if a==0x500000:m.emu_stop()
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=1000)
 data=bytes([size&255])if size<=127 else bytes([0x81,size&255,(size>>8)&255])if size<=65535 else bytes([0x84,size&255,(size>>8)&255,(size>>16)&255]);buf[offset:offset+len(data)]=data;struct.pack_into('<2I',buf,12,(count+len(data))&0xffffffff,record+offset+len(data))
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(record,1004))==buf
 return dict(encoded=data.hex(),record_sha256=hashlib.sha256(buf).hexdigest())
rows=[]
for size,count,offset in itertools.product([-2147483648,-65536,-1,0,1,126,127,128,255,256,65534,65535,65536,70000,0xffffff,0x1000000,0x7fffffff],[0,0xfffffffe,0xffffffff],[20,27,999]):
 a=run(original,False,size,count,offset);b=run(canonical,True,size,count,offset);assert a==b;rows.append(dict(size=size,count=count,offset=offset,result=a))
Path('cases/omf-communal-size-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole116B original/canonical no-child encoder, independent signed threshold/low1,2,3 byte model incl negative sizes/count wrap/cursor placement. All1004 record bytes, callee-saved registers/RET0 preserved. Signed negative/synthetic high cases are bounded controls, not claimed valid compiler producers; actual size68 only observed before acceptance.'),indent=2)+'\n');print(len(rows),'whole communal-size controls and6 actual ABI rejects pass')
