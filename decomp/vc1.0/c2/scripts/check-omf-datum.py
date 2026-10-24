"""Observed datum ABI and whole integer/raw-floating-width emission controls."""
from pathlib import Path
import copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_write_datum@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
params=[dict(name='record',location='ECX',type='OmfRecord pointer'),dict(name='initializer',location='EDX',type='Initializer pointer')]
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6b2d0'and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and e['no_hidden_inputs_proof']
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==params
 assert e['shape']==dict(slot=0,type=2,width=4,value_hex='03000000',count=40,cursor_offset=68)and e['original']['return_address']==0x6b0a4 and e['native']['caller']=='omf_output_relocation'
 for side in ['original','native']:
  d=e[side];assert d['record']==d['current']==d['record_owner']==d['base']and d['initializer']==d['initializer_owner']and d['count']==40 and d['cursor']==d['record']+68 and d['header_hex']=='02000400'and d['value_hex']=='03000000'
  assert [f[side]for f in e['formals']]==[d[k]for k in ['record','initializer']]
validate(e);reject=[]
for label,change in [('missing initializer',lambda e:e['formals'].pop()),('wrong initializer location',lambda e:e['formals'][1].update(location='entryESP+4')),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('wrong record owner',lambda e:e['original'].update(record_owner=0)),('wrong initializer owner',lambda e:e['native'].update(initializer_owner=0)),('wrong width',lambda e:e['native'].update(header_hex='02000800')),('missing active source',lambda e:e['native'].update(value_hex='')),('wrong cursor',lambda e:e['native'].update(cursor=0)),('invented x87 input',lambda e:e.update(hidden_inputs=['ST0']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b2d0 and row['end']==0x6b3c5 and original.read(0x6b3c4,1)==b'\xc3'
def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x3000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 node=0x400080;record=0x400400;d=bytearray((i*13+5)&255 for i in range(64));struct.pack_into('<2h',d,8,c['type'],c['width']);d[12:24]=bytes.fromhex(c['value_hex']);u.mem_write(node,bytes(d));buf=bytearray((i*29+7)&255 for i in range(1004));struct.pack_into('<2I',buf,12,c['count'],record+c['offset']);u.mem_write(record,bytes(buf));u.mem_write(0x300800,struct.pack('<2I',0x500000,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,record);u.reg_write(UC_X86_REG_EDX,node);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 append=names['@omf_append_bytes@12']if native else 0x6b650;error=names['@compiler_internal_error@8']if native else 0x6fb70;events=[];terminal=None
 def hook(m,a,size,_):
  nonlocal terminal
  if a==0x500000:terminal='return';m.emu_stop();return
  if a==error:
   ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);assert c['width']not in[1,2,4,8,10]and edx==0x242 and bytes(m.mem_read(ecx,len(b'@(#)omf.c:1.10\0')))==b'@(#)omf.c:1.10\0';events.append(['error',edx]);terminal='error';m.emu_stop();return
  if a!=append:return
  ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);n=struct.unpack('<I',m.mem_read(esp+4,4))[0];assert ecx==record and edx==node+12 and n==c['width']and(n in[8,10]or n==4 and c['type']==5);assert bytes(m.mem_read(record,1004))==buf and bytes(m.mem_read(edx,n))==d[12:12+n];events.append(['append',n]);buf[c['offset']:c['offset']+n]=d[12:12+n];struct.pack_into('<2I',buf,12,(c['count']+n)&0xffffffff,record+c['offset']+n);m.mem_write(record,bytes(buf))
  if c['mutate_width']is not None:struct.pack_into('<h',d,10,c['mutate_width']);m.mem_write(node,bytes(d))
  m.reg_write(UC_X86_REG_EAX,0xdeadbeef);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+8);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=1000)
 valid=c['width']in[1,2,4,8,10];copy=c['width']in[8,10]or c['width']==4 and c['type']==5
 if valid:
  assert terminal=='return'and events==([['append',c['width']]]if copy else[])
  if not copy:
   n=c['width'];buf[c['offset']:c['offset']+n]=d[12:12+n];struct.pack_into('<2I',buf,12,(c['count']+n)&0xffffffff,record+c['offset']+n)
  returned=struct.unpack_from('<h',d,10)[0]&0xffffffff;assert u.reg_read(UC_X86_REG_EAX)==returned and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x300804,4))==struct.pack('<I',0xfeedbeef)
 else:assert terminal=='error';returned=None
 assert bytes(u.mem_read(record,1004))==buf and bytes(u.mem_read(node,64))==d
 return dict(terminal=terminal,returned=returned,events=events,record_sha256=hashlib.sha256(buf).hexdigest(),initializer_sha256=hashlib.sha256(d).hexdigest())
values=['000000000000000000000000','ffffffffffffffffffffffff','123456780000a03f8899aabb'];cases=[]
for width,typ,count,offset,value in itertools.product([1,2,4,8,10],[2,5],[0,40,0xffffffff],[20,68,980],values):cases.append(dict(width=width,type=typ,count=count,offset=offset,value_hex=value,mutate_width=None))
for width in[4,8,10]:cases.append(dict(width=width,type=5,count=40,offset=68,value_hex=values[2],mutate_width=-5))
for width in[-1,0,3,6]:cases.append(dict(width=width,type=2,count=40,offset=68,value_hex=values[0],mutate_width=None))
rows=[]
for c in cases:
 a=run(original,False,c);b=run(canonical,True,c);assert a==b;rows.append(dict(inputs=c,result=a))
Path('cases/omf-datum-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole245-byte original/canonical function against independent byte/count/cursor/result model. Width1/2/4 integerLE, raw4 type5/8/10 append ABIs, source patterns/countwrap/cursor guards, post-append current signed-width return reload including-5, genuine diagnostic242 for unknown widths, RET0/callee-saved/stack and full owned objects. Append mocked with bounded defined source and state; fatal stops, no returning-fatal claim or floating numerical conversion. Actual type2/width4/value3/count40/cursor68 and two caller-owned formals separately observed; old destination not ABI input.'),indent=2)+'\n');print(len(rows),'whole datum controls and9 actual ABI rejects pass')
