"""S_REGISTER actual lexical ABI and packed record/child ordering controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@write_codeview_register@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
expected=dict(record_index=0,kind=1,flags=256,type=116,size=4,register_number=1,debug_offset=109,name_hex=b'_value\0'.hex())
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x69880'and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name='symbol',location='ECX',type='SymbolHeader pointer')]and e['shape']==expected
 for side in ['original','native']:
  d=e[side];assert d['symbol']==d['caller_symbol']==d['record_symbol']==e['formals'][0][side]and d['symbol']and d['scope']and d['record']and d['debug']and all(d[k]==v for k,v in expected.items())
 assert e['original']['return_address']==0x69a46 and e['native']['caller']=='write_debug_symbol'and e['original']['select']==0x6b6a0 and e['original']['data']==0x6b6c0 and e['native']['select']==e['native']['select_owner']and e['native']['data']==e['native']['data_owner']
validate(e);reject=[]
for label,change in [('missing formal',lambda e:e.update(formals=[])),('wrong register',lambda e:e['formals'][0].update(location='EDX')),('broken lexical identity',lambda e:e['native'].update(record_symbol=0)),('wrong typeWORD',lambda e:e['native'].update(type=0)),('wrong name',lambda e:e['native'].update(name_hex='')),('wrong childsize',lambda e:e['native'].update(size=0)),('wrong debug offset',lambda e:e['original'].update(debug_offset=0)),('wrong callback',lambda e:e['original'].update(data=0)),('invented stack record',lambda e:e.update(hidden_inputs=['record']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x69880 and row['end']==0x698fc and original.read(0x698fb,1)==b'\xc3'
s=Path('src/codeview-output.c').read_text();assert all(x in s for x in ['sizeof(CodeViewRegisterRecord) == 308','offsetof(CodeViewRegisterRecord, type) == 4','offsetof(CodeViewRegisterRecord, register_number) == 6','offsetof(CodeViewRegisterRecord, name) == 8'])
if args.abi_only:print(name,'actual lexical ABI/9 rejects/124B extent/packed308B layout pass; full controls explicit');raise SystemExit

def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 symbol,debug,other,textp=0x200000,0x201000,0x201100,0x202000;global_debug=names['_debug_symbols_segment']if native else 0x7914c;callbacks=names['_output_callbacks']if native else 0x7b8f0
 def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
 def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
 d=bytearray((i*19+5)&255 for i in range(44));struct.pack_into('<H',d,32,c['type']);u.mem_write(symbol,bytes(d));text=bytes((97+i%26)for i in range(c['length']));u.mem_write(textp,text+b'\0');put(global_debug,debug);put(debug+4,100);put(other+4,200);put(callbacks+40,0x400100);put(callbacks+36,0x400200);u.mem_write(0x300000,b'\xa5'*0x10000);put(0x300800,0x400000);put(0x300804,0xfeedbeef);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,symbol)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 children={names[n]if native else a:k for k,n,a in [('format','@format_function_name@4',0x4df70),('register','@codeview_register_number@4',0x69900),('copy','@copy_codeview_string@8',0x69520)]};children.update({0x400100:'select',0x400200:'data'});events=[];output=None;post_type=c['type']
 def hook(m,a,size,_):
  nonlocal output,post_type,text
  if a==0x400000:m.emu_stop();return
  if a not in children:return
  k=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);result=0xdeadbeef;events.append(k)
  if k=='format':
   assert ecx==symbol;result=textp
   if c['mutation']=='format':post_type=0x1234;struct.pack_into('<H',d,32,post_type);m.mem_write(symbol+32,bytes(d[32:34]))
  elif k=='select':
   assert ecx==debug
   if c['mutation']=='select':post_type=0x4321;struct.pack_into('<H',d,32,post_type);m.mem_write(symbol+32,bytes(d[32:34]));put(global_debug,other)
  elif k=='register':
   assert ecx==symbol;result=c['register']
   if c['mutation']=='register':struct.pack_into('<H',d,32,0x9876);m.mem_write(symbol+32,bytes(d[32:34]))
  elif k=='copy':
   assert edx==textp;m.mem_write(ecx,bytes([c['length']])+text);result=c['length']+1
   if c['mutation']=='copy':put(global_debug,other)
  else:
   assert edx==c['length']+9;output=bytes(m.mem_read(ecx,edx));expected=struct.pack('<4HB',c['length']+7,2,post_type,c['register']&65535,c['length'])+text;assert output==expected
   if c['mutation']=='data':put(global_debug,other);put(other+4,300)
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,word(esp))
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=10000);use_other=c['mutation']in['select','copy','data'];n=c['length']+9
 assert events==['format','select','register','copy','data']and u.reg_read(UC_X86_REG_EIP)==0x400000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and word(0x300804)==0xfeedbeef and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(symbol,44))==d and word(global_debug)==(other if use_other else debug)and word(debug+4)==100+(0 if use_other else n)and word(other+4)==(300 if c['mutation']=='data'else 200)+(n if use_other else 0)and bytes(u.mem_read(textp,c['length']+1))==text+b'\0'
 return dict(events=events,record_hex=output.hex(),offsets=[word(debug+4),word(other+4)])
rows=[]
for length,type,register,mutation in itertools.product([0,5,255],[0,116,65535],[18,0xabcdffff],['none','format','select','register','copy','data']):
 c=dict(length=length,type=type,register=register,mutation=mutation);a=run(original,False,c);b=run(canonical,True,c);assert a==b;rows.append(dict(inputs=c,result=a))
Path('cases/codeview-register-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole124B original/canonical S_REGISTER writer versus packed8B+countedname model.108 states length0/5/255,typeWORD,AX-only registerreturn, exactformat/select/register/copy/data order, postselecttype/pre-registertype timing, late globaldebugpointer/offset reload afterdata, owned44B symbol/name/stackguard/callee-saved/RET0 checked. All5 children mocked; actual lexical _value/type116/size4/reg1/debugoffset109 observed. Names>255 original pretruncation length issue remains unexercised/unmodified, no new guard or normalization.308B typed localrecord not entry input/seed; only emitted initialized span read.'),indent=2)+'\n');print(len(rows),'whole CodeView register controls/9 actual ABI rejects pass')
