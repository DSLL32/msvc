"""Finite resource-record pointer model used by debugger observations only."""
import re
import struct

ROOTS = [('resource_strings',1),('resource_files',3),('resource_id_types',2),('resource_named_types',2)]

def links(kind, raw):
    u=lambda offset:struct.unpack_from('<I',raw,offset)[0]
    if kind==1:return [(1,u(4),4)] if u(0)==1 else []
    if kind==2:return [(2,u(0),0),(1,u(4),4),(3,u(12),12),(3,u(16),16)]
    if kind==3:return [(3,u(0),0),(1,u(4),4),(1,u(12),12),(3,u(16),16),(4,u(20),20)]
    assert kind==4
    return []

def capture(read, roots, directory_pointer, directory_size):
    queue=[];seen=set();nodes=[]
    def add(kind,pointer):
        if pointer and (kind,pointer) not in seen:
            seen.add((kind,pointer));queue.append((kind,pointer))
            assert len(queue)<500,'resource-state bound exceeded'
    for name,kind in ROOTS:add(kind,roots[name])
    for kind,pointer in queue:
        raw=read(pointer,{1:20,2:24,3:32,4:24}[kind])
        if kind==1 and struct.unpack_from('<I',raw)[0]==1:
            length=struct.unpack_from('<H',raw,14)[0];assert length<256
            raw=read(pointer,20+2*length)
        nodes.append(dict(kind=kind,pointer=pointer,raw_hex=raw.hex()))
        for child_kind,child,_ in links(kind,raw):add(child_kind,child)
    assert 0<=directory_size<=65536 and bool(directory_pointer)==bool(directory_size)
    return dict(roots=roots,nodes=nodes,directory_pointer=directory_pointer,directory_size=directory_size,
                directory_hex=read(directory_pointer,directory_size).hex() if directory_pointer else '')

def original(text,prefix=''):
    p=re.escape(prefix)
    roots={name:int(pointer,16) for name,kind,pointer in re.findall(r'^'+p+r'heap-root (\w+) ([0-9]+) ([0-9a-f]+)$',text,re.M)}
    assert set(roots)=={name for name,_ in ROOTS},'missing resource heap roots'
    nodes=[dict(kind=int(kind),pointer=int(pointer,16),raw_hex=raw) for kind,pointer,raw in re.findall(r'^'+p+r'heap-node ([0-9]+) ([0-9a-f]+) ([0-9a-f]+)$',text,re.M)]
    count=re.search(r'^'+p+r'heap-count ([0-9]+) ([0-9]+)$',text,re.M)
    assert count and int(count[1])==int(count[2])==len(nodes)<500,'incomplete resource heap closure'
    directory=re.search(r'^'+p+r'heap-directory ([0-9a-f]+) ([0-9]+) ([0-9a-f]*)$',text,re.M);assert directory
    return dict(roots=roots,nodes=nodes,directory_pointer=int(directory[1],16),directory_size=int(directory[2]),directory_hex=directory[3])

def canonical(record,with_ids=False):
    nodes={(n['kind'],n['pointer']):bytes.fromhex(n['raw_hex']) for n in record['nodes']}
    assert len(nodes)==len(record['nodes'])<500,'duplicate resource nodes'
    identifiers={};order=[]
    def identity(kind,pointer):
        if not pointer:return 0
        key=(kind,pointer);assert key in nodes,'resource pointer missing from closure'
        if key not in identifiers:identifiers[key]=len(identifiers)+1;order.append(key)
        return identifiers[key]
    roots=[identity(kind,record['roots'][name]) for name,kind in ROOTS]
    result=[];named_unions=[]
    for key in order:
        kind,pointer=key;raw=bytearray(nodes[key]);u=lambda offset:struct.unpack_from('<I',raw,offset)[0]
        if kind==1:
            assert u(0) in (1,2),'invalid identifier tag'
            if u(0)==1:
                length=struct.unpack_from('<H',raw,14)[0]
                assert len(raw)==20+2*length and length<256
                assert raw[16+2*length:]==b'\0'*4,'identifier termination differs'
                named_unions.append(struct.unpack_from('<H',raw,4)[0])
            else:assert len(raw)==20
        else:assert len(raw)=={2:24,3:32,4:24}[kind]
        for child_kind,child,offset in links(kind,raw):struct.pack_into('<I',raw,offset,identity(child_kind,child))
        result.append((kind,raw.hex()))
    assert len(order)==len(nodes),'extraneous/unreachable resource node'
    size=record['directory_size'];data=bytes.fromhex(record['directory_hex'])
    assert len(data)==size and bool(record['directory_pointer'])==bool(size),'incomplete directory buffer'
    # The target compares a named identifier union's low WORD before wcscmp.
    # Preserve those equality decisions without equating process addresses.
    low_word_equal=[[left==right for right in named_unions] for left in named_unions]
    result=dict(roots=roots,nodes=result,named_union_equalities=low_word_equal,directory_hex=data.hex())
    return (result,identifiers) if with_ids else result
