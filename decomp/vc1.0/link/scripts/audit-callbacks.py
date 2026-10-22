#!/usr/bin/env python3
"""Compare reconstructed routines' relocated function-pointer constants."""
from binary import u16,u32
from indexes import indexes
original,reconstructed,cfg,oi,ri=indexes()
def fixups(pe):
    result=[]
    cursor=pe.offset(pe.base+u32(pe.data,pe.optional+136))
    end=cursor+u32(pe.data,pe.optional+140)
    while cursor<end:
        page,size=u32(pe.data,cursor),u32(pe.data,cursor+4)
        if not size: break
        for offset in range(cursor+8,cursor+size,2):
            item=u16(pe.data,offset)
            if item>>12==3: result.append(pe.base+page+(item&4095))
        cursor+=size
    return result
def references(pe,records,r,relocations):
    result=set()
    names={x['start']:x['name'] for x in records}
    for address in relocations:
        if r['start']<=address and address+4<=r['end']:
            target=u32(pe.data,pe.offset(address))
            if target in names: result.add(names[target])
    return result
old_fixups,new_fixups=fixups(original),fixups(reconstructed)
checked=0
for record in cfg['driver']:
    if record['name'] not in cfg['accepted']: continue
    counterpart=next(r for r in ri if r['name']==record['name'])
    old=references(original,oi,record,old_fixups)
    new=references(reconstructed,ri,counterpart,new_fixups)
    assert old==new,(record['name'],'original only',old-new,'reconstruction only',new-old)
    checked+=1
print('%d source routines preserve their named function-pointer references.'%checked)
