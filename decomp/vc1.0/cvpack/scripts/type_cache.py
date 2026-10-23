"""Named type cache: exact initialized nodes, graph and owning input records."""
import hashlib,json
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_MEM,X86_OP_REG
from binary import PE
from state import BINARY

def word(raw,offset,size=4):return int.from_bytes(raw[offset:offset+size],'little')

def validate(config,pe,rebuilt=None,linked=None):
    ref=config.get('type_cache_observations')
    required=any(r['name']=='index_new_types'and r['status']!='stub'for r in config['functions'])
    assert ref or not required,'missing type cache allocation/publication proof'
    if not ref:return
    path=BINARY/ref['evidence'];assert hashlib.sha256(path.read_bytes()).hexdigest()==ref['evidence_sha256'],'stale type cache proof'
    p=json.loads(path.read_text())
    assert p['size']==16 and p['buckets']==512 and p['maximum_nodes']==4096
    assert pe.read(0x24ed0,7).hex()=='6a10e889a1ffff','type cache allocation call differs'
    assert pe.read(0x24eee,0x26).hex()==p['publication_bytes'],'type cache publication instructions differ'
    assert pe.read(0x24cc0,0x2d7).hex()==p['owner_body'],'type cache owner body differs'
    for source,digest in p['sources'].items():assert hashlib.sha256((BINARY/source).read_bytes()).hexdigest()==digest,'type cache source changed'
    walk=next(r for r in json.loads((BINARY/'walks.json').read_text())if r['name']=='type_work_a')
    assert walk.get('pointer_array')is True and walk.get('type_cache')is True and walk['size']==2048,'missing typed cache graph observation'
    if rebuilt is not None:p['compiled_allocation']=compiled_provenance(rebuilt,linked)
    return p

def compiled_provenance(pe,linked):
    decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    start=linked['_index_new_types'];end=min(v for v in linked.values()if v>start)
    code=list(decoder.disasm(pe.read(start,end-start),start))
    sites=[j for j,i in enumerate(code)if i.mnemonic=='call'and i.op_str=='0x%x'%linked['_checked_zero']]
    assert len(sites)==1,'cache allocator identity differs'
    j=sites[0];call=code[j]
    assert (code[j-1].mnemonic,code[j-1].op_str)==('push','0x10'),'cache allocation size differs'
    assert code[j+1].bytes==b'\x83\xc4\x04'and code[j+2].mnemonic=='mov'and code[j+2].operands[0].type==X86_OP_MEM and code[j+2].operands[1].type==X86_OP_REG and code[j+2].op_str.endswith(', eax'),'cache allocation return provenance differs'
    slot=code[j+2].operands[0].mem.disp
    assert code[j+2].reg_name(code[j+2].operands[0].mem.base)=='ebp'and -128<=slot<0
    root=linked['_type_work_a'].to_bytes(4,'little');frame=bytes([slot&255])
    # The same allocation result first owns the next link, then becomes the
    # bucket head. Subsequent source fields use this same local node pointer.
    publication=b'\x8b\x4d'+frame+b'\x89\x41\x0c\x8b\x45'+frame
    raw=pe.read(call.address,end-call.address);at=raw.find(publication)
    assert at>=0 and raw.count(publication)==1,'cache publication allocation owner differs'
    tail=raw[at+len(publication):at+len(publication)+12]
    prefix=raw[at-12:at]
    assert len(prefix)==12 and prefix[:2]==b'\x8b\x45'and prefix[3:5]==b'\x8b\x0d'and prefix[5:9]==root and prefix[9:12]==b'\x8b\x04\x81','cache old bucket-head provenance differs'
    assert tail[:2]==b'\x8b\x4d'and tail[2]==prefix[2]and tail[3:5]==b'\x8b\x15'and tail[5:9]==root and tail[9:12]==b'\x89\x04\x8a','cache publication root provenance differs'
    helper=linked['_checked_zero'];helper_end=min(v for v in linked.values()if v>helper)
    body=pe.read(helper,helper_end-helper)
    instructions=list(decoder.disasm(body,helper))
    calls=[j for j,i in enumerate(instructions)if i.mnemonic=='call'and i.op_str=='0x%x'%linked['_malloc']]
    assert len(calls)==1,'cache zero helper allocator differs'
    k=calls[0]
    assert instructions[k-1].bytes==b'\xff\x75\x08'and instructions[k+1].bytes==b'\x83\xc4\x04','cache zero helper allocation provenance differs'
    store=instructions[k+2]
    assert store.bytes[:2]==b'\x89\x45'and store.size==3,'cache zero helper returned owner differs'
    data_slot=store.bytes[2:3]
    initializers=[i for i in instructions if i.bytes[:2]==b'\xc7\x45'and i.size==7 and i.bytes[3:]==b'\0'*4]
    assert len(initializers)==1,'cache zero helper counter initialization differs'
    counter=initializers[0].bytes[2:3];assert counter!=data_slot
    prefix=b'\xc7\x45'+counter+b'\0'*4+b'\xe9\x03\0\0\0\xff\x45'+counter
    suffix=b'\x8b\x45'+counter+b'\x8b\x4d'+data_slot+b'\xc6\x04\x08\0\xe9\xe2\xff\xff\xff\x8b\x45'+data_slot
    # VC1 varies which operand it loads first in this unchanged unsigned loop.
    # Both complete instruction strings prove exactly counter < requested size;
    # preserve the bound slot, condition, increment, owner and zero-store bytes.
    forms=[('size_then_counter',b'\x8b\x45\x08\x39\x45'+counter+b'\x0f\x83\x0f\0\0\0',2),
           ('counter_then_size',b'\x8b\x45'+counter+b'\x39\x45\x08\x0f\x86\x0f\0\0\0',5)]
    matches=[]
    for orientation,condition,bound in forms:
        loop=prefix+condition+suffix
        if loop in body:
            assert body.count(loop)==1,'ambiguous zero helper loop'
            at=body.index(loop);matches.append((orientation,at+len(prefix)+bound,at+len(prefix)+7))
    assert len(matches)==1,'cache initialized extent/zeroing differs'
    orientation,bound,branch=matches[0]
    return dict(zero_orientation=orientation,zero_bound=helper+bound,zero_branch=helper+branch,call=call.address,size=16,result_frame_offset=slot,publication_start=call.address+at,publication_bytes=raw[at:at+len(publication)+12].hex(),zero_helper=helper,zero_helper_sha256=hashlib.sha256(body).hexdigest(),rebuilt_sha256=hashlib.sha256(pe.data).hexdigest())

def compare(left,right,a,b,original,native,buffer_context=None):
    """All16 node bytes were initialized by reviewed checked_zero(16)."""
    def observe(root,walks):
        objects=root.get('objects',{});assert len(objects)<=4096
        table=bytes.fromhex(root['bytes']);assert len(table)==2048
        names=walks['type_data'];assert len(names)==1
        name_base=names[0]['address'];name_bytes=bytes.fromhex(names[0]['bytes'])
        assert len(name_bytes)==4746 and word(name_bytes,0)==1,'unreviewed cache name owner'
        proof=json.loads((BINARY/'cases/abi/type-cache.json').read_text())['original_symbol_preimage']
        assert proof['offset']==0xa3fc and proof['size']==4746 and proof['sha256']=='bcd39ae0eeb809e5a79fce8d64af2126edf8338eb1dc03bac5d0d446c0b0306b'
        preimage=(BINARY/proof['input']).read_bytes()[proof['offset']:proof['offset']+proof['size']]
        assert hashlib.sha256(preimage).hexdigest()==proof['sha256'],'stale original symbol name preimage'
        paired=BINARY/proof['paired_evidence'];assert hashlib.sha256(paired.read_bytes()).hexdigest()==proof['paired_sha256']
        assert bytes.fromhex(json.loads(paired.read_text())['walks']['type_data'][0]['bytes'])==preimage,'symbol preimage differs from paired before-pack state'
        allowed={};offset=4
        while offset<len(name_bytes):
            length=word(preimage,offset,2)+2;kind=word(preimage,offset+2,2)
            assert length>=4 and offset+length<=len(name_bytes),'invalid defined module type record'
            if kind==4:
                count=preimage[offset+6];assert offset+7+count<=offset+length and name_bytes[offset+6:offset+7+count]==preimage[offset+6:offset+7+count],'primary-cache Pascal name owner changed'
                allowed[name_base+offset+6]=(word(preimage,offset+4,2),preimage[offset+6:offset+7+count])
            offset+=length
        assert offset==len(name_bytes)
        records=walks['local_type_records'];assert len(records)==1
        data=bytes.fromhex(records[0]['bytes']);assert len(data)==12000*28
        seen=set();result=[];owned=set()
        for bucket in range(512):
            pointer=word(table,bucket*4);rank=0
            while pointer:
                key='cache-%d-%d'%(bucket,rank);assert key in objects,'missing type cache node'
                node=objects[key];raw=bytes.fromhex(node['bytes'])
                assert len(raw)==16 and pointer==node['address']and pointer not in seen,'type cache ownership differs'
                assert all(abs(pointer-other)>=16 for other in seen),'overlapping type cache allocations'
                seen.add(pointer);owned.add(key);assert len(seen)<=4096
                name,typ,index,next=word(raw,0),word(raw,4),word(raw,8,2),word(raw,12)
                assert raw[10:12]==b'\0\0','type cache initialized padding differs'
                assert name in allowed and allowed[name][0]==index,'type cache name/index identity differs'
                assert bucket==sum(allowed[name][1])&511,'type cache bucket differs'
                assert 0x1000<=index<0x1000+136,'unreviewed type cache record index'
                entry=data[(index-0x1000)*28:(index-0x1000+1)*28]
                assert not(word(entry,24,2)&0x5000)and typ,'type cache record owner differs'
                if word(entry,0)!=typ:
                    assert buffer_context,'missing stored primary-cache type lifetime'
                    copies=buffer_context['comparison']['copies'];matches=[(i,r)for i,r in enumerate(copies)if r['index']==index]
                    assert len(matches)==1 and index in(4114, 4171, 4177, 4185, 4192, 4197, 4209, 4213, 4216, 4220, 4222, 4225),'unreviewed primary-cache copied type'
                    rank,copy=matches[0];side='original_namespace'if walks is original else 'native_namespace'
                    payload=buffer_context[side]['type_buffer_payload'][0]
                    assert word(entry,0)==payload['address']+copy['offset']and word(entry,4,2)==0x1000+rank,'primary-cache current record does not own copied type'

                # Provider record storage is a previously verified typed owner.
                owner=walks['current_type_server'];assert len(owner)==1
                block=owner[0]['objects']['type-storage-1'];payload=bytes.fromhex(block['bytes']);start=typ-block['address']
                assert 12<=start and start+4<=12+4140,'type cache provider address differs'
                offsets=bytes.fromhex(owner[0]['objects']['offsets']['bytes'])
                assert word(offsets,(index-0x1000)*4)+2==typ,'primary-cache original provider record identity differs'
                size=word(payload,start,2)+2;kind=word(payload,start+2,2)
                assert kind in(4,5,6,7)and start+size<=12+4140,'type cache defined provider extent differs'
                result.append(dict(key=key,index=index,name_offset=name-name_base,name=allowed[name][1].hex(),type_offset=start,type_bytes=payload[start:start+size].hex(),next_key='cache-%d-%d'%(bucket,rank+1)if next else None,raw=raw))
                pointer=next;rank+=1
        assert owned==set(objects),'extra or unidentified type cache objects'
        return result
    old,new=observe(a,original),observe(b,native)
    assert len(old)==len(new),'type cache node count differs'
    result=[]
    for x,y in zip(old,new):
        for key in('key','index','name_offset','name','type_offset','type_bytes','next_key'):assert x[key]==y[key],('type cache state differs',key)
        l,r=bytearray(x['raw']),bytearray(y['raw'])
        for offset in(0,4,12):l[offset:offset+4]=r[offset:offset+4]=b'\0'*4
        assert l==r,'type cache initialized bytes differ'
        result.append({k:v for k,v in x.items()if k!='raw'})
    return result
