"""Frame-register allowance: owned input ABI, stack writes and local homes."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,re,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
from frame_allow_inputs import projection
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@function_allows_frame_register@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(d):
 assert d['matched']and d['function']==name and d['no_hidden_inputs_proof']and d['hidden_inputs']==row['abi']['hidden_inputs']==[]and d['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and d['return_contract']=='full EAX boolean0/1'
 assert [{k:f[k]for k in ['name','location','type']}for f in d['formals']]==row['abi']['parameters']==[dict(name='function',location='ECX',type='Function pointer')]
 for side in ['original','native']:
  v=d[side];assert v['function']==v['current']==d['formals'][0][side]and v['function']and v['first_work']==v['works'][0]['pointer']and len(v['works'])==5 and sum(bool(w['flags']&2)for w in v['works'])==2
 assert d['original']['return_address']==0x2bf12 and d['native']['caller']=='frame_register_can_be_allocated'
 assert projection(d['original'])==projection(d['native'])==d['projection']
validate(e);rejected=[]
for label,edit in [('missing formal',lambda d:d['formals'].clear()),('wrong register',lambda d:d['formals'][0].update(location='EDX')),('lost current function',lambda d:d['original'].update(current=0)),('invented work root',lambda d:d['native'].update(first_work=0)),('broken work closure',lambda d:d['original']['works'][0].update(next=0)),('changed active storage',lambda d:d['native']['works'][1]['instructions'][2]['operands'][0].update(storage=0)),('unknown hidden operand',lambda d:d.update(hidden_inputs=['?'])),('wrong cleanup',lambda d:d.update(callee_stack_cleanup=4))]:
 d=copy.deepcopy(e);edit(d)
 try:validate(d)
 except(AssertionError,IndexError,KeyError):rejected.append(label)
 else:raise AssertionError('accepted '+label)
assert row['start']==0x2f500 and row['end']==0x2f681 and original.read(0x2f680,1)==b'\xc3'
ops=re.findall(r'^OP\(([^,]+),',Path('src/machine-opcodes.def').read_text(),re.M);counts=list(struct.unpack('<231b',original.read(0x81560,231)));props=[bool(struct.unpack('<H',original.read(0x82210+2*i,2))[0]&0x40)for i in range(231)];assert len(ops)==231 and ops[0xbb]=='DATA'and set(counts)=={0,1,2}
assert list(struct.unpack('<231b',canonical.read(names['_machine_operand_counts'],231)))==counts
normalized=canonical.read(names['_machine_selection_properties'],231);assert all(bool(normalized[i]&1)==props[i]for i in range(231))
assert original.read(0x2f579,7)==bytes.fromhex('6685a810220800')
if args.abi_only:print('Frame allowance sole Function/owned5-work ABI,385B extent,231 implicit/count identities and8 rejects pass');raise SystemExit

def operand(kind=1,reg=0,base=0,index=0,symbol=None,poison=0):return dict(kind=kind,reg=reg,base=base,index=index,symbol=symbol,poison=poison)
def instruction(op='MOV',a=None,b=None,kind=0,writes=0,implicit=0,mutation=None):return dict(op=ops.index(op),kind=kind,operands=[a or operand(),b or operand(2)],writes=writes,implicit=implicit,mutation=mutation)
def run(image,side,blocks,expected_result):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x20000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 memory=bytearray((i*13+7)&255 for i in range(0x20000));function=0x400000;workbase=0x400100;blockbase=0x401000;instbase=0x402000;addrbase=0x408000;symbase=0x40c000
 def put(address,value,fmt='<I'):struct.pack_into(fmt,memory,address-0x400000,value)
 put(function+12,workbase if blocks else 0);records={};operand_records={};block_work={}
 for wi,(flags,instructions)in enumerate(blocks):
  work=workbase+wi*128;block=blockbase+wi*64;put(work,work+128 if wi+1<len(blocks)else 0);put(work+8,block);put(block+28,flags,'<H');put(block,instbase+wi*0x400 if instructions else 0);block_work[wi]=work
  for ii,spec in enumerate(instructions):
   address=instbase+wi*0x400+ii*32;records[address]=(wi,ii,spec);put(address,address+32 if ii+1<len(instructions)else 0);put(address+8,(spec['op']<<8)|spec['kind'])
   for oi,op in enumerate(spec['operands']):
    at=address+16+oi*8;ind=(wi*32+ii)*2+oi;ea=addrbase+ind*16;sy=symbase+ind*64;operand_records[at]=(address,oi,op);put(at,op['poison']|op['kind'])
    if op['kind']==1:put(at+4,op['reg'])
    elif op['kind']in [4,5]:
     put(at+4,ea);put(ea+8,op['base']);put(ea+12,op['index']);put(ea+4,sy if op['symbol']else 0)
     if op['symbol']:put(sy+12,op['symbol'][0],'<B');put(sy+36,op['symbol'][1])
    else:put(at+4,0xdeadbeef)
 u.mem_write(0x400000,bytes(memory));expected=bytearray(memory);u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,function);u.reg_write(UC_X86_REG_EDX,0xdeadbeef)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 children={names[n]if side else a:k for k,n,a in [('writes','@writes_first_operand@4',0x229d0),('implicit','@implicit_register_mask@8',0x22fc0),('mentions','@operand_mentions_register@8',0x21fd0)]};events=[];terminal=[]
 def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
 def write(a,v):u.mem_write(a,struct.pack('<I',v));struct.pack_into('<I',expected,a-0x400000,v)
 def mentioned(at):
  kind=word(at)&15
  if kind==1:return word(at+4)&65535==5
  if kind not in [4,5]:return False
  address=word(at+4)
  return (kind==5 and word(address+12)&65535==5)or word(address+8)&65535==5
 def hook(m,a,size,_):
  if a==0x500000:terminal.append('return');m.emu_stop();return
  if a not in children:return
  child=children[a];esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX)
  if child=='mentions':
   assert edx&65535==5 and ecx in operand_records;address,oi,_=operand_records[ecx];wi,ii,spec=records[address];value=int(mentioned(ecx));events.append([child,wi,ii,oi,5])
  else:
   assert ecx in records;address=ecx;wi,ii,spec=records[address];value=spec[child];oi=-1;events.append([child,wi,ii])
   if child=='implicit':assert edx==2
   else:assert counts[(word(address+8)>>8)&255]>0 and word(address+16)&15==1 and word(address+20)&65535==4
  mutation=spec.get('mutation')
  if mutation:
   phase,action=mutation
   if phase==child or phase=='mentions0'and child=='mentions'and oi==0 or phase=='mentions1'and child=='mentions'and oi==1:
    if action=='push':write(address+8,ops.index('PUSH')<<8)
    elif action=='data':write(address+8,ops.index('DATA')<<8)
    elif action=='next-null':write(address,0)
    elif action=='work-next-null':write(block_work[wi],0)
    elif action=='operand0-ebp':write(address+16,1);write(address+20,5)
    elif action=='operand-self-register':write(ecx,1);write(ecx+4,5)
    else:raise AssertionError(action)
  m.reg_write(UC_X86_REG_EAX,value);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if side else row['start'],0,count=100000)
 assert terminal==['return']and u.reg_read(UC_X86_REG_ESP)==0x300804 and u.reg_read(UC_X86_REG_EAX)==expected_result and all(u.reg_read(r)==v for r,v in saved),(side,blocks,events,u.reg_read(UC_X86_REG_EAX),expected_result)
 assert bytes(u.mem_read(0x400000,len(expected)))==expected
 return dict(result=expected_result,events=events,memory_sha256=hashlib.sha256(expected).hexdigest())
rows=[]
def case(label,blocks,result):
 a=run(original,False,blocks,result);b=run(canonical,True,blocks,result);assert a==b,(label,a,b);rows.append(dict(case=label,blocks=blocks,result=a))
case('empty work chain',[],1)
for flags in [0,1,2,3,0x100,0x102]:
 for instrs in [[],[instruction('DATA')],[instruction(kind=7),instruction('DATA')],[instruction(kind=4),instruction('MOV')]]:case('block flags/markers',[(flags,instrs)],0 if flags&2 and instrs and instrs[0]['kind']==0 and ops[instrs[0]['op']]=='DATA'else 1)
for op in ops:case('opcode '+op,[(2,[instruction(op)])],0 if op=='DATA'else 1)
for op in ['MOV','PUSH','CMP']:
 for write in [0,1]:case('explicit ESP write',[(2,[instruction(op,operand(1,reg=0xdead0004),writes=write)])],not write)
for mask in [0,1,0x10,0x20,0x80000010]:case('implicit ESP mask',[(2,[instruction('PUSH',implicit=mask)])],not bool(mask&0x10))
for oi,kind,reg,base,index,symbol in itertools.product([0,1],[1,2,3,4,5,6,8],[0,5],[0,5],[0,5],[None,(1,0),(1,0x10),(1,0x30),(1,0x70),(19,0),(3,0)]):
 # Reduce irrelevant Cartesian duplicates; retain independently active fields.
 if kind!=1 and reg:continue
 if kind not in [4,5]and(base or index or symbol):continue
 if kind==4 and index:continue
 if kind==1 and(base or index or symbol):continue
 if kind not in [1,4,5]and(reg or base or index or symbol):continue
 o=operand(kind,reg,base,index,symbol,0xabc00000);a=[operand(),operand(2)];a[oi]=o;mentions=(kind==1 and reg==5)or(kind in [4,5]and(base==5 or(kind==5 and index==5)));allowed=not mentions or(kind==4 and symbol is not None and(symbol[0]==19 or(symbol[0]==1 and symbol[1]&0x70 in [0x10,0x30])))
 case('conditional EBP operand',[(2,[instruction(a=a[0],b=a[1])])],int(allowed))
case('writes then refresh opcode',[(2,[instruction(a=operand(1,reg=4),implicit=0x10,mutation=('writes','push'))])],0)
case('implicit then refresh DATA',[(2,[instruction('PUSH',mutation=('implicit','data'))])],0)
case('later operand fresh after source callback',[(2,[instruction(b=operand(4,base=5,symbol=(1,0x10)),mutation=('mentions1','operand0-ebp'))])],0)
case('re-read kind after mentions',[(2,[instruction(a=operand(4,base=5,symbol=(1,0x30)),mutation=('mentions0','operand-self-register'))])],0)
case('fresh next after mentions',[(2,[instruction(mutation=('mentions0','next-null')),instruction('DATA')])],1)
case('fresh work next after mentions',[(2,[instruction(mutation=('mentions0','work-next-null'))]),(2,[instruction('DATA')])],1)
case('skip blocks and finish at marker',[(0,[instruction('DATA')]),(2,[instruction(kind=4),instruction(),instruction(kind=7),instruction('DATA')]),(2,[])],1)
Path('cases/frame-allow-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole385B original/canonical read-only scan with mocked accepted children: work/flag/empty/marker paths, all231 actual opcode counts/normalized implicit-property identities, explicitESP4 writes, implicitmaskbit10, DATA rejection, reverse operand5 mentions and basedlocal roles10/30/kind19 exception, nonlocal/register/indexed/NULL rejection. Child mutation controls verify opcode reload after writes/implicit query, later operand/kind reload, nextinstruction/work reload; caller-saved poison/RET0/fullboolean/nonvolatile/everyotherbyte preserved. Input inventory is separately bound to actual five-work graph, not an all-branches execution claim. Mocked child result combinations are controls, not valid public input claims; no new guard/data/helper.'),indent=2)+'\n');print(len(rows),'whole frame-register controls and8 actual ABI negatives pass')
