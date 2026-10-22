from compare import indexes, publics
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM
from binary import u32
import re
orig,rec,cfg,oi,ri=indexes()
failures = 0
checked = 0
cs=Cs(CS_ARCH_X86,CS_MODE_32)
cs.detail=True

def instructions(pe,record):
 # Compiler switch tables belong to the function extent, but are data. Find
 # indexed tables whose entries are internal code addresses, then skip them.
 cursor=record['start']
 tables=[]
 while cursor<record['end']:
  containing=next((end for begin,end in tables if begin<=cursor<end),None)
  if containing is not None:
   cursor=containing
   continue
  decoded=list(cs.disasm(pe.read(cursor,15),cursor,count=1))
  if not decoded:
   cursor+=1
   continue
  instruction=decoded[0]
  assert cursor+instruction.size<=record["end"], (record["name"],hex(cursor),instruction.mnemonic,hex(record["end"]))
  for operand in instruction.operands:
   if operand.type!=X86_OP_MEM: continue
   # /Od may shift the index before JMP [eax+table], with scale one.
   if not ((operand.mem.index and operand.mem.scale==4) or instruction.mnemonic=='jmp'): continue
   table=operand.mem.disp
   count=0
   try:
    while count<4096:
     target=u32(pe.read(table+count*4,4),0)
     if not record['start']<=target<record['end']: break
     count+=1
   except (ValueError,KeyError,StopIteration,IndexError):
    pass
   if count>=2: tables.append((table,table+count*4))
  yield instruction
  cursor+=instruction.size

def at(addr,records):
 exact=[r for r in records if r['start']==addr]
 if exact:return exact[0]['name']
 return hex(addr)
def edges(pe,records,r):
 calls=set()
 for inst in instructions(pe,r):
  if inst.mnemonic=='jmp' and re.fullmatch(r'0x[0-9a-f]+',inst.op_str):
   target=int(inst.op_str,16)
   if not r['start']<=target<r['end'] and any(x['start']==target for x in records):calls.add(at(target,records))
  if inst.mnemonic=='call':
   if re.fullmatch(r'0x[0-9a-f]+',inst.op_str):calls.add(at(int(inst.op_str,16),records))
   else:calls.add('indirect')
 return calls
progress=list(cfg['driver'])
progress += [r for r in oi if r.get('evidence') and not any(x['name']==r['name'] for x in progress)]
for r in progress:
 if r['name'] not in cfg['accepted']:continue
 rr=next(x for x in ri if x['name']==r['name'])
 a,b=edges(orig,oi,r),edges(rec,ri,rr)
 checked += 1
 if a!=b:
  print(r['name'],'original only',a-b,'reconstruction only',b-a)
  failures += 1
if failures: raise SystemExit(1)
print('%d accepted source/runtime routines have matching call targets (including indirect calls and tail calls).' % checked)

boundary_checked=0
for record in oi:
 if record["name"] not in cfg["accepted"]:continue
 list(instructions(orig,record));boundary_checked+=1
print("%d original accepted extents end at decoded instruction boundaries." % boundary_checked)
