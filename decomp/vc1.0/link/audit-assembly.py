#!/usr/bin/env python3
"""Verify every reassembled instruction, width, register and symbolic address."""
import json
from capstone import *
from capstone.x86 import *
from binary import u16,u32
from compare import indexes,publics
orig,rec,cfg,oi,ri=indexes();names=publics()
data=json.load(open('data-symbols.json'))
cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
p=orig.offset(orig.base+u32(orig.data,orig.optional+136));end=p+u32(orig.data,orig.optional+140);fix=set()
while p<end:
    page,size=u32(orig.data,p),u32(orig.data,p+4)
    if not size:break
    for q in range(p+8,p+size,2):
        n=u16(orig.data,q)
        if n>>12==3:fix.add(orig.base+page+(n&4095))
    p+=size
checked=0
for r in cfg['driver']:
    if r.get('implementation')!='assembly' or r['name'] not in cfg['accepted']:continue
    rr=next(x for x in ri if x['name']==r['name'])
    a=list(cs.disasm(orig.read(r['start'],r['end']-r['start']),r['start']))
    b=list(cs.disasm(rec.read(rr['start'],rr['end']-rr['start']),rr['start']))
    # Reassembly may add unreachable alignment after the original last RET.
    limit=max(i for i,x in enumerate(a) if x.mnemonic.startswith('ret'))+1
    assert len(b)>=limit,r['name']+' truncated'
    targets={x.address:y.address for x,y in zip(a,b)}
    def relocated(value):
        if value in targets:return targets[value]
        choices=[x for x in data if x['address']<=value<x['address']+x['size']]
        if choices:
            d=min(choices,key=lambda x:x['size']);return names['_'+d['name']]+value-d['address']
        funcs=[x for x in oi if x['start']==value and x['name'] in names]
        assert funcs,'unmapped relocated address '+hex(value)
        return names[funcs[0]['name']]
    for x,y in zip(a[:limit],b[:limit]):
        assert x.mnemonic==y.mnemonic,(r['name'],hex(x.address),x.mnemonic,y.mnemonic)
        assert len(x.operands)==len(y.operands),(r['name'],hex(x.address),'operands')
        symbolic_memory=x.address+x.disp_offset in fix
        symbolic_immediate=x.address+x.imm_offset in fix
        for u,v in zip(x.operands,y.operands):
            assert (u.type,u.size)==(v.type,v.size),(r['name'],hex(x.address),'operand width')
            if u.type==X86_OP_REG:assert u.reg==v.reg
            if u.type==X86_OP_MEM:
                assert (u.mem.segment,u.mem.base,u.mem.index,u.mem.scale)==(v.mem.segment,v.mem.base,v.mem.index,v.mem.scale)
                assert (relocated(u.mem.disp) if symbolic_memory else u.mem.disp)==v.mem.disp,(r['name'],hex(x.address),'memory address')
            if u.type==X86_OP_IMM:
                if x.group(CS_GRP_CALL) or x.group(CS_GRP_JUMP):expected=relocated(u.imm)
                elif symbolic_immediate:expected=relocated(u.imm)
                else:expected=u.imm
                assert expected==v.imm,(r['name'],hex(x.address),'immediate',hex(expected),hex(v.imm))
    checked+=1
print('%d complete assembly routines preserve instruction semantics, operand widths and named addresses.'%checked)
