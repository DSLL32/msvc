"""Whole OMF record lookup controls; original jump-data and actual ABI proof."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
start=time.perf_counter();ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args()
orig,canonical,cfg,_,_=indexes();symbols=publics();name='@omf_find_record@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);actual=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def abi(e):
 assert e['matched'] and e['function']==name and e['no_hidden_inputs_proof'] and e['hidden_inputs']==row['abi']['hidden_inputs']==[] and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:a[k]for k in ['name','location','type']}for a in e['formals']]==row['abi']['parameters']==[dict(name='type',location='ECX',type='unsigned32'),dict(name='context',location='EDX',type='null32')]
 assert [a['value']for a in e['formals']]==e['original_values']==e['native']['values']==[128,0]
 a=e['pool_input']['original'];b=e['pool_input']['native'];assert a['stride']==1004 and a['count']==4 and a['current']==a['base']and b['current']==b['base']and a['headers']==b['headers']==[bytes(20).hex()]*4
abi(actual);reject=[]
for label,mutate in [('missing context',lambda e:e['formals'].pop()),('wrong type register',lambda e:e['formals'][0].update(location='EDX')),('nonnull actual context',lambda e:e['native']['values'].__setitem__(1,1)),('unknown hidden input',lambda e:e.update(hidden_inputs=['?'])),('wrong stride',lambda e:e['pool_input']['original'].update(stride=1000)),('wrong current identity',lambda e:e['pool_input']['native'].update(current=0)),('changed header',lambda e:e['pool_input']['native']['headers'].__setitem__(0,'01'+bytes(19).hex()))]:
 e=copy.deepcopy(actual);mutate(e)
 try:abi(e)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
# Independent mapping of all75 original target DWORDs and exact extent. C uses
# semantic switch cases, not these addresses or an original table copy.
groups={0x6bda0:[0x80,0x88,0x8a,0x8e,0x99,0xa3],0x6bdb0:[0x8c,0x96,0xb0,0xb4,0xca],0x6bdd1:[0x91,0xb7],0x6be0b:[0x95,0xc5],0x6be30:[0x9a],0x6be60:[0x9d],0x6be80:[0xbc],0x6bea0:[0xc3],0x6beba:[0xa1]}
expected=[0x6bd73]*75
for target,types in groups.items():
 for t in types:expected[t-128]=target
assert list(struct.unpack('<75I',orig.read(0x6bf28,300)))==expected
assert row['start']==0x6bd50 and row['end']==0x6bf27 and row['shared']==[[0x6bf28,0x6c054]] and orig.read(0x6bf26,2)==b'\xc3\x90'
assert struct.unpack('<I',orig.read(0x85094,4))[0]==0 and orig.read(0x85090,1)==b'\0'
# Compile-time Segment size check and direct storage member are also verified
# by canonical control execution, including byte69 without a duplicate object.
assert 'omf_comdat.flags' in Path('src/omf-output.c').read_text()
if args.abi_only:
 print('OMF lookup actual ABI/7 rejects,75 dispatch DWORDs,extent and initial data pass');raise SystemExit
MASK=0xffffffff

def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 pool=symbols['_omf_records']if native else 0x7a8dc;current=symbols['_omf_current_record']if native else 0x7b88c
 init=symbols['@omf_initialize_record@12']if native else 0x6c060;pad=symbols['@omf_pad_record@8']if native else 0x6b2a0
 flags=symbols['_omf_comdat_flags']if native else 0x85090;fixups=symbols['_omf_fixup_count']if native else 0x85094
 requested=0x400000;owner=requested if c.get('same',True)else 0x400080
 for a,group in [(requested,0x400200),(0x400080,0x400220)]:
  obj=bytearray(72);struct.pack_into('<I',obj,24,group);struct.pack_into('<I',obj,64,c.get('order',4)if a==requested else 4);struct.pack_into('<I',obj,48,c.get('alignment',4)&MASK);struct.pack_into('<I',obj,4,c.get('position',100)&MASK);obj[69]=c.get('segment_flags',0);u.mem_write(a,bytes(obj))
 u.mem_write(0x400200,struct.pack('<II',c.get('group_name',3),c.get('group_number',2)));u.mem_write(0x400220,struct.pack('<II',3,2))
 before=bytearray((i*17+13)&255 for i in range(4016))
 for i in range(4):
  slot=pool+i*1004;typ=c['type']if i in c['matches']else c['type']^1
  struct.pack_into('<5I',before,i*1004,typ,owner,c.get('offset',80)&MASK,c.get('count',20)&MASK,(slot+998-c.get('remaining',300))&MASK)
 u.mem_write(pool,bytes(before));u.mem_write(current,struct.pack('<I',pool+c['first']*1004));u.mem_write(flags,bytes([c.get('cached_flags',0)]));u.mem_write(fixups,struct.pack('<I',c.get('fixups',0)&MASK));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,c['type']);u.reg_write(UC_X86_REG_EDX,requested)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 events=[]
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a not in(init,pad):return
  pointer=m.reg_read(UC_X86_REG_ECX);index=(pointer-pool)//1004;assert pointer==pool+index*1004 and index in c['matches'];esp=m.reg_read(UC_X86_REG_ESP);edx=m.reg_read(UC_X86_REG_EDX)
  if a==init:
   context=struct.unpack('<I',u.mem_read(esp+4,4))[0];assert edx==c['type']and context==requested;events.append(['initialize',index]);result=pool+((index+1)%4)*1004;u.mem_write(result+12,struct.pack('<I',0x13579bdf));cleanup=8
  else:
   events.append(['pad',index,edx]);result=0x2468ace0;cleanup=4
   # Deliberate bounded mock mutation, not execution of the padding body or
   # evidence for oversized buffer writes on malformed alignment/positions.
   u.mem_write(pointer+12,struct.pack('<I',(c.get('count',20)+edx)&MASK))
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcd0101);m.reg_write(UC_X86_REG_EDX,0xabcd0202);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+cleanup)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(symbols[name]if native else row['start'],0,count=10000)
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804
 assert all(u.reg_read(r)==v for r,v in saved)
 result=u.reg_read(UC_X86_REG_EAX);assert result==0 or pool<=result<pool+4016 and(result-pool)%1004==0
 output=bytearray(u.mem_read(pool,4016))
 for i in range(4):struct.pack_into('<I',output,i*1004+16,(struct.unpack_from('<I',output,i*1004+16)[0]-pool)&MASK)
 return dict(result=None if result==0 else(result-pool)//1004,events=events,pool_sha256=hashlib.sha256(output).hexdigest())
controls=[]
def add(**kw):
 c=dict(type=0x80,matches=[0],first=0);c.update(kw);controls.append(c)
for t,first,slot in itertools.product(list(range(128,203))+[0,127,203,0xffffffff,0x80000000],range(4),range(4)):add(type=t,first=first,matches=[slot])
for t,first in itertools.product([0x80,0x91,0xa1,0xc3,0x7f],range(4)):add(type=t,first=first,matches=[]);add(type=t,first=first,matches=[0,1,2,3])
for types,threshold in [([0x8c,0x96,0xb0,0xb4,0xca],259),([0x91,0xb7],256),([0x95,0xc5],6),([0x9a],25),([0x9d],11),([0xbc],4)]:
 for t,r,same in itertools.product(types,[threshold-1,threshold,threshold+1,-1,-2147483648,2147483647],[True,False]):add(type=t,remaining=r,same=same)
for t,field in itertools.product([0x91,0xb7,0x9a],['group_name','group_number','order']):add(type=t,same=False,**{field:9})
for t,remaining,fixups in itertools.product([0xa1,0xc3],[20,21,24,25,300,-1],[-1,248,249,250]):add(type=t,remaining=remaining,fixups=fixups)
for t,align,pos in itertools.product([0xa1,0xc3],[0,1,2,3,4,16,0xffffffff,0x80000000],[99,100,101,103,104,0xffffffff]):add(type=t,alignment=align,position=pos)
for t,offset,count,pos in itertools.product([0xa1,0xc3],[0xffffffff,0xfffffff0],[1,20],[0,4,0xfffffffe]):add(type=t,offset=offset,count=count,position=pos,alignment=0xffffffff)
for t,seg,cached,same in itertools.product([0xa1,0xc3],[0,2,0xff],[0,2,0xff],[True,False]):add(type=t,segment_flags=seg,cached_flags=cached,same=same)
rows=[]
for c in controls:
 a=run(orig,False,c);b=run(canonical,True,c);assert a==b,(c,a,b);rows.append(dict(inputs=c,result=a))
p=Path('cases/omf-find-validation.json');p.write_text(json.dumps(dict(actual_abi=True,abi_rejected=reject,dispatch_entries=75,extent_bytes=771,states=len(rows),cases=rows,seconds=time.perf_counter()-start,scope='Whole original471-byte lookup plus300-byte jump data vs canonical C. Original/native actual first80/NULL pool entry separately proved; controlled reuse/capacity/context/signed32/unsigned32/wraparound paths use mocked children, not actual cold-case execution. Initializer RET4/output pointer, padding ignored return, pool mutations, all callee-saved registers/ESP checked. Padding mock does not write arbitrary requested ranges or prove oversized writes safe.'),indent=2)+'\n');print(len(rows),'whole original/canonical OMF lookup controls pass;7 actual ABI rejects')
