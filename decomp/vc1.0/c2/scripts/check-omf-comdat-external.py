"""CEXTDEF actual ABI/storage and full index/lookup/record timing controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_emit_comdat_external@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
expected=dict(identifier=161,bucket=33,index=0,target_id=183,target_bucket=55,target_index=0,target_identifier=183,object_section=17,next_external=4)
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6ae00'and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name='symbol',location='ECX',type='SymbolHeader pointer')]and e['shape']==expected
 for side in ['original','native']:
  d=e[side];assert d['symbol']==d['member']==e['formals'][0][side]and d['symbol']and d['target']and all(d[k]==v for k,v in expected.items())
 assert e['original']['return_address']==0x6ac65 and e['native']['caller']=='omf_mark_symbol'
validate(e);reject=[]
for label,change in [('missing formal',lambda e:e.update(formals=[])),('wrong ECX',lambda e:e['formals'][0].update(location='EDX')),('wrong source owner',lambda e:e['native'].update(member=0)),('wrong lookup ID',lambda e:e['native'].update(target_id=1)),('wrong name index',lambda e:e['native'].update(object_section=0)),('wrong counter',lambda e:e['native'].update(next_external=1)),('invented old output',lambda e:e.update(hidden_inputs=['field14']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6ae00 and row['end']==0x6ae42 and original.read(0x6ae41,1)==b'\xc3'
assert original.read(0x85078,4)==canonical.read(names['_omf_next_external'],4)==b'\x01\0\0\0'
if args.abi_only:print(name,'actual ABI/7 negatives/66B extent/counter storage checks pass; full controls explicit');raise SystemExit

def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 symbol,record,segment=0x400080,0x400400,0x400a00;counter=names['_omf_next_external']if native else 0x85078;d=bytearray((i*13+5)&255 for i in range(128));struct.pack_into('<2I',d,20,c['old_output'],183);u.mem_write(symbol,bytes(d));target=bytearray((i*23+9)&255 for i in range(72));struct.pack_into('<I',target,56,c['index']);u.mem_write(segment,bytes(target));buf=bytearray((i*29+7)&255 for i in range(1004));struct.pack_into('<5I',buf,0,0xbc,0,0,c['count'],record+23);u.mem_write(record,bytes(buf));u.mem_write(counter,struct.pack('<I',c['counter']));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,symbol)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 specs=[('begin','@omf_begin_record@8',0x6b480),('lookup','@lookup_global_symbol@4',0x4dce0),('index','@omf_write_index@8',0x6c2b0)];children={names[n]if native else a:k for k,n,a in specs};events=[];assigned=0x80000000 if c['mutation']=='begin'else c['counter'];index=0xffffffff if c['mutation']=='lookup'else c['index']
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a not in children:return
  k=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);result=0xdeadbeef
  if k=='begin':
   assert ecx==0xbc and edx==0 and bytes(m.mem_read(symbol+20,4))==struct.pack('<I',c['old_output']);events.append(['begin',ecx,edx]);result=record
   if c['mutation']=='begin':m.mem_write(counter,struct.pack('<I',assigned));struct.pack_into('<I',d,24,7);m.mem_write(symbol+24,bytes(d[24:28]))
  elif k=='lookup':
   assert ecx==(7 if c['mutation']=='begin'else 183)and bytes(m.mem_read(symbol+20,4))==struct.pack('<I',assigned)and bytes(m.mem_read(counter,4))==struct.pack('<I',(assigned+1)&0xffffffff);events.append(['lookup',ecx]);result=segment
   if c['mutation']=='lookup':struct.pack_into('<I',target,56,index);m.mem_write(segment+56,bytes(target[56:60]))
  else:
   assert ecx==record and edx==index;events.append(['index',edx]);cursor,count=struct.unpack('<I',m.mem_read(record+16,4))[0],struct.unpack('<I',m.mem_read(record+12,4))[0];m.mem_write(cursor,b'\x91\x23');m.mem_write(record+12,struct.pack('<I',(count+2)&0xffffffff));m.mem_write(record+16,struct.pack('<I',cursor+2));buf[cursor-record:cursor-record+2]=b'\x91\x23';struct.pack_into('<2I',buf,12,(count+2)&0xffffffff,cursor+2)
   if c['mutation']=='index':m.mem_write(counter,struct.pack('<I',7))
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=1000);count,cursor=struct.unpack('<2I',buf[12:20]);buf[cursor-record]=0;struct.pack_into('<2I',buf,12,(count+1)&0xffffffff,cursor+1);struct.pack_into('<I',d,20,assigned);end_counter=7 if c['mutation']=='index'else(assigned+1)&0xffffffff
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(symbol,128))==d and bytes(u.mem_read(record,1004))==buf and bytes(u.mem_read(segment,72))==target and struct.unpack('<I',u.mem_read(counter,4))[0]==end_counter and len(events)==3
 return dict(events=events,index=assigned,next_external=end_counter,record_sha256=hashlib.sha256(buf).hexdigest())
rows=[]
for counter,count,old,index,mutation in itertools.product([1,0xffffffff],[0,0xffffffff],[0,0xa5a5a5a5],[17,0x80000000],['none','begin','lookup','index']):
 c=dict(counter=counter,count=count,old_output=old,index=index,mutation=mutation);a=run(original,False,c);b=run(canonical,True,c);assert a==b;rows.append(dict(inputs=c,result=a))
Path('cases/omf-comdat-external-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole66B original/canonical CEXTDEF. Counter loaded afterbegin/assignmentbeforelookup, symbol ID reload afterbegin, target.object_section reload afterlookup, full unsignedDWORD forwarded to index child, count/cursor reload afterindex, rawzero type, wraps/caller-savedclobbers/128symbol+72segment+1004recordbytes/callee-saved/RET0. Child semantics mocked; actual sourceid161/segmentid183/nameindex17/counter4 observed, no original output-field input dump/seed or complete artifact claim.'),indent=2)+'\n');print(len(rows),'whole CEXTDEF controls and7 actual ABI rejects pass')
