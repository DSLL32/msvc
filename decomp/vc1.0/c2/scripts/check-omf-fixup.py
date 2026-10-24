"""Actual fixup ABI, contextual layout and whole constructor/state-call controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_add_fixup@16';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
params=[dict(name=n,location=l,type=t)for n,l,t in [('symbol','ECX','SymbolHeader pointer'),('type','DL','unsigned8'),('displacement','entryESP+4','unsigned32'),('relative','entryESP+8','signed32')]]
actual=dict(formals=dict(type=9,displacement=0xfffffffc,relative=0),symbol_owner=dict(identifier=118,bucket=54,index=0),record=dict(slot=0,count=124),dos=1,threads=dict(group_frame=0,bss_target=3,data_target=1,const_target=0,code_target=2),payload=dict(kind=1,offset=40,flags=10624,attributes=8742,storage=4),segment=dict(identifier=2,order=2,bucket=2,index=0))
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6b990'and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==8 and e['no_hidden_inputs_proof']
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==params
 assert e['shape']==actual and e['original']['state']['return_address']==0x6b2cf and e['native']['state']['caller']=='omf_output_bytes'
 for side in ['original','native']:
  d=e[side];st=d['state'];assert st['symbol']==st['owner_symbol']==st['member']and st['symbol']and st['identifier']==118 and st['bucket']==54 and st['index']==0 and st['current']==st['base']and st['count']==124 and st['dos']==1
  assert [f[side]for f in e['formals']]==[st[k]for k in ['symbol','type','displacement','relative']]
  assert d['threads']==actual['threads']and d['payload']==actual['payload']and {k:v for k,v in d['segment'].items()if k!='pointer'}==actual['segment']and d['segment']['pointer']
validate(e);reject=[]
for label,change in [('missing relative',lambda e:e['formals'].pop()),('wrong type width',lambda e:e['formals'][1].update(type='unsigned32')),('wrong displacement location',lambda e:e['formals'][2].update(location='entryESP+8')),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('wrong symbol owner',lambda e:e['original']['state'].update(owner_symbol=0)),('wrong record count',lambda e:e['native']['state'].update(count=0)),('wrong descriptor flags',lambda e:e['native']['payload'].update(flags=0)),('missing segment owner',lambda e:e['original']['segment'].update(pointer=0)),('wrong threads',lambda e:e['native']['threads'].update(bss_target=2)),('invented output-entry input',lambda e:e.update(hidden_inputs=['old_fixup']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b990 and row['end']==0x6bcd8 and original.read(0x6bcd5,3)==b'\xc2\x08\0'
# Independent original stride and bit-zero inversion proof; layout also has
# canonical/native compile-time sizeof/offsetof assertions in omf-output.h.
assert original.read(0x6bd34,10)==bytes.fromhex('8d04498d04c554910700')
assert original.read(0x6b9b9,7)==bytes.fromhex('83f8011bdbf7db')
MASK=0xffffffff
BASE_THREADS=[0x11223344,0x22334455,0x33445566,0x44556677,0x55667788]

def initial(c):
 d=bytearray((i*13+5)&255 for i in range(84));d[12]=c['kind']&255;struct.pack_into('<H',d,14,c['symbol_flags']);struct.pack_into('<I',d,4,0x89abcdef);struct.pack_into('<4I',d,20,0x12345678,23,101,0x5a5a5a5a);struct.pack_into('<I',d,36,(c['storage']<<7)|(c['section']<<4));cur=bytearray((i*17+9)&255 for i in range(84));struct.pack_into('<H',cur,14,c['current_flags']);struct.pack_into('<2I',cur,20,0x87654321,29);fix=bytearray((i*23+7)&255 for i in range(24));struct.pack_into('<I',fix,8,c['old_flags']);return d,cur,fix

def model(c):
 d,cur,f=initial(c);threads=BASE_THREADS[:];count=c['count'];events=[['allocate']];kind=c['kind'];position=0x89abcdef
 if c['mutation']=='allocate':
  count=(count+17)&MASK;position=0x10203040;struct.pack_into('<I',d,4,position);threads[0]=0xa1b2c3d4
 if c['allocate_kind']is not None:kind=c['allocate_kind'];d[12]=kind&255
 def put(off,n,v):struct.pack_into({1:'<B',2:'<H',4:'<I'}[n],f,off,v&((1<<(n*8))-1))
 def flags():return struct.unpack_from('<I',f,8)[0]
 def fail(code):events.append(['internal',code,f.hex()]);return 'internal'
 cached=(c['old_flags']&~1)|int(c['relative']==0)|2;put(4,1,c['type']);put(6,2,count+c['displacement']);put(8,4,cached);put(12,1,threads[0]);put(14,2,0)
 def lookup(identifier):
  nonlocal position
  events.append(['lookup',identifier,f.hex()])
  if c['mutation']=='lookup':
   put(8,4,0xdead8007);position=0x10203040;struct.pack_into('<I',d,4,position)
   for i in range(5):threads[i]=(threads[i]+1)&MASK
  return c['order']
 terminal='return'
 if kind==1:
  storage=2 if c['symbol_flags']&32 else c['storage']
  if storage in[1,4]:
   section=c['section']
   if section not in[2,4,6]:terminal=fail(0x35e)
   else:
    order=lookup(23)
    if section!=6 and c['symbol_flags']&0x200:put(8,4,cached&~4);put(13,1,4);put(16,2,order)
    elif order!={2:2,4:3,6:4}[section]:terminal=fail({2:0x358,4:0x322,6:0x336}[section])
    else:put(8,4,cached|4);put(13,1,threads[{2:2,4:3,6:1}[section]])
   if terminal=='return':put(8,4,flags()&~8);put(20,4,position)
  elif storage in[2,3]:
   if not c['dos']:put(8,4,cached&~2);put(12,1,5)
   put(8,4,(flags()&~4)|8);put(13,1,6);put(16,2,0x12345678)
  else:terminal=fail(0x368)
 elif kind in[3,4]:
  put(8,4,cached|2 if c['dos']else cached&~2);put(12,1,threads[0]if c['dos']else 5)
  if kind==4 or c['current_flags']&32:
   put(8,4,(flags()&~4)|8);put(13,1,6);put(16,2,0x12345678 if kind==4 else 0x87654321)
   if kind==4 and c['section']==2:events.extend([['format',f.hex()],['error',0x81,f.hex()]]);terminal='error'
  elif c['current_flags']&0x200:
   saved=flags()&~4;put(8,4,saved);put(13,1,4);order=lookup(29);put(16,2,order);put(8,4,saved&~8);put(20,4,position)
  else:put(8,4,(flags()|4)&~8);put(13,1,threads[4]);put(20,4,position)
 else:terminal=fail(0x3a5)
 return dict(terminal=terminal,events=events,fixup=f,symbol=d,current_symbol=cur,threads=threads,count=count)

if args.abi_only:
 print(name,'actual ABI/negative/extent/storage checks pass; full semantic grid is explicit');raise SystemExit

def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x5000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 symbol=0x400080;seg=0x400200;current_symbol=0x400300;fixup=0x400400;record=0x401000;record2=0x401800;string=0x402000;d,cur,f=initial(c);u.mem_write(symbol,bytes(d));u.mem_write(current_symbol,bytes(cur));u.mem_write(fixup,bytes(f));segment=bytearray((i*11+1)&255 for i in range(72));struct.pack_into('<I',segment,64,c['order']);u.mem_write(seg,bytes(segment));u.mem_write(string,b'fixture_name\0');u.mem_write(record+12,struct.pack('<I',c['count']));u.mem_write(record2+12,struct.pack('<I',(c['count']+17)&MASK));fields={'record':('_omf_current_record',0x7b88c),'threads':('_omf_fixup_threads',0x7a8c4),'dos':('_dos_object',0x80684),'current':('_current_symbol',0x831ac)};globals_={k:names[n]if native else a for k,(n,a)in fields.items()};u.mem_write(globals_['record'],struct.pack('<I',record));u.mem_write(globals_['threads'],struct.pack('<5I',*BASE_THREADS));u.mem_write(globals_['dos'],struct.pack('<I',c['dos']));u.mem_write(globals_['current'],struct.pack('<I',current_symbol));u.mem_write(0x300800,struct.pack('<4I',0x500000,c['displacement'],c['relative']&MASK,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,symbol);u.reg_write(UC_X86_REG_EDX,0xabc00000|(c['type']&255));saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 specs=[('allocate','_omf_allocate_fixup',0x6bd10),('lookup','@lookup_global_symbol@4',0x4dce0),('format','@format_function_name@4',0x4df70),('internal','@compiler_internal_error@8',0x6fb70),('error','_compiler_error',0x70210)];children={names[n]if native else a:k for k,n,a in specs};events=[];terminal=None
 def hook(m,a,size,_):
  nonlocal terminal
  if a==0x500000:terminal='return';m.emu_stop();return
  if a not in children:return
  k=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);result=0xdeadbeef;pop=4
  if k=='allocate':
   events.append([k]);result=fixup
   if c['mutation']=='allocate':m.mem_write(globals_['record'],struct.pack('<I',record2));struct.pack_into('<I',d,4,0x10203040);m.mem_write(symbol,bytes(d));m.mem_write(globals_['threads'],struct.pack('<I',0xa1b2c3d4))
   if c['allocate_kind']is not None:d[12]=c['allocate_kind']&255;m.mem_write(symbol,bytes(d))
  elif k=='lookup':
   assert ecx in[23,29];events.append([k,ecx,bytes(m.mem_read(fixup,24)).hex()]);result=seg
   if c['mutation']=='lookup':
    m.mem_write(fixup+8,struct.pack('<I',0xdead8007));struct.pack_into('<I',d,4,0x10203040);m.mem_write(symbol,bytes(d));m.mem_write(globals_['threads'],struct.pack('<5I',*[(v+1)&MASK for v in BASE_THREADS]))
  elif k=='format':assert ecx==symbol;events.append([k,bytes(m.mem_read(fixup,24)).hex()]);result=string
  elif k=='internal':assert bytes(m.mem_read(ecx,15))==b'@(#)omf.c:1.10\0';events.append([k,edx,bytes(m.mem_read(fixup,24)).hex()]);terminal=k;m.emu_stop();return
  else:
   code,arg=struct.unpack('<2I',m.mem_read(esp+4,8));assert code==0x81 and arg==string;events.append([k,code,bytes(m.mem_read(fixup,24)).hex()]);terminal=k;m.emu_stop();return
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+pop);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=3000);want=model(c);assert terminal==want['terminal']and events==want['events'],(c,terminal,events,want['events']);assert bytes(u.mem_read(fixup,24))==want['fixup'],(c,bytes(u.mem_read(fixup,24)).hex(),want['fixup'].hex());assert bytes(u.mem_read(symbol,84))==want['symbol']and bytes(u.mem_read(current_symbol,84))==want['current_symbol']and bytes(u.mem_read(seg,72))==segment;assert bytes(u.mem_read(globals_['threads'],20))==struct.pack('<5I',*want['threads'])and struct.unpack('<I',u.mem_read(globals_['record'],4))[0]==(record2 if c['mutation']=='allocate'else record)and bytes(u.mem_read(globals_['dos'],4))==struct.pack('<I',c['dos'])and bytes(u.mem_read(globals_['current'],4))==struct.pack('<I',current_symbol)
 if terminal=='return':assert u.reg_read(UC_X86_REG_ESP)==0x30080c and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x30080c,4))==struct.pack('<I',0xfeedbeef)
 return dict(terminal=terminal,events=events,fixup_hex=want['fixup'].hex())
def case(**kw):
 c=dict(kind=1,storage=4,section=2,symbol_flags=0,current_flags=0,dos=1,old_flags=0xa5a5a5a5,type=9,count=124,displacement=0xfffffffc,relative=0,order=2,mutation='none',allocate_kind=None);c.update(kw);return c
cases=[]
for storage,section,comdat,dos,relative,old in itertools.product([1,4],[2,4,6],[0,0x200],[0,1],[0,1,-1],[0,0xa5a5a5a5,0xffffffff]):cases.append(case(storage=storage,section=section,symbol_flags=comdat,dos=dos,relative=relative,old_flags=old,order=0x12345 if comdat and section!=6 else{2:2,4:3,6:4}[section]))
for storage,dos,relative in itertools.product([2,3],[0,1],[0,1,-1]):cases.append(case(storage=storage,dos=dos,relative=relative))
for flags,dos,relative in itertools.product([0,0x20,0x200,0x220],[0,1],[0,1,-1]):cases.append(case(kind=3,current_flags=flags,dos=dos,relative=relative,order=0x12345))
for section,dos,relative in itertools.product([0,2,5],[0,1],[0,1,-1]):cases.append(case(kind=4,section=section,dos=dos,relative=relative))
for kind in[0,2,5,-1]:cases.append(case(kind=kind))
for storage in[0,5,15]:cases.append(case(storage=storage))
for section in[0,1,7]:cases.append(case(section=section))
for section in[2,4,6]:cases.append(case(section=section,order=9))
for section in[2,4,6]:cases.append(case(section=section,order={2:2,4:3,6:4}[section],mutation='lookup'));cases.append(case(section=section,symbol_flags=0x200,order={2:2,4:3,6:4}[section],mutation='lookup'))
cases.extend([case(kind=3,current_flags=0x200,mutation='lookup'),case(symbol_flags=0x20,storage=4),case(mutation='allocate'),case(kind=1,allocate_kind=4,section=5),case(kind=4,allocate_kind=1),case(count=0xffffffff,displacement=1,type=0xfe),case(count=0,displacement=0xffffffff,type=0)])
rows=[]
for c in cases:
 a=run(original,False,c);b=run(canonical,True,c);assert a==b;rows.append(dict(inputs=c,result=a))
Path('cases/omf-fixup-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole840-byte original/canonical constructor against independent24-byte field/guard and child-event model. Variable storage1/4 section2/4/6 predefined/COMDAT, storage2/3 and forcedflags20, label/current-function external/COMDAT/plain, kind4 format/error path, genuine invalid-kind/storage/section/order diagnostics. Relative0 sets bit0, all high old flags/reserved bytes preserved; unsigned8 codes/word index/location truncation/countwrap, allocator post-return descriptor/current-record/thread reload and lookup flag-cache/addend/thread mutations checked. Exact child call state/ABI and RET8/callee-saved/owned descriptor/globals checked on returning cases; real fatal/error entries stop. Allocator/lookup/name children mocked; no actual old fixup output seeded as incoming ABI, no callee behavior/artifact equivalence inferred.33e/315 assertion branches statically present but unreachable because flags20 forces storage2 before section dispatch; not claimed dynamically exercised. Actual kind1/storage4/section2 descriptor inputs independently observed, fresh original/native frontier still required.'),indent=2)+'\n');print(len(rows),'whole fixup constructor controls and10 actual ABI rejects pass')
