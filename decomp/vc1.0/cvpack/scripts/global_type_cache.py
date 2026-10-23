"""Published calloc16 global named-type nodes owned by observed type copies."""
import hashlib,json
from state import BINARY
from objects import namespace

def word(raw,offset,size=4):return int.from_bytes(raw[offset:offset+size],'little')

def validate(config,pe,rebuilt=None,linked=None):
    required=any(r.get('global_type_cache')for r in json.loads((BINARY/'walks.json').read_text()))
    ref=config.get('buffer_storage_observations')
    assert ref or not required,'missing global cache constructor proof'
    if not ref:return
    p=json.loads((BINARY/ref['evidence']).read_text())
    spec=p.get('global_cache')
    assert spec or not required,'missing global cache constructor proof'
    if not spec:return
    assert spec['root']==0x37164 and spec['size']==16 and spec['owner']=='save_type'and spec['original_allocation']==0x19ba4
    assert spec['publication']==[0x19bc0,0x19be1]and spec['input_types']==[4192,4185,4171,4177,4222,4225]
    assert pe.read(0x19ba2,7)==bytes.fromhex('6a10e8b7540000'),'global cache calloc size/callee differs'
    # Whole save_type, including this publication, is already bound in the
    # buffer lifetime proof; retain a second exact site binding here.
    assert pe.read(0x19bc0,33).hex()==spec['publication_bytes'],'global cache original publication differs'
    row=next(r for r in json.loads((BINARY/'walks.json').read_text())if r['name']=='type_work_b')
    assert row['size']==2048 and row.get('pointer_array')and row.get('global_type_cache'),'global cache typed root missing'
    if rebuilt is not None:return compiled(rebuilt,linked)
    return spec


def compiled(pe,linked):
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    decoder=Cs(CS_ARCH_X86,CS_MODE_32);start=linked['@save_type@4'];end=min(v for v in linked.values()if v>start)
    code=list(decoder.disasm(pe.read(start,end-start),start));sites=[j for j,r in enumerate(code)if r.mnemonic=='call'and r.op_str=='0x%x'%linked['_checked_zero']]
    assert len(sites)==1,'global cache allocator identity differs'
    j=sites[0];call=code[j];assert code[j-1].bytes==b'\x6a\x10','global cache allocation size differs'
    raw=pe.read(call.address+5,end-call.address-5)
    assert raw[:5]==b'\x83\xc4\x04\x89\x45','global cache allocation return owner differs'
    slot=raw[5:6];root=linked['_type_work_b'].to_bytes(4,'little')
    pattern=b'\x8b\x4d'+slot+b'\x89\x41\x0c\x8b\x45'+slot
    assert raw.count(pattern)==1,'global cache allocation->next owner differs'
    at=raw.index(pattern);prefix=raw[at-12:at];tail=raw[at+len(pattern):at+len(pattern)+12]
    assert len(prefix)==12 and prefix[:2]==b'\x8b\x45'and prefix[3:5]==b'\x8b\x0d'and prefix[5:9]==root and prefix[9:]==b'\x8b\x04\x81','global cache old-head root differs'
    assert tail[:2]==b'\x8b\x4d'and tail[2]==prefix[2]and tail[3:5]==b'\x8b\x15'and tail[5:9]==root and tail[9:]==b'\x89\x04\x8a','global cache publication owner/root differs'
    return dict(call=call.address,size=16,slot=int.from_bytes(slot,'little',signed=True),publication=raw[at-12:at+len(pattern)+12].hex(),rebuilt_sha256=hashlib.sha256(pe.data).hexdigest())


def compare(left,right,a,b,original,native,context):
    assert context,'missing global type-cache owner/lifetime proof'
    spec=json.loads((BINARY/'cases/abi/buffer-storage.json').read_text())['global_cache']
    copies=context['comparison']['copies'];expected={0x1000+i:copy for i,copy in enumerate(copies)if copy['index']in spec['input_types']}
    results=[]
    for root,walks,extra in((a,original,context['original_namespace']),(b,native,context['native_namespace'])):
        heads=bytes.fromhex(root['bytes']);assert len(heads)==2048
        objects=root.get('objects',{});assert len(objects)==len(expected),'missing/extra global named-cache nodes'
        payload=extra['type_buffer_payload'][0];raw_payload=bytes.fromhex(payload['bytes']);base=payload['address'];seen=set();owned=set();logical=[]
        for bucket in range(512):
            pointer=word(heads,bucket*4);rank=0
            while pointer:
                key='cache-%d-%d'%(bucket,rank);assert key in objects,'missing global cache chain object'
                node=objects[key];raw=bytes.fromhex(node['bytes']);assert len(raw)==16 and node['address']==pointer and pointer not in seen,'global cache alias/extent differs'
                seen.add(pointer);owned.add(key);index=word(raw,8,2)
                assert index in expected and raw[10:12]==b'\0\0','global cache input/index/initialized flags differ'
                copy=expected[index];start=copy['offset'];record=bytes.fromhex(copy['bytes'])
                assert spec['input_name_modes'][str(copy['index'])]==dict(kind=5,name_offset=16,size=copy['size'])and word(record,2,2)==5 and word(record,14,2)<0x8000,'unreviewed global cache record/name mode'
                name=start+16;assert word(raw,4)==base+start and word(raw,0)==base+name,'global cache pointers do not own the observed copied record'
                text=raw_payload[name:name+1+raw_payload[name]];assert len(text)==record[16]+1 and text==record[16:17+record[16]]and 17+record[16]<=copy['size']and bucket==sum(text)&511,'global cache Pascal name/bucket differs'
                assert all(not(pointer<address+size and address<pointer+16)for address,size,identity in namespace(walks)+namespace(extra)if identity!=('type_work_b',0,'object',key)),'global cache allocation overlaps another owner'
                logical.append(dict(key=key,bucket=bucket,input_type=copy['index'],index=index,record_offset=start,name=text.hex(),next_key='cache-%d-%d'%(bucket,rank+1)if word(raw,12)else None))
                pointer=word(raw,12);rank+=1
        assert owned==set(objects),'unowned global cache object'
        results.append(logical)
    assert results[0]==results[1],'global named-cache state differs'
    return results[0]
