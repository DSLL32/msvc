"""Actual malloc12288 recursive-node slab; reserved/tail bytes gain no credit."""
import hashlib,json
from state import BINARY
from objects import namespace

def word(raw,offset,size=4):return int.from_bytes(raw[offset:offset+size],'little')

def required(config,case):return case=='hello-debug-dll'and any(r['name']=='publish_recursive_type'and r['status']!='stub'for r in config['functions'])

def validate(config,pe,rebuilt=None,linked=None):
    ref=config.get('recursive_arena_observations');assert ref or not required(config,'hello-debug-dll'),'missing recursive slab ownership proof'
    if not ref:return
    path=BINARY/ref['evidence'];assert hashlib.sha256(path.read_bytes()).hexdigest()==ref['evidence_sha256'],'stale recursive slab proof'
    p=json.loads(path.read_text());assert p['schema']==1 and p['capacity']==0x3000 and p['record_size']==12 and p['slots']==1024
    assert p['maximum_allocations']==1 and p['published_types']==[0x1047]and p['input_types']==[4209]
    assert p['defined_fields']==[[0,6],[8,12]]and p['reserved_fields']==[[6,8]]
    assert p['counter']==0x352b4 and p['cursor']==0x352b8 and p['buckets']==0x37168 and p['counts']==0x3716c and p['allocation_return']==0x19cb6
    assert pe.read(0x352b4,8)==b'\0'*8,'recursive pool initial state differs'
    for source,digest in p['sources'].items():assert hashlib.sha256((BINARY/source).read_bytes()).hexdigest()==digest,'recursive pool source binding differs'
    for filename in('data.json','walks.json'):assert hashlib.sha256((BINARY/filename).read_bytes()).hexdigest()==p[filename+'_sha256'],'recursive pool ownership registry differs'
    for region in p['instructions']:assert hashlib.sha256(pe.read(region['start'],region['end']-region['start'])).hexdigest()==region['sha256'],'recursive pool creation/publication bytes differ'
    lines=(BINARY/'src/types.c').read_text().splitlines();line=p['native_allocation_line']
    assert lines[line-2].strip()=='block = checked_alloc(0x3000);'and lines[line-1].strip()=='recursive_type_cursor = block;','recursive pool allocation source differs'
    data={r['name']:r for r in json.loads((BINARY/'data.json').read_text())}
    for name,address in(('recursive_type_slots',0x352b4),('recursive_type_cursor',0x352b8)):
        assert data[name]['original']==address and data[name]['size']==4 and data[name]['source']=='src/types.c','recursive pool contextual global owner differs'
    if rebuilt is not None:p['compiled']=compiled_provenance(rebuilt,linked)
    return p

def compiled_provenance(pe,linked):
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    start=linked['@publish_recursive_type@4'];end=min(v for v in linked.values()if v>start)
    code=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(pe.read(start,end-start),start))
    calls=[(j,r)for j,r in enumerate(code)if r.mnemonic=='call']
    assert len(calls)==2 and [r.op_str for j,r in calls]==['0x%x'%linked['_checked_alloc'],'0x%x'%linked['@hash_recursive_type@4']],'recursive pool constructor callees differ'
    j,call=calls[0];assert code[j-1].bytes==bytes.fromhex('6800300000'),'recursive pool requested size differs'
    raw=pe.read(call.address+5,24);assert raw[:5]==bytes.fromhex('83c4048945'),'recursive pool allocation return differs'
    block=raw[5:6];cursor=linked['_recursive_type_cursor'].to_bytes(4,'little');counter=linked['_recursive_type_slots'].to_bytes(4,'little')
    assert raw==b'\x83\xc4\x04\x89\x45'+block+b'\x8b\x45'+block+b'\xa3'+cursor+b'\xc7\x05'+counter+b'\x00\x04\x00\x00','recursive pool malloc->cursor/capacity ownership differs'
    j,hashed=calls[1];before=code[j-1].bytes;assert before[:2]==b'\x8b\x4d'and len(before)==3,'recursive pool hash owner differs';entry=before[2:3]
    raw=pe.read(hashed.address+5,end-hashed.address-5);assert raw[:2]==b'\x89\x45'and raw[3:5]==b'\xff\x0d','recursive pool hash result differs';hash_slot=raw[2:3]
    at=3;assert raw[at:at+6]==b'\xff\x0d'+counter;at+=6
    assert raw[at:at+7]==b'\xa1'+cursor+b'\x89\x45';node=raw[at+7:at+8];at+=8
    prefix=b'\x83\x05'+cursor+b'\x0c';assert raw[at:at+len(prefix)]==prefix;at+=len(prefix)
    heads=linked['_name_hash_buckets'].to_bytes(4,'little');counts=linked['_name_hash_counts'].to_bytes(4,'little')
    fields=(b'\x8b\x45'+entry+b'\x8b\x00\x8b\x4d'+node+b'\x89\x01'+b'\x8b\x45'+entry+b'\x66\x8b\x40\x04\x8b\x4d'+node+b'\x66\x89\x41\x04'+b'\x8b\x45'+hash_slot+b'\x8b\x0d'+heads+b'\x8b\x04\x81\x8b\x4d'+node+b'\x89\x41\x08'+b'\x8b\x45'+node+b'\x8b\x4d'+hash_slot+b'\x8b\x15'+heads+b'\x89\x04\x8a'+b'\x8b\x45'+hash_slot+b'\x8b\x0d'+counts+b'\x66\xff\x04\x41')
    assert raw[at:at+len(fields)]==fields,'recursive pool same-node data/index/next/publication def-use differs'
    return dict(call=call.address,size=0x3000,node_frame_offset=int.from_bytes(node,'little',signed=True),publication=fields.hex(),rebuilt_sha256=hashlib.sha256(pe.data).hexdigest())

def parse(events):
    rows=[];frontier=None
    for line in events.splitlines():
        parts=line.split()
        if parts and parts[0]=='recursive-hash-allocation':
            assert len(parts)==5
            rows.append(dict(base=int(parts[1],16),size=int(parts[2]),owner=int(parts[3],16),esp=int(parts[4],16)))
        elif parts and parts[0]=='recursive-hash-bytes':
            assert len(parts)==4
            if parts[1]=='birth':
                assert rows and 'before'not in rows[-1]and rows[-1]['base']==int(parts[2],16);rows[-1]['before']=parts[3]
            else:
                assert parts[1]=='frontier'and frontier is None;frontier=dict(base=int(parts[2],16),bytes=parts[3])
    return dict(allocations=rows,frontier=frontier)

def compare(config,pe,events,native,old_walks,new_walks,state,case,buffer_context):
    p=validate(config,pe)
    if not native:
        assert not required(config,case),'missing mandatory recursive slab observations'
        return
    assert p and buffer_context and not native.get('error'),'recursive slab observer failed'
    original=parse(events);assert len(original['allocations'])==len(native['allocations'])==1 and original['frontier']and native['frontier'],'missing/extra recursive slab lifetimes'
    assert native['frontier']['name']=='hash_referenced_type','unsupported recursive slab consumer'
    assert events.splitlines().count('recursive-hash-rearm 00019cb6 00019cb9')==1,'recursive slab allocation probe not rearmed'
    memory={}
    for line in events.splitlines():
        if line.startswith('memory '):
            _,address,raw=line.split();memory.update((int(address,16)+i,v)for i,v in enumerate(bytes.fromhex(raw)))
    def value(address):return word(bytes(memory[address+i]for i in range(4)),0)
    extras=[];logical=[]
    for side,walks,buffers in((original,old_walks,buffer_context['original_namespace']),(native,new_walks,buffer_context['native_namespace'])):
        a=side['allocations'][0];f=side['frontier'];base=a['base'];before=bytes.fromhex(a['before']);after=bytes.fromhex(f['bytes'])
        assert base and base%4==0 and a['size']==len(before)==len(after)==0x3000 and f['base']==base,'recursive slab actual extent differs'
        table=walks['local_type_records'][0];assert a['owner']==table['address']+(4209-0x1000)*28,'recursive slab constructor parent differs'
        slots=value(p['counter'])if side is original else f['remaining'];cursor=value(p['cursor'])if side is original else f['cursor']
        assert slots==1023 and cursor==base+12,'recursive slab used prefix/cursor differs'
        if side is native:assert state['globals']['recursive_type_slots']==slots.to_bytes(4,'little').hex()and state['globals']['recursive_type_cursor']['address']==cursor,'recursive slab live globals differ'
        assert after[6:8]==before[6:8]and after[12:]==before[12:],'recursive slab reserved/unused bytes changed without a defining write'
        heads=bytes.fromhex(walks['name_hash_buckets'][0]['bytes']);counts=bytes.fromhex(walks['name_hash_counts'][0]['bytes']);assert len(heads)==16384 and len(counts)==8192
        seen=set();nodes=[];payload=buffers['type_buffer_payload'][0];copy=buffer_context['comparison']['copies'][71]
        assert copy['index']==4209 and copy['size']==36
        for bucket in range(4096):
            pointer=word(heads,bucket*4);count=0
            while pointer:
                assert pointer==base and pointer not in seen,'recursive slab unowned/aliased/uninitialized node'
                seen.add(pointer);count+=1
                assert word(after,4,2)==0x1047 and word(after,0)==payload['address']+copy['offset']and word(after,8)==0,'recursive node index/record/next differs'
                record=bytes.fromhex(table['bytes'])[(4209-0x1000)*28:(4209-0x1000+1)*28]
                assert word(record,10,2)==bucket and word(record,4,2)==0x1047 and word(record,0)==word(after,0),'recursive node hash/TypeRecord owner differs'
                nodes.append(dict(bucket=bucket,index=0x1047,input_type=4209,data_offset=copy['offset'],next=None));pointer=word(after,8)
            assert word(counts,bucket*2,2)==count,'recursive node bucket count differs'
        assert len(seen)==1,'missing actual recursive slab node'
        assert all(not(base<address+size and address<base+0x3000)for address,size,identity in namespace(walks)+namespace(buffers)),'recursive slab overlaps another allocation owner'
        extras.append(dict(recursive_hash_storage=[dict(address=base,bytes=f['bytes'])]));logical.append(nodes)
    assert logical[0]==logical[1],'recursive slab logical state differs'
    return dict(nodes=logical[0],defined_ranges=[[0,6],[8,12]],original_namespace=extras[0],native_namespace=extras[1],original=original,native=native)
