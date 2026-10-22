"""Analysis-only recovery of grammar RHS symbols from original LR kernels.

A completed production fixes its last symbol through the state's incoming
label. Backward propagation across proven transitions fixes earlier positions.
An item at dot zero proves the corresponding nonterminal goto and closure.
Repeating these constraints recovers every production without guessing C++ BNF.
This investigation tool is never an input to the reconstruction build.
"""
import sys, struct, collections, json
from pathlib import Path
HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(HERE))
from binary import PE
pe = PE(HERE.parents[3] / 'toolchains/1.0-nt/bin/C1XX3232.EXE')
def shorts(a,n): return struct.unpack('<'+'h'*n,pe.read(a,n*2))
act=shorts(0xa3b10,2584);pact=shorts(0xa4f40,575)
pgo=shorts(0xa53c0,145);lhs=list(pe.read(0xa54e8,357));length=list(pe.read(0xa5650,357))
check=shorts(0xa57b8,575);default=shorts(0xa5c38,575)
rules_by_lhs=collections.defaultdict(list)
for p in range(1,len(lhs)):rules_by_lhs[-lhs[p]].append(p)
def shift(q,t):
 if pact[q] <= -1000:return None
 k=pact[q]+t
 if not 0<=k<len(act):return None
 d=act[k]
 return d if 0<=d<len(check) and check[d]==t else None
def goto(q,n):
 k=pgo[-n]+q+1
 d=act[k] if k<len(act) else -1
 if not 0<=d<len(check) or check[d]!=n:d=act[pgo[-n]]
 assert check[d]==n,(q,n,d,check[d])
 return d
items=[set() for _ in check];reverse=[set() for _ in check];queue=collections.deque();rhs={};waiting=collections.defaultdict(set);edges=set()
def item(q,p,k):
 if (p,k) not in items[q]:items[q].add((p,k));queue.append((q,p,k))
def edge(q,s,d):
 if (q,s,d) in edges:return
 edges.add((q,s,d));reverse[d].add((q,s))
 for p,k in list(items[d]):
  if k>0:item(q,p,k-1)
for q in range(len(check)):
 for t in range(257):
  d=shift(q,t)
  if d is not None:edge(q,t,d)
 if default[q]>0:item(q,default[q],length[default[q]])
 elif default[q]==-2:
  offset=pe.read(0x9e8a0+q,1)[0];a=0x9e788+offset*2
  while True:
   token,p=shorts(a,2)
   if p>0:item(q,p,length[p])
   if token<0:break
   a+=4
while queue:
 q,p,k=queue.popleft()
 if k:
  key=(p,k-1);label=check[q]
  assert label!=-1000,(q,p,k)
  if key in rhs:assert rhs[key]==label,('conflicting production symbol',q,p,k,rhs[key],label)
  else:
   rhs[key]=label
   for state in waiting.pop(key,set()):queue.append((state,p,k-1))
  for previous,label in list(reverse[q]):item(previous,p,k-1)
 else:
  n=-lhs[p]
  edge(q,n,goto(q,n))
  for alternative in rules_by_lhs[n]:item(q,alternative,0)
 if k<length[p]:
  key=(p,k)
  if key not in rhs:waiting[key].add(q)
  else:
   label=rhs[key]
   if label<0:
    for alternative in rules_by_lhs[label]:item(q,alternative,0)
    d=goto(q,label);edge(q,label,d)
   else:d=shift(q,label)
   if d is not None:item(d,p,k+1)
productions=[]
for p in range(1,len(lhs)):
 productions.append(dict(rule=p,lhs=lhs[p],rhs=[rhs.get((p,k)) for k in range(length[p])]))
if len(sys.argv) != 2:
 raise SystemExit('usage: recover-grammar.py OUTPUT.json (analysis only)')
Path(sys.argv[1]).write_text(json.dumps(productions,indent=2)+'\n')
complete=[p for p in productions if None not in p['rhs']]
print('Recovered',len(complete),'of',len(productions),'productions,',len(rhs),'of',sum(length),'RHS symbols; proven edges:',len(edges),'LR items:',sum(map(len,items)))
for p in productions:
 if None in p['rhs']:print('incomplete',p)
