"""Actual mutable OMF string/record ABI and whole counted-string controls."""
from pathlib import Path
import copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
start=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_append_string@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:a[k]for k in ['name','location','type']}for a in e['formals']]==row['abi']['parameters']==[dict(name='record',location='ECX',type='OmfRecord pointer'),dict(name='text',location='EDX',type='char pointer')]
 a=e['record_input'];b=e['native']['record'];assert e['formals'][0]['original']==a['pointer']==a['current']==a['base']and e['formals'][0]['native']==b['pointer']==b['current']==b['base']
 for r in [a,b]:
  raw=bytes.fromhex(r['hex']);assert len(raw)==23 and struct.unpack('<5I',raw[:20])==(128,0,0,0,r['pointer']+23)and raw[20:]==b'\x80\0\0'
 assert e['formals'][1]['original']==e['original_source_global']and e['formals'][1]['native']==e['native']['source_global']==e['native']['pointer']and e['original_string_hex']==e['native']['hex']==b'omftypes.c\0'.hex()
validate(e);reject=[]
for label,mutate in [('missing text',lambda e:e['formals'].pop()),('wrong register',lambda e:e['formals'][1].update(location='ECX')),('unknown hidden output',lambda e:e.update(hidden_inputs=['?'])),('wrong record identity',lambda e:e['native']['record'].update(current=0)),('wrong cursor',lambda e:e['native']['record'].update(hex=bytes(23).hex())),('wrong source alias',lambda e:e.update(original_source_global=0)),('different source bytes',lambda e:e['native'].update(hex=b'other.c\0'.hex()))]:
 bad=copy.deepcopy(e);mutate(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
def run(image,native,length,count,offset):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 record=0x400000;text=0 if length<0 else 0x402000;source=bytes((i%250)+1 for i in range(max(length,0)))+b'\0'+b'\xa5'*17;u.mem_write(0x402000,source);before=bytearray((i*11+7)&255 for i in range(1004));struct.pack_into('<5I',before,0,128,0,0,count,record+offset);u.mem_write(record,bytes(before));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,record);u.reg_write(UC_X86_REG_EDX,text)
 warning=names['_compiler_warning']if native else 0x70490;append=names['@omf_append_bytes@12']if native else 0x6b650;events=[]
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a not in [warning,append]:return
  esp=m.reg_read(UC_X86_REG_ESP)
  if a==warning:
   assert struct.unpack('<3I',m.mem_read(esp+4,12))==(1,0x312,255);events.append(dict(kind='warning',record_hex=bytes(m.mem_read(record,1004)).hex(),text_hex=bytes(m.mem_read(0x402000,len(source))).hex()));cleanup=4
  else:
   n=struct.unpack('<I',m.mem_read(esp+4,4))[0];assert m.reg_read(UC_X86_REG_ECX)==record and m.reg_read(UC_X86_REG_EDX)==text and n==min(max(length,0),255);events.append(dict(kind='bytes',count=n,record_hex=bytes(m.mem_read(record,1004)).hex(),text_hex=bytes(m.mem_read(0x402000,len(source))).hex()));cursor=struct.unpack('<I',m.mem_read(record+16,4))[0]
   if n:m.mem_write(cursor,bytes(m.mem_read(text,n)))
   old=struct.unpack('<I',m.mem_read(record+12,4))[0];m.mem_write(record+12,struct.pack('<I',(old+n)&0xffffffff));m.mem_write(record+16,struct.pack('<I',cursor+n));cleanup=8
  m.reg_write(UC_X86_REG_EAX,0xabcdef01);m.reg_write(UC_X86_REG_ECX,0x12345678);m.reg_write(UC_X86_REG_EDX,0x23456789);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+cleanup)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=10000);assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804
 n=min(max(length,0),255);expected=bytearray(before);struct.pack_into('<I',expected,12,(count+n+1)&0xffffffff);struct.pack_into('<I',expected,16,record+offset+n+1);expected[offset]=n;expected[offset+1:offset+n+1]=source[:n];expected_source=bytearray(source)
 if length>255:expected_source[255]=0
 assert bytes(u.mem_read(record,1004))==expected and bytes(u.mem_read(0x402000,len(source)))==expected_source and [e['kind']for e in events]==(['warning']if length>255 else[])+['bytes']
 return dict(events=events,result_sha256=hashlib.sha256(expected+expected_source).hexdigest())
rows=[]
for length,count,offset in itertools.product([-1,0,1,10,254,255,256,300],[0,7,0xffffffff],[23,80]):
 a=run(original,False,length,count,offset);b=run(canonical,True,length,count,offset);assert a==b,(length,count,offset,a,b);rows.append(dict(length=length,count=count,cursor_offset=offset,result=a))
Path('cases/omf-string-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-start,scope='Whole96-byte original/canonical string writer: NULL/empty/short/255/oversize mutable strings, warn-before-truncate state, exact255th-byte mutation, same source pointer, counted length byte, count overflow/cursor/caller cleanup and unchanged spare bytes. Warning/byte-writer children mocked; long-string branch is bounded control evidence, actual case only omftypes.c. Full input pointer/source/record/header/3-prefix identity proof is independent.'),indent=2)+'\n');print(len(rows),'whole OMF mutable-string controls and7 actual ABI rejects pass')
