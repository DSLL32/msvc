"""Debug register-location pass: closed input/ABI/storage and full body controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='_record_debug_register_locations';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def shape(d):
 assert len(d['heads'])==96 and[(g,b)for g,b,_ in d['heads']]==[(g,b)for g in range(3)for b in range(32)]
 owners={v['node']:i+1 for i,v in enumerate(d['nodes'])};assert len(owners)==len(d['nodes']);homes={};rows=[]
 for v in d['nodes']:
  assert not v['next']or v['next']in owners
  if v['home']not in homes:homes[v['home']]=len(homes)+1 if v['home']else 0
  rows.append(dict(next=owners.get(v['next'],0),home=homes[v['home']],kind=v['kind'],attributes=v['attributes']))
 effective=[dict(owner=owners[v['owner']],source_flags=v['source_flags'],representative=v['representative'],target=owners[v['target']],flags=v['flags'],cost=v['cost'],conflicts=bool(v['conflicts']))for v in d['effective']]
 for _,_,p in d['heads']:assert not p or p in owners
 return dict(heads=[[g,b,owners.get(p,0)]for g,b,p in d['heads']],nodes=rows,effective=effective)
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x2b1b0'and e['no_hidden_inputs_proof']and e['formals']==row['abi']['parameters']==[]and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert shape(e['original'])==shape(e['native'])==e['shape']and hashlib.sha256(json.dumps(e['shape'],sort_keys=True).encode()).hexdigest()=='4d179fcd5a4fbf3030c8aa33c56750b620448680d534e34bdab3ce3abaa4547f'
 assert len(e['shape']['nodes'])==27 and len(e['shape']['effective'])==2 and e['original']['return_address']==0x2acf3 and e['native']['caller']=='allocate_machine_registers'
validate(e);reject=[]
for label,change in [('invented formal',lambda e:e.update(formals=[dict(name='unused',location='EAX')])),('missing head',lambda e:e['original']['heads'].pop()),('broken next identity',lambda e:e['native']['nodes'][0].update(next=1)),('wrong eligible flags',lambda e:e['native']['effective'][0].update(flags=0)),('missing effective payload',lambda e:e['native'].update(effective=[])),('wrong symbol attributes',lambda e:e['native']['nodes'][16].update(attributes=0)),('invented old output',lambda e:e.update(hidden_inputs=['symbol_field28']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x2b1b0 and row['end']==0x2b249 and original.read(0x2b248,1)==b'\xc3'
assert original.read(0x2acee,5)==b'\xe8'+struct.pack('<i',0x2b1b0-0x2acf3)
for image,base in [(original,0x7c730),(canonical,names['_register_variable_buckets'])]:
 section=next(s for s in image.sections if image.base+s['rva']<=base and base+384<=image.base+s['rva']+s['virtual_size']);assert section['flags']&0x80  # Loader-zero BSS has no raw-file initializer to read.
if args.abi_only:print(name,'actual ABI/7 rejects/153B extent/3x32 typed BSS checks pass; full controls explicit');raise SystemExit

def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x3000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 base=names['_register_variable_buckets']if native else 0x7c730;v0,v1,target=0x400100,0x400200,0x400300;h0,h1=0x401000,0x401100;variables={};symbols={}
 def variable(p,next,home,flags,cost,conflicts):
  b=bytearray((i*19+5)&255 for i in range(52));struct.pack_into('<I',b,0,next);struct.pack_into('<I',b,8,0x402000 if conflicts else 0);struct.pack_into('<h',b,22,cost);struct.pack_into('<2H',b,24,17,17);struct.pack_into('<I',b,28,flags);struct.pack_into('<I',b,44,home);variables[p]=b;u.mem_write(p,bytes(b))
 flags=(0x800 if c['alias']else 0)|c['flags'];variable(v0,v1,0 if c['eligibility']=='null'else h0,flags,c['cost'],c['conflicts']);variable(v1,0,h1,6,1,False);variable(target,0,0,c['flags']^0xa040,c['cost'],c['conflicts']);matrix=bytearray(384);struct.pack_into('<I',matrix,(c['group']*32+31)*4,v0);struct.pack_into('<I',matrix,17*4,target);u.mem_write(base,bytes(matrix))
 for p,kind,attrs in [(h0,4 if c['eligibility']=='kind'else 1,0x12345030 if c['eligibility']=='role'else 0x12345010),(h1,1,0x23456010)]:
  b=bytearray((i*23+7)&255 for i in range(44));b[12]=kind;struct.pack_into('<I',b,36,attrs);symbols[p]=b;u.mem_write(p,bytes(b))
 u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 child=names['@lookup_register_variable@8']if native else 0x2c870;events=[];eligible=c['eligibility']=='yes';effective=target if c['alias']else v0
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a!=child:return
  assert eligible and c['alias']and m.reg_read(UC_X86_REG_ECX)&65535==17 and m.reg_read(UC_X86_REG_EDX)==1;events.append('lookup');esp=m.reg_read(UC_X86_REG_ESP)
  if c['mutation']:
   struct.pack_into('<I',variables[v0],0,0);m.mem_write(v0,bytes(variables[v0]));struct.pack_into('<I',symbols[h0],36,0x44440010);m.mem_write(h0,bytes(symbols[h0]))
  m.reg_write(UC_X86_REG_EAX,target);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=10000)
 for p,var in [(h0,effective)]if eligible and(c['cost']or c['conflicts'])else[]:
  attrs=struct.unpack_from('<I',symbols[p],36)[0];struct.pack_into('<I',symbols[p],36,attrs|0x8000);struct.pack_into('<H',symbols[p],40,struct.unpack_from('<I',variables[var],28)[0]&63)
 attrs=struct.unpack_from('<I',symbols[h1],36)[0];struct.pack_into('<I',symbols[h1],36,attrs|0x8000);struct.pack_into('<H',symbols[h1],40,6)
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(base,384))==matrix and all(bytes(u.mem_read(p,len(b)))==b for p,b in list(variables.items())+list(symbols.items()))and len(events)==int(eligible and c['alias'])
 return dict(events=events,symbols=[b.hex()for b in symbols.values()])
rows=[]
for group,eligibility,alias,cost,conflicts,flags,mutation in itertools.product(range(3),['null','kind','role','yes'],[False,True],[0,-1],[False,True],[0x4001,0x403f],[False,True]):
 c=dict(group=group,eligibility=eligibility,alias=alias,cost=cost,conflicts=conflicts,flags=flags,mutation=mutation);a=run(original,False,c);b=run(canonical,True,c);assert a==b;rows.append(dict(inputs=c,result=a))
Path('cases/debug-register-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole153B original/canonical3x32 bucket walk, null/non-kind1/non-temporary skips, original home/next held across representative lookup, postlookup signed spillcost/conflicts/low6flags, postchild attributeOR8000, exactWORD40 assignment, all52B variables/44B symbols/384B heads/callee-saved/RET0. Mock child can change originalnext/attributes to prove timing; actual readonly child remains separate accepted function. Actual27 nodes/2 eligible noalias observed, coldalias controls do not claim actual case branch coverage. Synthetic old output bytes are overwrite guard only, never actual input dump/seed.'),indent=2)+'\n');print(len(rows),'whole debug-register controls and7 actual ABI rejects pass')
