"""Compare complete observed pool bytes with explicit typed pointer fields."""
import hashlib

def pointer_identity(walks,value):
    if not value:return None
    matches=[((name,index),value-node['address'])
             for name,nodes in walks.items() for index,node in enumerate(nodes)
             if node['address']<=value<node['address']+len(bytes.fromhex(node['bytes']))]
    assert len(matches)==1,('unresolved/ambiguous observed argument pointer',hex(value),matches)
    return matches[0]

def original_walks(events,specifications):
    by_root={item['original']:item for item in specifications}
    walks={item['name']:[] for item in specifications}
    ended=set()
    for line in events.splitlines():
        if line.startswith('node '):
            _,root,index,address,raw=line.split()
            item=by_root[int(root,16)]
            nodes=walks[item['name']]
            assert int(index)==len(nodes)
            nodes.append(dict(address=int(address,16),bytes=raw))
        elif line.startswith('walk-end '):
            _,root,count,tail=line.split()
            item=by_root[int(root,16)]
            assert int(tail,16)==0,'unterminated original object list'
            assert int(count)==len(walks[item['name']])
            ended.add(item['name'])
    assert ended==set(walks),'missing original object walk'
    return walks

def compare_walks(specifications,original,native):
    assert set(original)==set(native)=={item['name']for item in specifications}
    def namespace(walks):
        return [(node['address'],len(bytes.fromhex(node['bytes'])),(name,index))
                for name,nodes in walks.items() for index,node in enumerate(nodes)]
    original_names,native_names=namespace(original),namespace(native)
    def logical(value,namespace):
        if not value:return None
        candidates=[(identity,value-address)for address,size,identity in namespace if address<=value<address+size]
        assert len(candidates)==1,('unresolved/ambiguous observed pointer',hex(value),candidates)
        return candidates[0]
    receipt={}
    for item in specifications:
        name=item['name']
        assert len(original[name])==len(native[name]),('object count differs',name)
        nodes=[]
        for index,(a,b)in enumerate(zip(original[name],native[name])):
            left,right=bytearray.fromhex(a['bytes']),bytearray.fromhex(b['bytes'])
            assert len(left)==len(right)
            fields=list(range(0,len(left),4)) if item.get('pointer_array') else item.get('pointer_fields',[])
            pointers={}
            for offset in fields:
                old=int.from_bytes(left[offset:offset+4],'little')
                new=int.from_bytes(right[offset:offset+4],'little')
                old_id,new_id=logical(old,original_names),logical(new,native_names)
                assert old_id==new_id,('pointer identity differs',name,index,offset,old_id,new_id)
                pointers[str(offset)]=old_id
                left[offset:offset+4]=right[offset:offset+4]=b'\0'*4
            assert left==right,('observed object bytes differ',name,index)
            nodes.append(dict(bytes=len(left),nonpointer_sha256=hashlib.sha256(left).hexdigest(),pointers=pointers))
        receipt[name]=nodes
    return receipt
