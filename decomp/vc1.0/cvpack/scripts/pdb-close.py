"""Bounded consumed owners for the first genuine two-provider close call."""
import json,re
from pathlib import Path
BINARY=Path(__file__).resolve().parents[1]

def word(raw,offset):return int.from_bytes(raw[offset:offset+4],'little')

def capture(address,read):
    raw=read(address,116);chains=[]
    for offset in (72,84):
        blocks=[];pointer=word(raw,offset)
        while pointer:
            assert len(blocks)<8 and pointer not in [x['address']for x in blocks],'invalid close block chain'
            body=read(pointer,12);blocks.append(dict(address=pointer,bytes=body.hex()));pointer=word(body,8)
        chains.append(blocks)
    return dict(address=address,bytes=raw.hex(),chains=chains)

def compare(events,argument):
    rows=re.findall(r'^pdb-close-parent ([0-9a-f]+) ([0-9a-f]+)$',events,re.M)
    assert len(rows)==1,'missing actual close parent'
    address,raw=rows[0];old=dict(address=int(address,16),bytes=raw,chains=[[],[]])
    for family,index,base,body in re.findall(r'^pdb-close-block (\d+) (\d+) ([0-9a-f]+) ([0-9a-f]+)$',events,re.M):
        family,index=int(family),int(index);assert family in(0,1)and index==len(old['chains'][family]);old['chains'][family].append(dict(address=int(base,16),bytes=body))
    stack={int(i):int(v,16)for i,v in re.findall(r'^stack (\d+) ([0-9a-f]+)$',events,re.M)}
    roots=re.findall(r'^memory 000373d0 ([0-9a-f]+)$',events,re.M)
    assert len(roots)==1 and stack[0]==0x1a687 and stack[1]==old['address']==int.from_bytes(bytes.fromhex(roots[0]),'little'),'close caller/publication differs'
    assert argument['owner']==dict(root='current_type_server',address=argument['address'],requested_size=116)and argument['close']['address']==argument['address']
    master=(BINARY/'cases/two-pdb-providers/input/pdb-two.pdb').read_bytes()
    def summarize(parent):
        raw=bytes.fromhex(parent['bytes']);assert len(raw)==116 and raw[:64]==master[:64],'close PDB header differs'
        assert word(raw,68)==0 and word(raw,100)==0xffffffff,'unsupported close hash/descriptor state'
        count=int.from_bytes(raw[58:60],'little')-0x1000;payload=word(raw,60);capacity=(payload+0x1fff)&~0xfff
        assert count==30 and payload==608 and capacity==8192,'unreviewed actual close allocation sizes'
        types,index=parent['chains'];assert len(types)==2 and len(index)==1,'unexpected close chain inventory'
        ranges=[(parent['address'],116,'server'),(word(raw,64),count*4,'offsets')]
        ranges.extend((x['address'],12+(capacity if i==1 else 0),'types%d'%i)for i,x in enumerate(types))
        ranges.append((index[0]['address'],12,'index0'))
        for i,(base,size,name)in enumerate(ranges):
            assert base and all(base+size<=a or a+n<=base for a,n,_ in ranges[:i]),'aliased/unowned close allocation'
        assert word(raw,72)==types[0]['address']and word(raw,76)==word(raw,96)==types[1]['address']
        assert word(raw,84)==word(raw,88)==index[0]['address']and word(raw,80)==608 and word(raw,92)==0
        chains=[]
        for family,blocks in enumerate((types,index)):
            values=[]
            for i,block in enumerate(blocks):
                body=bytes.fromhex(block['bytes']);assert len(body)==12
                next_address=blocks[i+1]['address']if i+1<len(blocks)else 0
                used=608 if family==0 and i==1 else 0
                assert word(body,0)==0 and word(body,4)==block['address']+12+used and word(body,8)==next_address,'defined close block header differs'
                values.append(dict(owner=('types'if family==0 else'index')+str(i),available=0,cursor_offset=12+used,next=i+1 if next_address else None))
            chains.append(values)
        return dict(parent_bytes=116,offsets_requested=count*4,type_block_requested=[12,capacity+12],index_block_requested=12,descriptor=-1,hash_null=True,chains=chains)
    left,right=summarize(old),summarize(argument['close']);assert left==right
    return dict(original=old,native=argument['close'],matched=left,scope='Actual current_type_server116B and allocation provenance from constructor/read_records publication. Only descriptor/free owners and fully defined12B arena next headers consumed by close; offsets payload/type-data/tails not read or compared, parent incremental108 retained raw without equality claim.')
