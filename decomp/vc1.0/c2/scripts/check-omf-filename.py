"""Filename record ownership and whole header-tail adapter ABI."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_output_debug_pair@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['previous_provisional_name']=='_omf_output_debug_pair'and e['original_va']=='0x6b730'and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and e['no_hidden_inputs_proof']
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name='filename',location='ECX',type='char pointer')]
 assert e['shape']==dict(opcode=5,list_index=1,filename_hex=b'omftypes.c\0'.hex())and e['original']['return_address']==0x6856d and e['native']['caller']=='emit_machine_instructions'
 for side in ['original','native']:
  d=e[side];assert d['source']==d['current']==d['listing']==d['record_source']==e['formals'][0][side]and d['source']and d['instruction']==d['member']and d['instruction']and d['opcode']==5 and d['list_index']==1 and d['filename_hex']==b'omftypes.c\0'.hex()
validate(e);reject=[]
for label,change in [('missing filename formal',lambda e:e.update(formals=[])),('wrong register',lambda e:e['formals'][0].update(location='EDX')),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('wrong current owner',lambda e:e['original'].update(current=0)),('wrong listing owner',lambda e:e['native'].update(listing=0)),('wrong record owner',lambda e:e['native'].update(record_source=0)),('wrong closed list member',lambda e:e['original'].update(member=0)),('wrong opcode',lambda e:e['native'].update(opcode=4)),('missing filename bytes',lambda e:e['original'].update(filename_hex='')),('invented return-address formal',lambda e:e.update(hidden_inputs=['return_address']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b730 and row['end']==0x6b735 and original.read(row['start'],1)==b'\xe9'and row['end']+struct.unpack('<i',original.read(row['start']+1,4))[0]==0x6a980
assert 'void __fastcall omf_output_debug_pair(char *filename);'in Path('src/output.h').read_text()and 'output_callbacks.debug_pair = omf_output_debug_pair;'in Path('src/output.c').read_text()
if args.abi_only:print('Filename adapter actual pointer/10 rejects,5-byte tail extent and typed callback binding pass');raise SystemExit

def run(image,native,text,edx):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in[(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x1000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 source=0x400080;u.mem_write(source,text+b'\0');u.mem_write(0x300800,struct.pack('<2I',0x500000,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,source);u.reg_write(UC_X86_REG_EDX,edx);u.reg_write(UC_X86_REG_EAX,0x89abcdef);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 child=names['@omf_write_module_header@4']if native else 0x6a980;events=[];terminal=False
 def hook(m,a,size,_):
  nonlocal terminal
  if a==0x500000:terminal=True;m.emu_stop();return
  if a!=child:return
  assert m.reg_read(UC_X86_REG_ECX)==source and bytes(m.mem_read(source,len(text)+1))==text+b'\0';events.append(['header',text.hex()]);esp=m.reg_read(UC_X86_REG_ESP);m.reg_write(UC_X86_REG_EAX,0xdeadbeef);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=200);assert terminal and events==[['header',text.hex()]]and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x300804,4))==struct.pack('<I',0xfeedbeef)and bytes(u.mem_read(source,len(text)+1))==text+b'\0'
 return dict(events=events,stack_cleanup=0)
rows=[]
for text,edx in itertools.product([b'',b'omftypes.c',b'sub\\unit.c',b'A'*255,b'\x80\xfe.c'],[0,0x89abcdef,0xffffffff]):
 a=run(original,False,text,edx);b=run(canonical,True,text,edx);assert a==b;rows.append(dict(filename_hex=text.hex(),unused_edx=edx,result=a))
Path('cases/omf-filename-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole5-byte original tail JMP to accepted header versus typed C wrapper call.15 source/request-shape states verify exact ECX pointer/unchanged full source+NUL, one logical header edge, RET0/callee-saved/stack guards despite caller-clobbered registers. Header child mocked, does not consume return address; physical tail/call return addresses intentionally not equated and no assembly bridge. Actual omftypes.c source/current/listing/machine record opcode5/list member1 pointer alias and sole formal observed separately. No child output/artifact equivalence inferred by controls; genuine normal case must advance fresh matching frontier.'),indent=2)+'\n');print(len(rows),'whole filename adapter controls and10 actual ABI rejects pass')
