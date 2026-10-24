"""Actual low-byte class/string ABI and complete COMENT prefix controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument("--abi-only",action="store_true");args=ap.parse_args()
start=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_write_comment@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:a[k]for k in ['name','location','type']}for a in e['formals']]==row['abi']['parameters']==[dict(name='kind',location='ECX',type='unsigned8'),dict(name='text',location='EDX',type='char pointer')]
 assert e['formals'][0]['original']==e['formals'][0]['native']==e['native']['comment_kind']==e['comment_input']['kind']==(e['original_kind_raw']&255)==223
 assert e['formals'][1]['original']==e['comment_input']['owner']and e['formals'][1]['native']==e['native']['comment_owner']==e['native']['pointer']and e['original_string_hex']==e['native']['hex']==b'OMF typed debug case\0'.hex()
validate(e);reject=[]
for label,change in [('missing class',lambda e:e['formals'].pop(0)),('wrong class width',lambda e:e['formals'][0].update(type='unsigned32')),('wrong pointer register',lambda e:e['formals'][1].update(location='ECX')),('unknown hidden pointer',lambda e:e.update(hidden_inputs=['?'])),('wrong class',lambda e:e['native'].update(comment_kind=0)),('lost owner',lambda e:e['native'].update(comment_owner=0)),('missing consumed bytes',lambda e:e['native'].update(hex=''))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b940 and row['end']==0x6b98e and original.read(0x6b98d,1)==b'\xc3'
if args.abi_only:print('OMF COMENT actual byte-class/string ABI,7 rejects and78-byte extent pass');raise SystemExit
def run(image,native,kind,length,count,offset,alias=False):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 record=0x400000;text=record+offset if alias else 0x402000;source=bytes((i%250)+1 for i in range(length))+b'\0'+b'\xa5'*17;u.mem_write(0x402000,source);before=bytearray((i*17+11)&255 for i in range(1004));struct.pack_into('<5I',before,0,136,0,0,count,record+offset)
 if alias:before[offset:offset+len(source)]=source
 u.mem_write(record,bytes(before));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,kind);u.reg_write(UC_X86_REG_EDX,text);begin=names['@omf_begin_record@8']if native else 0x6b480;append=names['@omf_append_bytes@12']if native else 0x6b650;events=[]
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a not in[begin,append]:return
  esp=m.reg_read(UC_X86_REG_ESP)
  if a==begin:
   assert m.reg_read(UC_X86_REG_ECX)==136 and m.reg_read(UC_X86_REG_EDX)==0;events.append(['begin',136,0]);result=record;cleanup=4
  else:
   n=struct.unpack('<I',m.mem_read(esp+4,4))[0];assert m.reg_read(UC_X86_REG_ECX)==record and m.reg_read(UC_X86_REG_EDX)==text and n==(0 if alias else length);cursor=struct.unpack('<I',m.mem_read(record+16,4))[0];actual_count=struct.unpack('<I',m.mem_read(record+12,4))[0];assert cursor==record+offset+2 and actual_count==(count+2)&0xffffffff and bytes(m.mem_read(record+offset,2))==bytes([0,kind&255]);events.append(['bytes',n,actual_count,cursor-record]);cleanup=8;result=0xdeadbeef
   if n:m.mem_write(cursor,bytes(m.mem_read(text,n)))
   m.mem_write(record+12,struct.pack('<I',(actual_count+n)&0xffffffff));m.mem_write(record+16,struct.pack('<I',cursor+n))
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+cleanup)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=20000);assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)
 expected=bytearray(before);n=0 if alias else length;expected[offset:offset+2]=bytes([0,kind&255]);expected[offset+2:offset+2+n]=source[:n];struct.pack_into('<I',expected,12,(count+n+2)&0xffffffff);struct.pack_into('<I',expected,16,record+offset+2+n);assert bytes(u.mem_read(record,1004))==expected and bytes(u.mem_read(0x402000,len(source)))==source and len(events)==2
 return dict(events=events,record_sha256=hashlib.sha256(expected).hexdigest())
rows=[]
for kind,length,count,offset in itertools.product([0,1,0xdf,0x100,0x123456df,0xffffffff],[0,1,20,255],[0,7,0xffffffff],[23,80]):
 a=run(original,False,kind,length,count,offset);b=run(canonical,True,kind,length,count,offset);assert a==b;rows.append(dict(raw_ecx=kind,length=length,initial_count=count,cursor_offset=offset,result=a))
for kind,count in itertools.product([0,0xdf,0x123456df],[0,7,0xffffffff]):
 a=run(original,False,kind,20,count,23,True);b=run(canonical,True,kind,20,count,23,True);assert a==b;rows.append(dict(raw_ecx=kind,initial_count=count,text_aliases_prefix=True,result=a))
Path('cases/omf-comment-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-start,scope='Whole78-byte original/canonical COMENT wrapper: lowCL class with high24 bits poisoned, empty/1/20/255 strings, count wrap/cursor offset, begin first and its return pointer,0/class prefix before strlen including source-alias overwrite, exact byte helper pointer/count, all spare bytes and callee-save/ESP preserved. Children mocked; actual classDF/head-owned20-char user string observed independently, no other classes claimed executed.'),indent=2)+'\n');print(len(rows),'whole OMF COMENT controls and7 ABI rejects pass')
