"""Actual directive ABI, contextual dispatch map and whole callback controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument("--abi-only",action="store_true");args=ap.parse_args()
start=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_output_record@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:a[k]for k in ['name','location','type']}for a in e['formals']]==row['abi']['parameters']==[dict(name='directive',location='ECX',type='ModuleDirective pointer')]
 for side in ['original','native']:
  d=e[side];assert e['formals'][0][side]==d['pointer']==d['head']and d['pointer']not in[0,d['next'],d['text']]and d['next']and d['text']and d['kind']==6 and struct.unpack('<3I',bytes.fromhex(d['raw_hex']))==(d['next'],6,d['text'])and bytes.fromhex(d['text_hex'])==b'OMF typed debug case\0'
validate(e);rejected=[]
for label,change in [('missing pointer',lambda e:e.update(formals=[])),('wrong register',lambda e:e['formals'][0].update(location='EDX')),('hidden output',lambda e:e.update(hidden_inputs=['?'])),('lost head identity',lambda e:e['native'].update(head=0)),('wrong directive kind',lambda e:e['native'].update(kind=8)),('missing text input',lambda e:e['native'].update(text_hex='')),('wrong descriptor',lambda e:e['original'].update(raw_hex=bytes(12).hex()))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):rejected.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b860 and row['end']==0x6b912 and row['shared']==[[0x6b914,0x6b940]]and original.read(0x6b911,1)==b'\xc3'
assert struct.unpack('<11I',original.read(0x6b914,44))==(0x6b890,0x6b8a0,0x6b8a7,0x6b8b0,0x6b8b7,0x6b8c0,0x6b8c7,0x6b8e7,0x6b8f0,0x6b8f7,0x6b900)
assert original.string(0x852b8)==canonical.string(names['_compiler_identification'])=='@(#) Microsoft C/C++ 32 bits x86 Compiler Version 8.00.XXXX'
if args.abi_only:print("OMF directive actual ABI/7 rejects,11 switch targets and shared identifier identity pass");raise SystemExit
classes={1:0xdb,2:0xdc,3:0xa4,4:0x9f,5:0xdd,6:0xdf,8:0,9:0x9d,10:0xa0,11:0xa1}
def run(image,native,kind,length,mutate,null):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 directive=0x400000;text=0 if null else 0x400100;second=0x400300;record=0x400500;identification=names['_compiler_identification']if native else 0x852b8;before=bytearray(struct.pack('<3I',0x400800,kind,text));u.mem_write(directive,bytes(before));u.mem_write(0x400100,bytes([length])+bytes(range(1,129))+b'\0');u.mem_write(second,bytes([length])+bytes(range(128,0,-1))+b'\0');u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,directive)
 children={names[n]if native else a:k for k,n,a in [('begin','@omf_begin_record@8',0x6b480),('bytes','@omf_append_bytes@12',0x6b650),('comment','@omf_write_comment@8',0x6b940),('error','@compiler_internal_error@8',0x6fb70)]};events=[];terminal=None
 def hook(m,a,size,_):
  nonlocal terminal
  if a==0x500000:terminal='return';m.emu_stop();return
  if a not in children:return
  k=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);cleanup=4;result=0xdeadbeef
  if k=='error':
   raw=b''
   for i in range(64):
    c=bytes(m.mem_read(ecx+i,1));raw+=c
    if c==b'\0':break
   assert edx==0x294 and raw==b'@(#)omf.c:1.10\0';events.append(['error',edx]);terminal='internal-error';m.emu_stop();return
  if k=='begin':
   assert kind==7 and ecx==0x88 and edx==0;events.append(['begin',ecx,edx]);result=record
   if mutate:u.mem_write(directive+8,struct.pack('<I',second));before[8:12]=struct.pack('<I',second)
  elif k=='bytes':
   count=struct.unpack('<I',m.mem_read(esp+4,4))[0];assert ecx==record and edx==(second if mutate else text)+1;events.append(['bytes',count,edx]);cleanup=8
  else:
   assert ecx&255==classes[kind]and edx==(identification if kind==1 else text);events.append(['comment',ecx&255,'compiler_identification'if kind==1 else edx])
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0x12345678);m.reg_write(UC_X86_REG_EDX,0x23456789);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+cleanup)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=10000)
 if 1<=kind<=11:
  assert terminal=='return'and u.reg_read(UC_X86_REG_ESP)==0x300804
  if kind==1:before[8:12]=struct.pack('<I',identification)
  if kind==7:assert events==[['begin',136,0],['bytes',(length if length<128 else length-256)&0xffffffff,(second if mutate else text)+1]]
 else:assert terminal=='internal-error'and events==[['error',0x294]]
 assert bytes(u.mem_read(directive,12))==before
 if kind==1:before[8:12]=bytes(4)
 return dict(terminal=terminal,events=events,node_hex=before.hex())
rows=[]
for kind,length,mutate,null in itertools.product(list(range(1,12))+[0,12,0xffffffff,0x80000000],[0,1,127,128,255],[False,True],[False,True]):
 if kind==7 and null:continue
 a=run(original,False,kind,length,mutate,null);b=run(canonical,True,kind,length,mutate,null);assert a==b,(kind,length,mutate,null,a,b);rows.append(dict(kind=kind,prefix_byte=length,begin_changes_text=mutate,null_text=null,result=a))
Path('cases/omf-directive-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,states=len(rows),cases=rows,seconds=time.perf_counter()-start,scope='Whole178-byte original/canonical dispatcher+44-byte switch table: all11 kinds and4 unsupported ids, exact shared compiler identification address mutation, unchanged node fields, proper low8 comment classes, begin-before-text-load and sign-extended counted byte forwarded in stack. Children mocked; negative counted lengths prove request shape only, not actual huge byte copy. Unsupported ids stop at genuine294 diagnostic with no fabricated default class or error return. Actual kind6/head-owned20-char user text and12-byte input descriptor observed separately.'),indent=2)+'\n');print(len(rows),'whole OMF directive dispatch controls and7 ABI rejects pass')
