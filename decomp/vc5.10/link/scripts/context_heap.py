"""Typed ownership proof for the two newly initialized help-case string tables."""
import json
import struct
from member_pointers import CONTRACTS,project


def words(data):return struct.unpack('<%dI'%(len(data)//4),data)


def compare(original,state,trap,configuration_context=None):
    context=trap['stack'][1] if configuration_context is None else configuration_context
    ncontext=state['context_this']
    assert context and ncontext==state['caller_context'] and ncontext
    if configuration_context is None:assert trap['stack'][2:4]==[0,0]
    assert state['configuration_arguments']==dict(context=ncontext,first=0,second=0)
    assert state['context_vxd']==0 and state['context_extension']=='.dll'
    data=bytearray((original/'link-context.bin').read_bytes());native=bytearray.fromhex(state['link_context'])
    assert len(data)==len(native)==664
    roles={0x24:(context,ncontext),0x168:(0x402228,int.from_bytes(native[0x168:0x16c],'little')),
           0x1b8:(context+0x1b4,ncontext+0x1b4),0x1c0:(context+0x1bc,ncontext+0x1bc),
           0x1d8:(0x407058,state['default_dos_image_address'])}
    aux=struct.unpack_from('<I',data,0x260)[0];naux=state['context_auxiliary_address']
    assert aux and naux
    assert (original/'context-auxiliary.bin').read_bytes()==bytes.fromhex(state['context_auxiliary'])==bytes(44)
    roles[0x260]=(aux,naux)
    original_ranges=[(context,664),(aux,44)];native_ranges=[(ncontext,664),(naux,44)]
    expected_virtual=[];original_bases=[]
    for index,(buckets,capacity) in enumerate([(1024,128),(128,32)]):
        table=words((original/('table-%d.bin'%index)).read_bytes());nrecord=state['context_heap'][index]
        ntable=words(bytes.fromhex(nrecord['table_raw']))
        pointer=struct.unpack_from('<I',data,0x1d0+index*4)[0];npointer=nrecord['table']
        assert pointer and npointer
        assert table[0] and ntable[0]==nrecord['hash'] and ntable[0]
        assert table[1:9]==ntable[1:9]==(0,)*8
        assert table[9]==pointer+32 and ntable[9]==npointer+32
        roles[0x1d0+index*4]=(pointer,npointer)
        original_ranges.extend([(pointer,40),(table[0],56)]);native_ranges.extend([(npointer,40),(ntable[0],56)])
        header=words((original/('hash-%d.bin'%index)).read_bytes());nheader=words(bytes.fromhex(nrecord['hash_raw']))
        assert len(header)==len(nheader)==14
        assert header[:8]==nheader[:8]==(0,buckets,buckets,0,0,capacity,buckets,0)
        assert header[8]==0x4622e0 and nheader[8]==state['string_key_name_address']
        assert header[9]==nheader[9]==header[13]==nheader[13]==0
        page_bases=[];npage_bases=[];pages=[];npages=[]
        for kind,(used,reserved) in enumerate([(capacity*4+4,4096),(buckets*4,(buckets*4//4096+1)*4096)]):
            arena=words((original/('arena-%d-%d.bin'%(index,kind))).read_bytes());nentry=nrecord['arenas'][kind]
            narena=words(bytes.fromhex(nentry['raw']))
            assert header[11+kind] and nheader[11+kind]==nentry['address'] and nentry['address']
            assert arena[:3]==narena[:3]==(0,used,reserved-used)
            assert arena[3] and narena[3]
            page=(original/('arena-data-%d-%d.bin'%(index,kind))).read_bytes();npage=bytes.fromhex(nentry['data'])
            assert len(page)==len(npage)==reserved
            original_ranges.extend([(header[11+kind],16),(arena[3],reserved)])
            native_ranges.extend([(nentry['address'],16),(narena[3],reserved)])
            page_bases.append(arena[3]);npage_bases.append(narena[3]);pages.append(page);npages.append(npage)
            expected_virtual.append(dict(arguments=[0,reserved,0x3000,4],result=narena[3],initial=bytes(reserved).hex()))
            original_bases.append(arena[3])
        assert header[10]==page_bases[0] and nheader[10]==npage_bases[0]
        original_directory=bytearray(pages[0]);native_directory=bytearray(npages[0]);cell=capacity*4
        for offset,expected,nexpected in [(0,page_bases[0]+cell,npage_bases[0]+cell),(cell,page_bases[1],npage_bases[1])]:
            assert struct.unpack_from('<I',original_directory,offset)[0]==expected
            assert struct.unpack_from('<I',native_directory,offset)[0]==nexpected
            original_directory[offset:offset+4]=native_directory[offset:offset+4]=bytes(4)
        assert original_directory==native_directory==bytes(4096)
        assert pages[1]==npages[1]==bytes(len(pages[1]))
    for offset,(expected,nexpected) in roles.items():
        assert expected and nexpected
        assert struct.unpack_from('<I',data,offset)[0]==expected
        assert struct.unpack_from('<I',native,offset)[0]==nexpected
        data[offset:offset+4]=native[offset:offset+4]=bytes(4)
    assert data==native,'context differs outside its explicit ownership roles'
    for ranges in [original_ranges,native_ranges]:
        ordered=sorted(ranges)
        assert all(a+size<=b for (a,size),(b,_) in zip(ordered,ordered[1:])),'overlapping live allocations'
    from binary import PE
    assert bytes.fromhex(state['default_dos_image'])==PE(original/'module-000.bin').read(0x407058,128)
    assert state['virtual_alloc_calls']==expected_virtual
    events=[json.loads(line) for line in (original/'events.jsonl').read_text().splitlines()]
    observations=[e for e in events if e['event']=='virtual_observation']
    assert len(observations)==2
    assert [e['site'] for e in observations]==[0x44f659,0x44f65f]
    assert all(e['arguments']==[0,4096,0x3000,4] for e in observations)
    assert observations[1]['eax']==original_bases[0]
    assert (original/'virtual-first-zero.bin').read_bytes()==bytes(4096)
    for member,contract in CONTRACTS.items():
        filename={'image_header_size':'member-header-size.bin','image_write_section':'member-write-section.bin','image_write_headers':'member-write-headers.bin'}[member]
        assert state['member_pointers'][member]['bytes']==8
        assert project((original/filename).read_bytes(),bytes.fromhex(state['member_pointers'][member]['raw']),'pe',contract,state['member_methods'])==contract['methods']['pe']
