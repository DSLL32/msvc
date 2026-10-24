"""Actual current-function callback ABI and whole conditional public forwarding."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_output_symbol@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['previous_provisional_name']=='_omf_output_symbol'and e['original_va']=='0x6b710'and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and e['no_hidden_inputs_proof']
 assert [{k:f[k]for k in['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name='symbol',location='ECX',type='FunctionSymbol pointer')]
 assert e['shape']==dict(flags=1,payload=dict(segment_id=5))and e['original']['state']['return_address']==0x68541 and e['native']['state']['caller']=='emit_machine_instructions'
 for side in ['original','native']:
  d=e[side];assert d['state']['pointer']==d['state']['current']==e['formals'][0][side]and d['state']['pointer']and d['state']['flags']==1 and d['payload']==dict(segment_id=5)
validate(e);reject=[]
for label,change in [('missing formal',lambda e:e.update(formals=[])),('wrong register',lambda e:e['formals'][0].update(location='EDX')),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('wrong current alias',lambda e:e['original']['state'].update(current=0)),('wrong flags',lambda e:e['native']['state'].update(flags=33)),('wrong segment ID',lambda e:e['native'].update(payload=dict(segment_id=6))),('invented child result',lambda e:e.update(hidden_inputs=['segment']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b710 and row['end']==0x6b72c and original.read(0x6b72b,1)==b'\xc3'and original.read(0x6b713,4)==bytes.fromhex('f6460e20')
assert 'void __fastcall omf_output_symbol(FunctionSymbol *symbol);'in Path('src/output.h').read_text()and'output_callbacks.symbol = omf_output_symbol;'in Path('src/output.c').read_text()
if args.abi_only:print('Public callback actual sole/current symbol/7 rejects,28-byte extent/flag20 and typed callback pass');raise SystemExit

def run(image,native,flags,identifier,mutation):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in[(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x1000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 symbol=0x400080;segment=0x400400;d=bytearray((i*13+5)&255 for i in range(64));struct.pack_into('<H',d,14,flags);struct.pack_into('<I',d,24,identifier);u.mem_write(symbol,bytes(d));u.mem_write(segment,b'RETURNED SEGMENT');u.mem_write(0x300800,struct.pack('<2I',0x500000,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,symbol);u.reg_write(UC_X86_REG_EDX,0xabcdef98);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 lookup=names['@lookup_global_symbol@4']if native else 0x4dce0;public=names['@omf_emit_public@8']if native else 0x6aef0;events=[];terminal=False
 def hook(m,a,size,_):
  nonlocal terminal
  if a==0x500000:terminal=True;m.emu_stop();return
  if a not in[lookup,public]:return
  if a==lookup:
   assert not events and not flags&32 and m.reg_read(UC_X86_REG_ECX)==identifier;events.append(['lookup',identifier])
   if mutation:struct.pack_into('<H',d,14,flags|32);struct.pack_into('<I',d,24,0xdeadbeef);m.mem_write(symbol,bytes(d))
  else:assert len(events)==1 and m.reg_read(UC_X86_REG_ECX)==symbol and m.reg_read(UC_X86_REG_EDX)==segment;events.append(['public'])
  esp=m.reg_read(UC_X86_REG_ESP);m.reg_write(UC_X86_REG_EAX,segment if a==lookup else 0xdeadbeef);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=300)
 assert terminal and events==([]if flags&32 else[['lookup',identifier],['public']])and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x300804,4))==struct.pack('<I',0xfeedbeef)and bytes(u.mem_read(symbol,64))==d and bytes(u.mem_read(segment,16))==b'RETURNED SEGMENT'
 return dict(events=events,symbol_sha256=hashlib.sha256(d).hexdigest())
rows=[]
for flags,identifier,mutation in itertools.product([0,1,0x10,0x20,0x21,0x40,0xff,0xffdf,0xffff],[0,5,0xffffffff],[False,True]):
 a=run(original,False,flags,identifier,mutation);b=run(canonical,True,flags,identifier,mutation);assert a==b;rows.append(dict(flags=flags,identifier=identifier,lookup_mutation=mutation,result=a))
Path('cases/omf-symbol-callback-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole28-byte callback versus conditional forwarding model: bit20 gates lookup/public, other low/high flag combinations preserved; identifiers0/5/UINTMAX, lookup mutation flags/ID proves no re-test/relookup, exact original symbol and returned segment forwarded once. All owned byte guards/RET0/saved registers/stack checked. Children mocked; no cold public body/segment validity claim. Actual current-function pointer/flags1/segmentID5/caller observed separately. No incoming segment result/opaque source tail seed; synthetic unused bytes are guards only.'),indent=2)+'\n');print(len(rows),'whole public symbol callback controls/7 actual ABI rejects pass')
