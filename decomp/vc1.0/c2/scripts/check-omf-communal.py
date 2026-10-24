"""COMDEF writer: actual ABI/storage plus independent whole-body controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_emit_communal@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6ae90'and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name='symbol',location='ECX',type='SymbolHeader pointer')]
 assert e['size']==68 and e['type_index']==4107
 for side in ['original','native']:
  d=e[side];assert d['symbol']==d['member']==e['formals'][0][side]and d['symbol']and d['bucket']==26 and d['index']==0 and d['next_external']==1
  assert {k:d[k]for k in ['header_hex','identifier_hex','attributes_hex','name_hex','value_hex','type_hex']}==dict(header_hex='01000009',identifier_hex='9a000000',attributes_hex='a6010100',name_hex=b'_packet_small\0'.hex(),value_hex='44000000',type_hex='0b10')
validate(e);reject=[]
for label,change in [('missing pointer',lambda e:e.update(formals=[])),('wrong ECX',lambda e:e['formals'][0].update(location='EDX')),('wrong owner',lambda e:e['native'].update(member=0)),('wrong external counter',lambda e:e['original'].update(next_external=0)),('wrong size',lambda e:e['native'].update(value_hex='00000000')),('wrong type',lambda e:e['native'].update(type_hex='ffff')),('missing name bytes',lambda e:e['native'].update(name_hex='')),('invented old output input',lambda e:e.update(hidden_inputs=['old_field14']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6ae90 and row['end']==0x6aee5 and original.read(0x6aee4,1)==b'\xc3'
assert original.read(0x85078,4)==canonical.read(names['_omf_next_external'],4)==b'\x01\0\0\0'
assert len({names['_omf_next_external'],names['_omf_next_group'],names['_omf_next_segment'],names['_omf_next_name']})==4
if args.abi_only:
 print(name,'actual ABI/8 negatives/extent/storage checks pass; full controls explicit');raise SystemExit

def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 symbol=0x400080;record=0x400400;text=0x400900;counter=names['_omf_next_external']if native else 0x85078;d=bytearray((i*13+5)&255 for i in range(128));struct.pack_into('<I',d,20,c['old_output']);struct.pack_into('<I',d,8,c['size']);struct.pack_into('<H',d,32,c['type']);u.mem_write(symbol,bytes(d));buf=bytearray((i*29+7)&255 for i in range(1004));struct.pack_into('<5I',buf,0,0xb0,0,0,c['count'],record+23);u.mem_write(record,bytes(buf));u.mem_write(text,b'shown\0');u.mem_write(counter,struct.pack('<I',c['counter']));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,symbol)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 specs=[('begin','@omf_begin_record@8',0x6b480),('display','@symbol_display_name@4',0x4e000),('string','@omf_append_string@8',0x6c4e0),('index','@omf_write_index@8',0x6c2b0),('size','@omf_write_communal_size@8',0x6c600)];children={names[n]if native else a:k for k,n,a in specs};events=[];assigned=0x80000000 if c['mutation']=='begin'else c['counter']
 def append(m,data):
  cursor,count=struct.unpack('<I',m.mem_read(record+16,4))[0],struct.unpack('<I',m.mem_read(record+12,4))[0];m.mem_write(cursor,data);m.mem_write(record+12,struct.pack('<I',(count+len(data))&0xffffffff));m.mem_write(record+16,struct.pack('<I',cursor+len(data)));buf[cursor-record:cursor-record+len(data)]=data;struct.pack_into('<2I',buf,12,(count+len(data))&0xffffffff,cursor+len(data))
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a not in children:return
  k=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);result=0xdeadbeef
  if k=='begin':
   assert ecx==0xb0 and edx==0 and bytes(m.mem_read(symbol+20,4))==struct.pack('<I',c['old_output']);events.append(['begin',ecx,edx]);result=record
   if c['mutation']=='begin':m.mem_write(counter,struct.pack('<I',assigned))
  elif k=='display':
   assert ecx==symbol and bytes(m.mem_read(symbol+20,4))==struct.pack('<I',assigned)and bytes(m.mem_read(counter,4))==struct.pack('<I',(assigned+1)&0xffffffff);events.append(['display',assigned]);result=text
   if c['mutation']=='display':m.mem_write(record+12,struct.pack('<I',10));m.mem_write(record+16,struct.pack('<I',record+64));struct.pack_into('<2I',buf,12,10,record+64)
  elif k=='string':
   assert ecx==record and edx==text;events.append(['string']);append(m,b'\x02XY')
   if c['mutation']=='string':m.mem_write(symbol+32,b'\xff\xff');struct.pack_into('<H',d,32,65535)
  elif k=='index':
   expected=-1 if c['mutation']=='string'else struct.unpack('<h',struct.pack('<H',c['type']))[0];assert ecx==record and edx==expected&0xffffffff;events.append(['index',expected]);append(m,b'\x91\x23')
   if c['mutation']=='index':m.mem_write(symbol+8,struct.pack('<I',70000));struct.pack_into('<I',d,8,70000)
  else:
   assert ecx==record and edx==(70000 if c['mutation']=='index'else c['size']);events.append(['size',edx]);append_expected=b'\x62';count,cursor=struct.unpack('<2I',buf[12:20]);buf[cursor-record]=0x62;struct.pack_into('<2I',buf,12,(count+1)&0xffffffff,cursor+1);assert bytes(m.mem_read(record,1004))==buf
   if c['mutation']=='size':m.mem_write(counter,struct.pack('<I',7))
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=1000);struct.pack_into('<I',d,20,assigned);end_counter=7 if c['mutation']=='size'else(assigned+1)&0xffffffff
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(symbol,128))==d and bytes(u.mem_read(record,1004))==buf and bytes(u.mem_read(text,6))==b'shown\0'and struct.unpack('<I',u.mem_read(counter,4))[0]==end_counter and len(events)==5
 return dict(events=events,index=assigned,next_external=end_counter,record_sha256=hashlib.sha256(buf).hexdigest())
rows=[]
for counter,count,old,typ,size,mutation in itertools.product([1,0xffffffff],[0,0xffffffff],[0,0xa5a5a5a5],[0x100b,0x8000],[68,0xffffffff],['none','begin','display','string','index','size']):
 c=dict(counter=counter,count=count,old_output=old,type=typ,size=size,mutation=mutation);a=run(original,False,c);b=run(canonical,True,c);assert a==b;rows.append(dict(inputs=c,result=a))
Path('cases/omf-communal-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole85B original/canonical COMDEF writer. Child order/tag, counter loaded after begin, output assignment/counter wrap, signed type reload after name children, raw62/cursor/count after index, size reload after index, caller-saved clobbers, all128 symbol/1004 record/6 text bytes and callee-saved/RET0 checked. Synthetic patterned old output is overwrite guard, never actual input dump/seed. Child encoding mocked: no actual child semantics/completed artifact claim.'),indent=2)+'\n');print(len(rows),'whole COMDEF controls and8 actual ABI rejects pass')
