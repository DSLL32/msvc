"""Observed type buffer writes and three fully owned published hash nodes.

Malloc payload/padding remain raw evidence: only each observed copy's bytes
become defined. No allocator extent, initial-zero, or universal default claim.
"""
import hashlib,json,re
from objects import namespace,pointer_identity
from state import BINARY

def word(raw,offset,size=4):return int.from_bytes(raw[offset:offset+size],'little')

def required(config,case):return case=='hello-debug-dll'and any(r['name']=='append_buffer'and r['status']!='stub'for r in config['functions'])

def validate(config,pe):
    ref=config.get('buffer_storage_observations')
    assert ref or not required(config,'hello-debug-dll'),'missing type buffer lifetime proof'
    if not ref:return
    path=BINARY/ref['evidence'];assert hashlib.sha256(path.read_bytes()).hexdigest()==ref['evidence_sha256'],'stale type buffer proof'
    p=json.loads(path.read_text())
    assert p['schema']==1 and p['root']==0x3301c and p['block_size']==0x7f00 and p['node_size']==12 and p['maximum_blocks']==1
    assert p['record_indices']==[4115,4133,4134]and p['copy_sizes']==[8,12,8]and p['frontier']==0x262d0
    assert p['original_sites']==dict(entry=0x22f40,allocated=0x22f6d,returned=0x19a65)
    for source,digest in p['sources'].items():assert hashlib.sha256((BINARY/source).read_bytes()).hexdigest()==digest,'type buffer source binding differs'
    for r in p['instructions']:assert hashlib.sha256(pe.read(r['start'],r['end']-r['start'])).hexdigest()==r['sha256'],'type buffer instruction ownership differs'
    lines=(BINARY/'src/types.c').read_text().splitlines();line=p['native_allocation_line']
    assert lines[line-2].strip()=='payload = checked_alloc(buffer->block_size);'and lines[line-1].strip()=='block->data = payload;','type buffer allocation site differs'
    assert p['pdb_defined_writes']==[dict(type_index=4133,field=10,before=4115,after=0x1000),dict(type_index=4134,field=6,before=4133,after=0x1001)],'unreviewed PDB in-place writes'
    assert p['defined_hash_node_ranges']==[[0,2],[4,12]]and p['reserved_hash_node_range']==[2,4]
    return p


def parse(events):
    result=dict(entries=[],returns=[],allocations=[],hash_nodes={});current=None
    for line in events.splitlines():
        f=line.split()
        if line.startswith('buffer-entry '):
            assert len(f)==8
            current=dict(zip(('sequence','buffer','source','size','owner','esp','return_address'),[int(v,10 if i in(0,3)else 16)for i,v in enumerate(f[1:])]))
            result['entries'].append(current)
        elif line.startswith('buffer-allocation '):
            assert len(f)==6
            current=dict(zip(('base','node','size','buffer','esp'),[int(v,10 if i==2 else 16)for i,v in enumerate(f[1:])]))
            result['allocations'].append(current)
        elif line.startswith('buffer-return '):
            assert len(f)==4
            current=dict(sequence=int(f[1]),result=int(f[2],16),esp=int(f[3],16));result['returns'].append(current)
        elif line.startswith('buffer-bytes '):
            assert len(f)==4
            label,address,raw=f[1],int(f[2],16),f[3]
            if label.startswith('hash-'):
                assert label not in result['hash_nodes'],'duplicate type hash object'
                result['hash_nodes'][label]=dict(address=address,bytes=raw);continue
            mapping={'entry-root':'root','source':'source_bytes','birth-payload':'before','birth-node':'node_before','birth-root':'root_before','return-root':'root','return-node':'node','return-payload':'payload'}
            if label.startswith('frontier-'):
                current=result.setdefault('frontier',dict(name='finish_recursive_types'));key=label[9:]
            else:
                assert current is not None and label in mapping,'unowned buffer memory event';key=mapping[label]
            assert key not in current,'duplicate buffer state field'
            current[key]=raw;current[key+'_address']=address
    return result


def compare(config,pe,events,native,old_walks,new_walks,state,case):
    p=validate(config,pe)
    if not native:
        assert not required(config,case),'missing mandatory type buffer observations'
        return None
    assert p and case=='hello-debug-dll'and not native.get('error'),'native buffer observer failed'
    old=parse(events);results=[];spaces=[]
    for side,walks in((old,old_walks),(native,new_walks)):
        assert len(side['allocations'])==1 and len(side['entries'])==len(side['returns'])==3,'missing/extra type buffer copy lifetime'
        a=side['allocations'][0];assert a['base']and a['node']and a['size']==0x7f00
        assert len(bytes.fromhex(a['before']))==0x7f00 and bytes.fromhex(a['node_before'])==b'\0'*12,'buffer node calloc ownership differs'
        assert word(bytes.fromhex(a['root_before']),0)==0x7f00 and bytes.fromhex(a['root_before'])[4:]==b'\0'*20,'buffer root was already active at allocation'
        block=walks['current_type_server'][0]['objects']['type-storage-1'];defined=bytes.fromhex(block['bytes']);table=walks['local_type_records'][0]
        master=(BINARY/'cases/hello-debug-dll/input/msvc.pdb').read_bytes()
        assert hashlib.sha256(master).hexdigest()=='6421bc52a3c6b42958d229e92b0b4612dfb5335be933fe4ec995cd66fb30cd28'
        expected=bytearray(master[64:]);writes=[]
        # These are the two actual defined fields consumed by the observed
        # copies, with their preimage indices retained in the frozen PDB.
        for rank,field,prior,replacement in((1,10,4115,0x1000),(2,6,4133,0x1001)):
            entry=side['entries'][rank];offset=entry['source']-block['address']+field
            assert 12<=offset<=4150 and word(expected,offset-12,2)==prior,'PDB conversion field/preimage differs'
            expected[offset-12:offset-10]=replacement.to_bytes(2,'little')
            writes.append(dict(offset=offset,before=prior.to_bytes(2,'little').hex(),after=replacement.to_bytes(2,'little').hex(),type_index=p['record_indices'][rank],field=field))
        assert defined[12:4152]==expected,'unexpected PDB in-place modification'
        before=bytes.fromhex(a['before']);used=0;copies=[];last=before
        for i,(entry,returned,index,size)in enumerate(zip(side['entries'],side['returns'],p['record_indices'],p['copy_sizes'])):
            assert entry['sequence']==returned['sequence']==i+1 and entry['size']==size,'buffer call inventory differs'
            assert entry['buffer']==a['buffer']and entry['owner']==table['address']+(index-0x1000)*28,'buffer call parent ownership differs'
            source=entry['source'];offset=source-block['address'];raw=bytes.fromhex(entry['source_bytes'])
            assert 12<=offset and offset+size<=4152 and raw==defined[offset:offset+size]and word(raw,0,2)+2==size,'buffer source not the complete owned defined PDB record'
            root=bytes.fromhex(entry['root']);assert len(root)==24 and word(root,0)==0x7f00
            assert word(root,4)==(0 if i==0 else 1)and word(root,16)==(0 if i==0 else 0x7f00-used),'buffer pre-copy available state differs'
            assert word(root,8)==word(root,20)==(0 if i==0 else a['node'])and word(root,12)==(0 if i==0 else a['base']+used),'buffer pre-copy owner/cursor differs'
            assert returned['result']==a['base']+used,'buffer return address differs'
            post=bytes.fromhex(returned['payload']);assert len(post)==0x7f00 and post[:used]==last[:used]and post[used:used+size]==raw and post[used+size:]==last[used+size:],'unproved buffer write or untouched-tail mutation'
            used+=size;root=bytes.fromhex(returned['root']);node=bytes.fromhex(returned['node'])
            assert len(root)==24 and [word(root,o)for o in(0,4,8,12,16,20)]==[0x7f00,1,a['node'],a['base']+used,0x7f00-used,a['node']],'buffer post-copy root state differs'
            assert len(node)==12 and [word(node,o)for o in(0,4,8)]==[used,a['base'],0],'buffer post-copy block state differs'
            copies.append(dict(index=index,source_offset=offset,size=size,offset=used-size,bytes=raw.hex(),result=returned['result']))
            last=post
        f=side['frontier'];assert f['name']=='finish_recursive_types'and f['root']==side['returns'][-1]['root']and f['node']==side['returns'][-1]['node']and f['payload']==side['returns'][-1]['payload'],'buffer frontier changed without an observed write'
        extra={'type_buffer_payload':[dict(address=a['base'],bytes=f['payload'])],'type_buffer_block':[dict(address=a['node'],bytes=f['node'])]}
        nodes=side['hash_nodes'];assert len(nodes)==3,'missing/extra published type hash nodes'
        heads=bytes.fromhex(walks['type_hash_buckets'][0]['bytes']);counts=bytes.fromhex(walks['type_hash_counts'][0]['bytes']);seen=set();logical=[]
        by_address={r['address']:(key,r)for key,r in nodes.items()};assert len(by_address)==3,'aliased hash node allocation'
        for bucket in range(4096):
            pointer=word(heads,bucket*4);rank=0
            while pointer:
                assert pointer in by_address and pointer not in seen,'unowned/cyclic type hash node'
                seen.add(pointer);key,row=by_address[pointer];raw=bytes.fromhex(row['bytes']);assert len(raw)==12
                index=word(raw,0,2);assert key=='hash-%d-%d'%(bucket,index)and 0x1000<=index<0x1003,'wrong type hash owner/index'
                copy=copies[index-0x1000];assert word(raw,4)==copy['result'],'type hash record pointer does not own observed copy'
                record=bytes.fromhex(table['bytes'])[(copy['index']-0x1000)*28:(copy['index']-0x1000+1)*28]
                assert word(record,0)==copy['result']and word(record,4,2)==index,'type record publication differs'
                extra['type_hash_node_%04x'%index]=[row]
                logical.append(dict(bucket=bucket,index=index,data_offset=copy['offset'],next_index=word(bytes.fromhex(by_address[word(raw,8)][1]['bytes']),0,2)if word(raw,8)else None))
                pointer=word(raw,8);rank+=1
            assert word(counts,bucket*2,2)==rank,'type hash count does not match owned chain'
        assert len(seen)==3
        owners=namespace(walks)+namespace(extra)
        new_owners=namespace(extra)
        for address,size,identity in new_owners:
            assert address%4==0 and all(identity==other or not(address<base+length and base<address+size)for base,length,other in owners),'buffer allocation overlaps another typed owner'
        results.append(dict(pdb_writes=writes,copies=[{k:v for k,v in r.items()if k!='result'}for r in copies],hash_nodes=logical,defined=used,payload_sha256=hashlib.sha256(last[:used]).hexdigest()))
        spaces.append(extra)
    assert results[0]==results[1],'type buffer defined writes/hash topology differ'
    a,b=old['allocations'][0],native['allocations'][0]
    assert a['buffer']==0x3301c and b['buffer']==state['global_addresses']['type_symbol_storage'],'buffer root identity differs'
    for entry,returned in zip(old['entries'],old['returns']):
        assert entry['return_address']==0x19a65 and returned['esp']==entry['esp']+8,'buffer callee cleanup/caller proof differs'
    for site,next,count in((0x22f40,0x22f41,3),(0x22f6d,0x22f70,1),(0x19a65,0x19a67,3)):
        assert events.splitlines().count('buffer-rearm %08x %08x'%(site,next))==count,'missing per-call buffer observer rearm'
    return dict(comparison=results[0],original_namespace=spaces[0],native_namespace=spaces[1],original=old,native=native)


def compiled_provenance(pe,linked):
    """VC1 emitted allocation result -> same local -> fields -> bucket head."""
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    start=linked['@store_type@4'];end=min(v for v in linked.values()if v>start)
    code=list(decoder.disasm(pe.read(start,end-start),start));sites=[i for i,r in enumerate(code)if r.mnemonic=='call'and r.op_str=='0x%x'%linked['_checked_alloc']]
    assert len(sites)==1,'type hash allocator identity differs'
    i=sites[0];call=code[i];assert code[i-1].bytes==b'\x6a\x0c','type hash allocation size differs'
    raw=pe.read(call.address+5,80)
    assert raw[:5]==b'\x83\xc4\x04\x89\x45','type hash allocation return differs'
    slot=raw[5:6];entry=raw[8:9];bucket=raw[33:34]
    heads=linked['_type_hash_buckets'].to_bytes(4,'little');counts=linked['_type_hash_counts'].to_bytes(4,'little')
    expected=(b'\x83\xc4\x04\x89\x45'+slot+b'\x8b\x45'+entry+b'\x8b\x00\x8b\x4d'+slot+b'\x89\x41\x04\x8b\x45'+entry+b'\x66\x8b\x40\x04\x8b\x4d'+slot+b'\x66\x89\x01\x0f\xb7\x45'+bucket+b'\x8b\x0d'+heads+b'\x8b\x04\x81\x8b\x4d'+slot+b'\x89\x41\x08\x8b\x45'+slot+b'\x0f\xb7\x4d'+bucket+b'\x8b\x15'+heads+b'\x89\x04\x8a\x0f\xb7\x45'+bucket+b'\x8b\x0d'+counts+b'\x66\xff\x04\x41')
    assert raw[:len(expected)]==expected,'type hash allocation field/publication def-use differs'
    append=linked['@append_buffer@12'];append_end=min(v for v in linked.values()if v>append)
    instructions=list(decoder.disasm(pe.read(append,append_end-append),append))
    zeros=[j for j,r in enumerate(instructions)if r.mnemonic=='call'and r.op_str=='0x%x'%linked['_checked_zero']]
    mallocs=[j for j,r in enumerate(instructions)if r.mnemonic=='call'and r.op_str=='0x%x'%linked['_checked_alloc']]
    assert len(zeros)==len(mallocs)==1,'buffer allocation callee identity differs'
    j,k=zeros[0],mallocs[0]
    assert instructions[j-1].bytes==b'\x6a\x0c'and k==j+5,'buffer calloc size/ordering differs'
    middle=pe.read(instructions[j].address+5,instructions[k].address-instructions[j].address-5)
    assert middle[:5]==b'\x83\xc4\x04\x89\x45'and middle[6:8]==b'\x8b\x45'and middle[9:]==b'\xff\x30','buffer payload size is not the parent block_size'
    node_slot=middle[5:6];buffer_slot=middle[8:9]
    tail=pe.read(instructions[k].address+5,18)
    assert tail[:5]==b'\x83\xc4\x04\x89\x45','buffer payload result owner differs'
    payload_slot=tail[5:6]
    assert tail[6:]==b'\x8b\x45'+payload_slot+b'\x8b\x4d'+node_slot+b'\x89\x41\x04\x8b\x45'+buffer_slot,'buffer payload->calloc-node->parent def-use differs'
    from type_cache import compiled_provenance as zero_provenance
    zero=zero_provenance(pe,linked)
    return dict(call=call.address,size=12,node_frame_offset=int.from_bytes(slot,'little',signed=True),publication=expected.hex(),zero_helper=zero['zero_helper_sha256'],rebuilt_sha256=hashlib.sha256(pe.data).hexdigest())
