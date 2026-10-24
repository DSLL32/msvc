"""Actual marked-global ABI and whole OMF symbol classification controls."""
from pathlib import Path
import copy,hashlib,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_mark_symbol@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());ref=Path(e['original_reference']['directory']);binding=ref/'original-binding.json';assert hashlib.sha256(binding.read_bytes()).hexdigest()==e['original_reference']['binding_sha256'];bound=json.loads(binding.read_text());assert all(hashlib.sha256((ref/n).read_bytes()).hexdigest()==h for n,h in bound['files'].items())
def fields(d):
 h=bytes.fromhex(d['header_hex']);i=bytes.fromhex(d['identity_hex']);f=bytes.fromhex(d['flags_hex']);assert len(h)==4 and len(i)==8 and len(f)==6
 return dict(kind=struct.unpack('<b',h[:1])[0],symbol_flags=struct.unpack('<H',h[2:])[0],segment_identifier=struct.unpack('<I',i[:4])[0],identifier=struct.unpack('<I',i[4:])[0],output_flags=struct.unpack('<H',f[:2])[0],attributes=struct.unpack('<I',f[2:])[0],name_hex=d['name_hex'])
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6ab20'and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name='symbol',location='ECX',type='SymbolHeader pointer')]
 expected=dict(kind=1,symbol_flags=0x980,segment_identifier=2,identifier=121,output_flags=4,attributes=0xa3,name_hex=b'_omf_callback\0'.hex());assert e['consumed_fields']==expected
 for side in ['original','native']:
  d=e[side];assert d['pointer']==d['member']==e['formals'][0][side]and d['pointer']and d['bucket']==57 and d['index']==0 and fields(d)==expected;assert bytes.fromhex(d['header_hex'])[1]==0  # observed SymbolHeader.field0d naming discriminator
validate(e);reject=[]
for label,change in [('missing pointer',lambda e:e.update(formals=[])),('wrong ECX',lambda e:e['formals'][0].update(location='EDX')),('wrong owner',lambda e:e['native'].update(member=0)),('wrong hash bucket',lambda e:e['original'].update(bucket=0)),('wrong output flags',lambda e:e['native'].update(flags_hex='0500'+e['native']['flags_hex'][4:])),('wrong storage class',lambda e:e['native'].update(flags_hex=e['native']['flags_hex'][:4]+'a3010000')),('missing input name',lambda e:e['native'].update(name_hex='')),('unproved hidden register',lambda e:e.update(hidden_inputs=['EAX']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6ab20 and row['end']==0x6acb7 and original.read(0x6acb6,1)==b'\xc3'

def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 symbol=0x400080;segment=0x400280;text=0x400480;d=bytearray((i*13+5)&255 for i in range(128));d[12]=c['kind']&255;struct.pack_into('<H',d,14,0x20 if c['comdat']else 0x980);struct.pack_into('<I',d,16,text);struct.pack_into('<2I',d,24,2,121);struct.pack_into('<H',d,34,c['flags']);struct.pack_into('<I',d,36,c['attrs']);u.mem_write(symbol,bytes(d));seg=bytearray((i*17+3)&255 for i in range(72));struct.pack_into('<I',seg,56,c['section']);seg[69]=0x81;u.mem_write(segment,bytes(seg));u.mem_write(text,b'printed_name\0');u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,symbol)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 specs=[('lookup','@lookup_global_symbol@4',0x4dce0),('display','@symbol_display_name@4',0x4e000),('public-name','@omf_name_index@4',0x6a940),('local-name','@omf_local_name_index@4',0x6ae50),('comdat','@omf_emit_comdat_external@4',0x6ae00),('public','@omf_emit_public@8',0x6aef0),('external','@omf_emit_external@4',0x6ad80),('local-external','@omf_emit_local_external@4',0x6adc0),('communal','@omf_emit_communal@4',0x6ae90),('weak','@omf_emit_weak_external@4',0x6acc0)];children={names[n]if native else a:k for k,n,a in specs};events=[];last_result=None
 def hook(m,a,size,_):
  nonlocal last_result
  if a==0x500000:m.emu_stop();return
  if a not in children:return
  k=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);result=0xdeadbeef
  assert struct.unpack('<H',m.mem_read(symbol+34,2))[0]==(c['flags']|1)
  if k=='lookup':
   assert ecx==2;events.append([k,2]);result=segment
   if c['mutation']=='lookup':m.mem_write(symbol+36,struct.pack('<I',0x200 if c['kind']==1 else 0x40))
  elif k=='display':
   assert ecx==symbol;events.append([k]);result=text
   if c['mutation']=='display':m.mem_write(segment+56,struct.pack('<I',99))
  elif k in ['public-name','local-name']:
   assert ecx==text;events.append([k]);result=31 if k=='public-name'else 47;last_result=result
   if c['mutation']=='name':m.mem_write(segment+69,b'\x80')
  else:
   assert ecx==symbol
   if k=='public':assert edx==segment
   if k=='comdat':
    expected=last_result if last_result is not None else c['section'];assert struct.unpack('<I',m.mem_read(segment+56,4))[0]==expected
   events.append([k])
   if k=='external'and c['mutation']=='external':m.mem_write(symbol+36,struct.pack('<I',struct.unpack('<I',m.mem_read(symbol+36,4))[0]^0x8000))
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=2000)
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)
 result=bytes(u.mem_read(symbol,128));after=bytes(u.mem_read(segment,72));assert result[:34]==d[:34]and result[40:]==d[40:]and struct.unpack('<H',result[34:36])[0]==(c['flags']|1 if c['flags']&5==4 else c['flags'])and after[:56]==seg[:56]and after[60:69]==seg[60:69]and after[70:]==seg[70:]and bytes(u.mem_read(text,13))==b'printed_name\0'
 return dict(events=events,symbol_hex=result.hex(),segment_hex=after.hex())
cases=[]
def add(kind,attrs,flags=4,comdat=False,section=0,mutation='none'):cases.append(dict(kind=kind,attrs=attrs,flags=flags,comdat=comdat,section=section,mutation=mutation))
for kind in [1,4,3,-1]:
 for flags in [0,1,5,0x8004]:add(kind,0,flags)
for storage in range(7):
 for role in [0,0x40]:add(1,(storage<<7)|role)
for storage in range(8):
 for weak in [0,0x8000]:
  for mutation in ['none','external']:add(4,(storage<<4)|weak,mutation=mutation)
for kind in [1,4]:
 for attrs in [0x200,0x180,0x50,0x40]:
  for section in [0,17]:
   for mutation in ['none','lookup','display','name']:add(kind,attrs,comdat=True,section=section,mutation=mutation)
rows=[]
for c in cases:
 a=run(original,False,c);b=run(canonical,True,c);assert a==b,(c,a['events'],b['events']);rows.append(dict(inputs=c,result=a))
Path('cases/omf-mark-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole407-byte original/canonical marker; gate/high flags, variable/function/unsupported signed kinds, storage dispatch, COMDAT existing/new public/local name registration and bit4, name assignment after child mutations, cached branch classification versus post-lookup attribute reload, post-external weak-bit reload, full owned128 Symbol/72 Segment/13 name bytes and callee-saved/RET0 checked.126 controls mock every child including actual lookup/name helpers; no child behavior or cold branch real-case execution acceptance. Actual non-COMDAT variable _omf_callback inputs/closed global hash owner observed; no uninitialized descriptor tail dumped or seeded.'),indent=2)+'\n');print(len(rows),'whole OMF symbol marker controls and8 actual ABI rejects pass')
