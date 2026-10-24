"""Scope-discovery input graph and pointer identities, without execution."""
def projection(d):
 trees={};symbols={};out=[]
 for index,row in enumerate(d['expressions']):
  assert row['index']==index and row['next']==(d['expressions'][index+1]['pointer']if index+1<len(d['expressions'])else 0)
  assert row['tree'];v=dict(opcode=row['opcode'],tree=trees.setdefault(row['tree'],len(trees)));out.append(v)
  if row['opcode']in [0x29,0x3a]:assert row['symbol'];v.update(symbol=symbols.setdefault(row['symbol'],len(symbols)),marker=row['marker'])
 return out
