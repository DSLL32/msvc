"""Typed assembly-scan input identities, independent of its execution model."""
def projection(d):
 ids={role:{}for role in ['block','instruction','symbol']}
 def identity(role,pointer):
  if not pointer:return None
  return ids[role].setdefault(pointer,len(ids[role]))
 out=[]
 for wi,w in enumerate(d['works']):
  assert w['index']==wi and w['next']==(d['works'][wi+1]['pointer']if wi+1<len(d['works'])else 0)
  row=dict(block=identity('block',w['block']),flags=w['flags']);out.append(row)
  if w['flags']&2:
   instructions=w['instructions'];assert w['first']==(instructions[0]['pointer']if instructions else 0);row['first']=identity('instruction',w['first']);row['instructions']=[]
   for ii,ins in enumerate(instructions):
    assert ins['index']==ii and ins['work']==wi
    a={k:v for k,v in ins.items()if k not in ['pointer','next','operands']};a['pointer']=identity('instruction',ins['pointer']);row['instructions'].append(a)
    if ins['kind']==7:assert ii==len(instructions)-1;continue
    assert ins['next']==(instructions[ii+1]['pointer']if ii+1<len(instructions)else 0);a['next']=identity('instruction',ins['next'])
    if ins['kind']==0:
     a['operands']=[]
     for o in ins['operands']:
      b=dict(o)
      if 'symbol_pointer'in b:b['symbol_pointer']=identity('symbol',b['symbol_pointer'])
      a['operands'].append(b)
 return out
