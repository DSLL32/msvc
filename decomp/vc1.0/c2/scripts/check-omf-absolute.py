"""Actual lookup-owned pointer and whole absolute-reference adapter ABI."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_output_bytes@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
params=[dict(name='displacement',location='ECX',type='unsigned32'),dict(name='symbol',location='EDX',type='SymbolHeader pointer')]
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6b2c0'and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and e['no_hidden_inputs_proof']
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==params
 assert e['shape']==dict(displacement=0xfffffffc,identifier=118,bucket=54,index=0)and e['original']['return_address']==0x6b145 and e['native']['caller']=='omf_output_relocation'
 for side in ['original','native']:
  d=e[side];assert d['symbol']==d['referenced_owner']==d['member']and d['symbol']and d['displacement']==0xfffffffc and d['identifier']==118 and d['bucket']==54 and d['index']==0
  assert [f[side]for f in e['formals']]==[d[k]for k in ['displacement','symbol']]
validate(e);reject=[]
for label,change in [('missing symbol formal',lambda e:e['formals'].pop()),('wrong symbol location',lambda e:e['formals'][1].update(location='entryESP+4')),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('wrong lookup owner',lambda e:e['original'].update(referenced_owner=0)),('wrong global member',lambda e:e['native'].update(member=0)),('wrong identifier',lambda e:e['native'].update(identifier=119)),('wrong displacement',lambda e:e['original'].update(displacement=0)),('invented pointee input',lambda e:e.update(hidden_inputs=['symbol.flags']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b2c0 and row['end']==0x6b2d0 and original.read(0x6b2cf,1)==b'\xc3'and original.read(0x6bcd5,3)==b'\xc2\x08\0'
if args.abi_only:
 print(name,'actual ABI/negative/extent/storage checks pass; full semantic grid is explicit');raise SystemExit

def run(image,native,displacement,symbol):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 u.mem_write(0x300800,struct.pack('<2I',0x500000,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,displacement);u.reg_write(UC_X86_REG_EDX,symbol);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 child=names['@omf_add_fixup@16']if native else 0x6b990;calls=[];terminal=False
 def hook(m,a,size,_):
  nonlocal terminal
  if a==0x500000:terminal=True;m.emu_stop();return
  if a!=child:return
  ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);values=list(struct.unpack('<2I',m.mem_read(esp+4,8)));assert ecx==symbol and edx&255==9 and values==[displacement,0];calls.append([symbol,9,displacement,0]);m.reg_write(UC_X86_REG_EAX,0xdeadbeef);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+12);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=100);assert terminal and len(calls)==1 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x300804,4))==struct.pack('<I',0xfeedbeef)
 return dict(calls=calls)
rows=[]
for displacement,symbol in itertools.product([0,1,0xfffffffc,0x80000000,0xffffffff],[0,0xdeadbeef,0x400080]):
 a=run(original,False,displacement,symbol);b=run(canonical,True,displacement,symbol);assert a==b;rows.append(dict(displacement=displacement,symbol=symbol,result=a))
Path('cases/omf-absolute-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole16-byte original/canonical wrapper, exact displacement/symbol permutation to ECXsymbol/DL9/two stack words(displacement,0), childRET8 and parentRET0/callee-saved/stack guards. Symbol pointers opaque/unmapped in request-shape controls prove no wrapper dereference, not valid child/frontend inputs. Upper EDX bits outside childunsigned8 contract, no fabricated native hidden formal. Actual displacement-4 and closed globallookup member id118/bucket54 observed; child remains named unaccepted trap, no behavior/return/artifact credit.'),indent=2)+'\n');print(len(rows),'whole absolute adapter controls and8 actual ABI rejects pass')
