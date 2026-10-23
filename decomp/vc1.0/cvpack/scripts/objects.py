"""Compare complete observed pool bytes with explicit typed pointer fields."""
import hashlib

def namespace(walks):
    result=[]
    for name,nodes in walks.items():
        for index,node in enumerate(nodes):
            result.append((node['address'],len(bytes.fromhex(node['bytes'])),(name,index)))
            for field,kind in (('strings','string'),('arrays','array')):
                for offset,pointee in node.get(field,{}).items():
                    if pointee['address']:result.append((pointee['address'],len(bytes.fromhex(pointee['bytes'])),(name,index,kind,int(offset))))
    return result

def pointer_identity(walks,value):
    if not value:return None
    matches=[(identity,value-address)for address,size,identity in namespace(walks)if address<=value<address+size or (size==0 and value==address)]
    assert len(matches)==1,('unresolved/ambiguous observed argument pointer',hex(value),matches)
    return matches[0]

def original_walks(events,specifications):
    by_root={item['original']:item for item in specifications}
    walks={item['name']:[] for item in specifications}
    ended=set()
    for line in events.splitlines():
        if line.startswith('node '):
            fields=line.split();_,root,index,address=fields[:4];raw=fields[4]if len(fields)==5 else ''
            item=by_root[int(root,16)]
            nodes=walks[item['name']]
            assert int(index)==len(nodes)
            nodes.append(dict(address=int(address,16),bytes=raw))
        elif line.startswith(('node-string ','node-array ')):
            fields=line.split()
            kind,root,index,offset,address=fields[:5]
            raw=fields[5]if len(fields)==6 else ''
            node=walks[by_root[int(root,16)]['name']][int(index)]
            pointees=node.setdefault('strings'if kind=='node-string'else 'arrays',{})
            assert offset not in pointees,'duplicate object pointee'
            pointees[offset]=dict(address=int(address,16),bytes=raw)
        elif line.startswith('walk-end '):
            _,root,count,tail=line.split()
            item=by_root[int(root,16)]
            assert int(tail,16)==0,'unterminated original object list'
            assert int(count)==len(walks[item['name']])
            ended.add(item['name'])
        elif line.startswith('node-fd '):
            _,root,index,offset,fd,flags,type,position,owned=line.split()
            node=walks[by_root[int(root,16)]['name']][int(index)]
            descriptors=node.setdefault('descriptors',{})
            assert offset not in descriptors
            descriptors[offset]=dict(descriptor=int(fd),osfile=int(flags,16),type=int(type),position=int(position),owned=bool(int(owned)))
    assert ended==set(walks),'missing original object walk'
    return walks

def compare_walks(specifications,original,native,definitions=None):
    definitions=definitions or {}
    assert set(original)==set(native)=={item['name']for item in specifications}
    original_names,native_names=namespace(original),namespace(native)
    def logical(value,namespace):
        if not value:return None
        candidates=[(identity,value-address)for address,size,identity in namespace if address<=value<address+size or (size==0 and value==address)]
        assert len(candidates)==1,('unresolved/ambiguous observed pointer',hex(value),candidates)
        return candidates[0]
    receipt={}
    for item in specifications:
        name=item['name']
        assert len(original[name])==len(native[name]),('object count differs',name)
        if item.get('allocation_name'):assert len(original[name])==len(native[name])==1,'missing live scratch allocation'
        nodes=[]
        for index,(a,b)in enumerate(zip(original[name],native[name])):
            left,right=bytearray.fromhex(a['bytes']),bytearray.fromhex(b['bytes'])
            assert len(left)==len(right)
            guard=item.get('mode_guard')
            if guard:
                for raw in (left,right):
                    flags=int.from_bytes(raw[guard['offset']:guard['offset']+4],'little')
                    assert flags&guard['mask']==guard['value'],('unobserved object union mode',name,index,flags)
            if item.get('file_descriptor'):
                old,new=a.get('descriptors',{}),b.get('descriptors',{})
                fd=int.from_bytes(left[16:20],'little')
                expected={'16'}if fd>=3 and fd!=0xffffffff else set()
                assert set(old)==set(new)==expected,'missing object descriptor proof'
                assert old==new,('object descriptor state differs',name,index,old,new)
                for descriptor in old.values():
                    if descriptor['osfile']&1:assert descriptor['type']==1 and descriptor['owned'],'object descriptor does not own approved disk input'
            for offset in item.get('string_fields',[]):
                old,new=a['strings'][str(offset)],b['strings'][str(offset)]
                assert old['bytes']==new['bytes'],('object string differs',name,index,offset)
                for raw,text in ((left,old),(right,new)):
                    assert int.from_bytes(raw[offset:offset+4],'little')==text['address']
                    assert (text['bytes'].endswith('00')and len(bytes.fromhex(text['bytes']))<=1024)if text['address']else not text['bytes']
            arrays={}
            for description in item.get('arrays',[]):
                when=description['when'];flags=int.from_bytes(left[when['offset']:when['offset']+4],'little')
                if flags&when['mask']!=when['value']:continue
                offset=description['offset']
                old,new=a['arrays'][str(offset)],b['arrays'][str(offset)]
                assert len(old['bytes'])==len(new['bytes'])
                entries=[]
                for raw,payload in ((left,old),(right,new)):
                    assert int.from_bytes(raw[offset:offset+4],'little')==payload['address']
                    count=description['fixed_count']if 'fixed_count'in description else int.from_bytes(raw[description['count_offset']:description['count_offset']+4],'little')>>description['count_shift']
                    assert count<=description['max_count']
                    assert len(bytes.fromhex(payload['bytes']))==(count*description.get('element_size',4)if payload['address']else 0)
                if not description.get('pointers',True):
                    if description.get('allocation_name'):
                        from allocations import mask_undefined
                        observed_left,observed_right=bytearray.fromhex(old['bytes']),bytearray.fromhex(new['bytes'])
                        mask_undefined(description['allocation_name'],observed_left,observed_right,definitions)
                        arrays[str(offset)]=dict(allocation=description['allocation_name'],bytes=len(observed_left),raw_original_sha256=hashlib.sha256(bytes.fromhex(old['bytes'])).hexdigest(),raw_native_sha256=hashlib.sha256(bytes.fromhex(new['bytes'])).hexdigest());continue
                    assert old['bytes']==new['bytes'],('binary array contents differ',name,index,offset)
                    arrays[str(offset)]=dict(bytes=len(bytes.fromhex(old['bytes'])),sha256=hashlib.sha256(bytes.fromhex(old['bytes'])).hexdigest())
                    continue
                assert description.get('element_size',4)==4
                for entry in range(0,len(old['bytes'])//2,4):
                    values=[int.from_bytes(bytes.fromhex(payload['bytes'])[entry:entry+4],'little')for payload in (old,new)]
                    identities=[('sentinel',value)if value in description.get('sentinels',[])else logical(value,space)for value,space in zip(values,(original_names,native_names))]
                    assert identities[0]==identities[1],('array pointer differs',name,index,offset,entry,identities)
                    entries.append(identities[0])
                arrays[str(offset)]=entries
            fields=list(range(0,len(left),4)) if item.get('pointer_array') else item.get('pointer_fields',[])
            if item.get('pointer_stride'):
                stride=item['pointer_stride'];fields=list(range(stride['start'],len(left),stride['stride']))
            pointers={}
            for offset in fields:
                old=int.from_bytes(left[offset:offset+4],'little')
                new=int.from_bytes(right[offset:offset+4],'little')
                old_id,new_id=logical(old,original_names),logical(new,native_names)
                assert old_id==new_id,('pointer identity differs',name,index,offset,old_id,new_id)
                pointers[str(offset)]=old_id
                left[offset:offset+4]=right[offset:offset+4]=b'\0'*4
            allocation=None
            if item.get('allocation_name'):
                from allocations import mask_undefined
                allocation=mask_undefined(item['allocation_name'],left,right,definitions)
            assert left==right,('observed object bytes differ',name,index)
            nodes.append(dict(bytes=len(left),nonpointer_sha256=hashlib.sha256(left).hexdigest(),pointers=pointers,arrays=arrays,descriptors=a.get('descriptors',{}),allocation=allocation))
        receipt[name]=nodes
    return receipt
