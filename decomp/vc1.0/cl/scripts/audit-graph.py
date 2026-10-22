from compare import indexes, publics
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
import re
orig,rec,cfg,oi,ri=indexes()
failures = 0
checked = 0
cs=Cs(CS_ARCH_X86,CS_MODE_32)
def at(addr,records):
 exact=[r for r in records if r['start']==addr]
 if exact:return exact[0]['name']
 return hex(addr)
def edges(pe,records,r):
 calls=set()
 for inst in cs.disasm(pe.read(r['start'],r['end']-r['start']),r['start']):
  if inst.mnemonic=='call':
   if re.fullmatch(r'0x[0-9a-f]+',inst.op_str):calls.add(at(int(inst.op_str,16),records))
   else:calls.add('indirect')
 return calls
for r in cfg['driver']:
 if r['name'] not in cfg['accepted']:continue
 rr=next(x for x in ri if x['name']==r['name'])
 a,b=edges(orig,oi,r),edges(rec,ri,rr)
 checked += 1
 if a!=b:
  print(r['name'],'original only',a-b,'reconstruction only',b-a)
  failures += 1
if failures: raise SystemExit(1)
print('%d reconstructed functions have matching call targets (including indirect calls).' % checked)
