"""Actual distinct packed-node slab; contained nodes share this one owner."""
import hashlib,json
from state import BINARY
from objects import namespace
from recursive_arena import word

def required(config):return any(r['name']=='publish_packed_type'and r['status']!='stub'for r in config['functions'])

def validate(config,pe,rebuilt=None,linked=None):
    ref=config.get('packed_arena_observations');assert ref or not required(config),'missing packed slab ownership proof'
    if not ref:return
    path=BINARY/ref['evidence'];assert hashlib.sha256(path.read_bytes()).hexdigest()==ref['evidence_sha256'],'stale packed slab evidence'
    p=json.loads(path.read_text());assert p['schema']==1 and p['capacity']==12288 and p['slots']==1024 and p['record_size']==12 and p['maximum_allocations']==1
    assert p['published_types']==[0x1049,0x1048,0x1047]and p['input_types']==[4199,4208,4209]and p['frontier']==0x257a0
    assert p['defined_fields']==[[0,2],[4,12]]and p['reserved_fields']==[[2,4]]
    assert p['counter']==0x350a8 and p['cursor']==0x350ac and p['allocation_return']==0x19c3b and pe.read(0x350a8,8)==b'\0'*8
    for filename,digest in p['sources'].items():assert hashlib.sha256((BINARY/filename).read_bytes()).hexdigest()==digest,'packed slab source changed'
    for filename in('data.json','walks.json'):assert hashlib.sha256((BINARY/filename).read_bytes()).hexdigest()==p[filename+'_sha256'],'packed slab registry changed'
    for r in p['instructions']:assert hashlib.sha256(pe.read(r['start'],r['end']-r['start'])).hexdigest()==r['sha256'],'packed slab constructor bytes changed'
    lines=(BINARY/'src/types.c').read_text().splitlines();line=p['native_allocation_line'];assert lines[line-2].strip()=='block = checked_alloc(0x3000);'and lines[line-1].strip()=='packed_type_cursor = block;','packed slab source allocation differs'
    data={r['name']:r for r in json.loads((BINARY/'data.json').read_text())}
    for name,address in(('packed_type_slots',0x350a8),('packed_type_cursor',0x350ac)):assert data[name]['original']==address and data[name]['size']==4 and data[name]['source']=='src/types.c','packed slab global owner differs'
    if 'address_frontier'in p:assert p['address_frontier']==0x25920,'unreviewed packed-pool frontier'
    if rebuilt is not None:p['compiled']=compiled_provenance(rebuilt,linked)
    return p

def compiled_provenance(pe,linked):
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    start=linked['@publish_packed_type@8'];end=min(v for v in linked.values()if v>start);code=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(pe.read(start,end-start),start));calls=[(j,r)for j,r in enumerate(code)if r.mnemonic=='call']
    assert len(calls)==2 and [r.op_str for j,r in calls]==['0x%x'%linked['_checked_alloc'],'0x%x'%linked['@hash_type@4']],'packed pool callees differ'
    j,call=calls[0];assert code[j-1].bytes==bytes.fromhex('6800300000'),'packed pool allocation size differs'
    raw=pe.read(call.address+5,24);block=raw[5:6];cursor=linked['_packed_type_cursor'].to_bytes(4,'little');counter=linked['_packed_type_slots'].to_bytes(4,'little')
    assert raw==b'\x83\xc4\x04\x89\x45'+block+b'\x8b\x45'+block+b'\xa3'+cursor+b'\xc7\x05'+counter+b'\x00\x04\x00\x00','packed pool allocation cursor/capacity def-use differs'
    j,hashed=calls[1];arg=code[j-1].bytes;assert arg[:2]==b'\x8b\x4d'and len(arg)==3;record=arg[2:3]
    raw=pe.read(call.address+29,hashed.address-call.address-29);assert raw[:6]==b'\xff\x0d'+counter and raw[6:13]==b'\xa1'+cursor+b'\x89\x45';node=raw[13:14]
    assert raw[14:]==b'\x83\x05'+cursor+b'\x0c'+b'\x8b\x4d'+record,'packed pool prehash decrement/advance differs'
    raw=pe.read(start,24);assert raw[9:12]==b'\x66\x89\x55'and raw[13:15]==b'\x89\x4d'and raw[15:16]==record,'packed pool incoming typed ABI slots differ';type_slot=raw[12:13]
    raw=pe.read(hashed.address+5,end-hashed.address-5);assert raw[:3]==b'\x66\x89\x45';bucket=raw[3:4];heads=linked['_type_hash_buckets'].to_bytes(4,'little');counts=linked['_type_hash_counts'].to_bytes(4,'little')
    expected=(b'\x66\x89\x45'+bucket+b'\x8b\x45'+record+b'\x8b\x4d'+node+b'\x89\x41\x04'+b'\x66\x8b\x45'+type_slot+b'\x8b\x4d'+node+b'\x66\x89\x01'+b'\x0f\xb7\x45'+bucket+b'\x8b\x0d'+heads+b'\x8b\x04\x81\x8b\x4d'+node+b'\x89\x41\x08'+b'\x8b\x45'+node+b'\x0f\xb7\x4d'+bucket+b'\x8b\x15'+heads+b'\x89\x04\x8a'+b'\x0f\xb7\x45'+bucket+b'\x8b\x0d'+counts+b'\x66\xff\x04\x41')
    assert raw[:len(expected)]==expected,'packed pool index/data/next/publication def-use differs'
    return dict(call=call.address,size=12288,node_frame_offset=int.from_bytes(node,'little',signed=True),publication=expected.hex(),rebuilt_sha256=hashlib.sha256(pe.data).hexdigest())

def parse(events):
    rows=[];frontier=None
    for line in events.splitlines():
        parts=line.split()
        if parts and parts[0]=='packed-hash-allocation':
            assert len(parts)==6;rows.append(dict(base=int(parts[1],16),size=int(parts[2]),owner=int(parts[3],16),esp=int(parts[4],16),type=int(parts[5])))
        elif parts and parts[0]=='packed-hash-bytes':
            assert len(parts)==4
            if parts[1]=='birth':assert rows and 'before'not in rows[-1]and rows[-1]['base']==int(parts[2],16);rows[-1]['before']=parts[3]
            else:assert parts[1]=='frontier'and frontier is None;frontier=dict(base=int(parts[2],16),bytes=parts[3])
    return dict(allocations=rows,frontier=frontier)

def compare(config,pe,events,native,old_walks,new_walks,state,case,old_buffer,native_buffer):
    p=validate(config,pe)
    if not native:
        assert not(case=='hello-debug-dll'and native_buffer and native_buffer['frontier']['name']in('convert_procedure32','address_in_module','convert_bp_relative32','convert_register_symbol','end_symbol_block','link_symbol_scopes','record_symbol_offsets')),'missing packed slab observations'
        return
    assert p and not native.get('error')and native['frontier']['name']in('convert_procedure32','address_in_module','convert_bp_relative32','convert_register_symbol','end_symbol_block','link_symbol_scopes','record_symbol_offsets'),'unsupported packed slab frontier'
    if native['frontier']['name']=='address_in_module':assert p['address_frontier']==0x25920,'missing packed module-address scope'
    if native['frontier']['name']=='convert_bp_relative32':assert p['bp_frontier']==0x254f0,'missing BP-relative observed scope'
    original=parse(events);assert len(original['allocations'])==len(native['allocations'])==1 and original['frontier']and native['frontier'],'missing/extra packed slab lifetime'
    assert events.splitlines().count('packed-hash-rearm 00019c3b 00019c3e')==1,'packed pool return probe missing rearm'
    memory={}
    for line in events.splitlines():
        if line.startswith('memory '):
            _,a,raw=line.split();memory.update((int(a,16)+i,v)for i,v in enumerate(bytes.fromhex(raw)))
    def value(address):return word(bytes(memory[address+i]for i in range(4)),0)
    extras=[];logical=[];addresses=[]
    for side,walks,buffer in((original,old_walks,old_buffer),(native,new_walks,native_buffer)):
        a=side['allocations'][0];f=side['frontier'];base=a['base'];before=bytes.fromhex(a['before']);after=bytes.fromhex(f['bytes']);assert base and base%4==0 and a['size']==len(before)==len(after)==12288 and f['base']==base,'packed slab extent differs'
        assert a['type']==0x1049 and a['owner']==buffer['returns'][73]['result'],'packed slab first owner/type differs'
        slots=value(p['counter'])if side is original else f['remaining'];cursor=value(p['cursor'])if side is original else f['cursor'];assert slots==1021 and cursor==base+36,'packed slab prefix/cursor/slots differ'
        if side is native:assert state['globals']['packed_type_slots']==slots.to_bytes(4,'little').hex()and state['globals']['packed_type_cursor']['address']==cursor,'packed slab live globals differ'
        assert all(after[i+2:i+4]==before[i+2:i+4]for i in(0,12,24))and after[36:]==before[36:],'packed slab reserved/unused bytes changed'
        table=walks['local_type_records'][0];records=bytes.fromhex(table['bytes']);nodes=[];owned={}
        for rank,(index,input_type)in enumerate(zip(p['published_types'],p['input_types'])):
            at=rank*12;pointer=base+at;raw=after[at:at+12];copy=buffer['returns'][index-4096];entry=buffer['entries'][index-4096]
            assert word(raw,0,2)==index and word(raw,4)==copy['result']and entry['owner']==table['address']+(input_type-4096)*28,'packed slab node owner/index/data differs'
            record=records[(input_type-4096)*28:(input_type-4096+1)*28];assert word(record,0)==copy['result']and word(record,4,2)==index,'packed slab TypeRecord publication differs'
            matched=[(key,row)for key,row in buffer['hash_nodes'].items()if row['address']==pointer];assert len(matched)==1 and matched[0][1]['bytes']==raw.hex(),'packed slab child not actual published hash node'
            key,row=matched[0];bucket=int(key.split('-')[1]);assert key=='hash-%d-%d'%(bucket,index);owned[pointer]=row
            nodes.append(dict(index=index,input_type=input_type,bucket=bucket,allocation_offset=at))
        assert all(not(base<address+size and address<base+12288)for address,size,identity in namespace(walks)),'packed slab aliases another typed owner'
        a_buffer=buffer['allocations'][0];assert not(base<a_buffer['base']+a_buffer['size']and a_buffer['base']<base+12288),'packed slab aliases copied type payload'
        extras.append(dict(packed_hash_storage=[dict(address=base,bytes=f['bytes'])]));logical.append(nodes);addresses.append(owned)
    assert logical[0]==logical[1],'packed slab typed node inventory differs'
    return dict(nodes=logical[0],defined_ranges=[[at+a,at+b]for at in(0,12,24)for a,b in((0,2),(4,12))],original_namespace=extras[0],native_namespace=extras[1],original_nodes=addresses[0],native_nodes=addresses[1],original=original,native=native)
