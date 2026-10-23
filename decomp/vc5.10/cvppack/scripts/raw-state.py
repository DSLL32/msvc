"""Pure original-event decoder and typed application-state comparison."""
import json
import re
import struct
import hashlib


def decode(events):
    lines = events.splitlines()
    frontiers = [(index,int(line.split()[1],16)) for index,line in enumerate(lines)
                 if re.fullmatch(r'frontier [0-9a-f]{8}',line)]
    assert len(frontiers) == 1, 'expected one raw original frontier'
    index,address = frontiers[0]
    memory,strings = {},{}
    file_calls=[];all_strings={};last_error=None;handles={};mapping_calls=[];mapped_views={};file_reads=[];directory_allocation={};subsection_allocations=[];image_allocations=[];checked_allocations=[];module_allocations=[];cleared_allocations=[]
    for line in lines:
        if line.startswith('module-allocation-call '):
            _,entry,size=line.split();module_allocations.append(dict(entry=int(entry,16),size=int(size)))
        elif line.startswith('module-allocation-return '):
            assert module_allocations and 'address' not in module_allocations[-1]
            module_allocations[-1]['address']=int(line.split()[1],16)
        elif line.startswith('cleared-allocation-call '):
            cleared_allocations.append(dict(size=int(line.split()[1])))
        elif line.startswith('cleared-allocation-return '):
            assert cleared_allocations and 'address' not in cleared_allocations[-1]
            cleared_allocations[-1]['address']=int(line.split()[1],16)
        elif line.startswith('checked-allocation-call '):
            checked_allocations.append(dict(size=int(line.split()[1])))
        elif line.startswith('checked-allocation-return '):
            assert checked_allocations and 'address' not in checked_allocations[-1]
            checked_allocations[-1]['address']=int(line.split()[1],16)
        elif line.startswith('image-allocation-call '):
            _,site,size=line.split();image_allocations.append(dict(site=int(site,16),size=int(size)))
        elif line.startswith('image-allocation-return '):
            assert image_allocations and 'address' not in image_allocations[-1]
            image_allocations[-1]['address']=int(line.split()[1],16)
        elif line.startswith('subsection-allocation-call '):
            _,entry,size=line.split();subsection_allocations.append(dict(entry=int(entry,16),size=int(size)))
        elif line.startswith('subsection-allocation-return '):
            assert subsection_allocations and 'address' not in subsection_allocations[-1]
            subsection_allocations[-1]['address']=int(line.split()[1],16)
        elif line.startswith('directory-allocation-call '):
            assert not directory_allocation,'directory allocation generation needs an explicit model'
            directory_allocation['size']=int(line.split()[1])
        elif line.startswith('directory-allocation-return '):
            directory_allocation['address']=int(line.split()[1],16)
        elif line.startswith(('blob-observation-failed','public-node-observation-failed')):raise AssertionError('incomplete initialized allocation observation')
        elif line.startswith('file-read-call '):
            _,file,buffer,size,pc,position=line.split()
            file_reads.append(dict(file=int(file),buffer=int(buffer,16),size=int(size),pc=int(pc,16),entry_position=int(position),entry_last_error=last_error))
        elif line.startswith('file-read-return '):
            assert file_reads and 'returned' not in file_reads[-1]
            _,returned,position=line.split()
            file_reads[-1].update(returned=int(returned),exit_position=int(position),exit_last_error=last_error)
        elif line.startswith('file-read-bytes '):
            assert file_reads and 'returned' in file_reads[-1]
            file_reads[-1]['bytes']=line.split(' ',1)[1]
            assert len(bytes.fromhex(file_reads[-1]['bytes']))==file_reads[-1]['returned']
        elif line in ('file-read-failed','file-read-overlap'):raise AssertionError('incomplete actual read observation')
        elif line.startswith(('mapping-create-arguments ','view-map-arguments ')):
            fields=line.split();args=[int(value,16) for value in fields[1:]];assert len(args)==6
            mapping_calls.append(dict(kind='create' if fields[0]=='mapping-create-arguments' else 'map',args=args))
        elif line.startswith('mapping-close-argument '):
            mapping_calls.append(dict(kind='close',args=[int(line.split()[1],16)]))
        elif line.startswith(('mapping-create-result ','view-map-result ','mapping-close-result ')):
            assert mapping_calls and 'result' not in mapping_calls[-1]
            mapping_calls[-1].update(result=int(line.split()[1],16),last_error=last_error)
        elif line.startswith('file-open-arguments '):
            args=[int(value,16) for value in line.split()[1:]];assert len(args)==7
            file_calls.append(dict(pointer=args[0],**dict(zip(('access','share','security','disposition','attributes','template'),args[1:]))))
        elif line.startswith('file-open-result '):
            assert file_calls and 'handle' not in file_calls[-1]
            file_calls[-1].update(handle=int(line.split()[1],16),last_error=last_error)
        elif line.startswith('last-error '):last_error=int(line.split()[1])
        elif line.startswith('string '):
            _,location,raw=line.split()
            assert raw not in ('FAILED','UNBOUNDED')
            all_strings[int(location,16)]=raw
    for call in file_calls:call['pathname']=all_strings[call.pop('pointer')]

    type_compare_calls=[]
    for line in lines:
        if line.startswith('type-compare-failed'):raise AssertionError('incomplete recursive comparison observation')
        if line.startswith('type-compare-call '):
            _,record,offset,size,left,right=line.split()
            size=int(size)
            assert left!='FAILED' and right!='FAILED','unobserved recursive comparison bytes'
            assert len(bytes.fromhex(left))==len(bytes.fromhex(right))==size,'truncated recursive comparison bytes'
            type_compare_calls.append(dict(record=int(record,16),offset=int(offset),size=size,left=left,right=right))
        elif line.startswith('type-compare-return '):
            assert type_compare_calls and 'different' not in type_compare_calls[-1],'recursive comparison return inventory differs'
            type_compare_calls[-1]['different']=bool(int(line.split()[1]))
    lookup_calls=[];lookup_stack=[]
    for line in lines:
        if line.startswith('lookup-type-failed'):raise AssertionError('incomplete global-type lookup observation')
        if line.startswith('lookup-type-entry '):
            _,depth,argument,output,stack=line.split();depth=int(depth);output=int(output,16);stack=int(stack,16)
            assert depth==len(lookup_stack),'lookup entry nesting differs'
            row=dict(depth=depth,index=int(argument),parent_argument_alias=bool(lookup_stack and output==lookup_stack[-1]['stack']+8),stack=stack)
            if depth:assert row['parent_argument_alias'],'original recursive lookup output does not alias parent argument'
            lookup_calls.append(row);lookup_stack.append(row)
        elif line.startswith('lookup-type-return '):
            _,depth,result,output=line.split();depth=int(depth)
            assert lookup_stack and depth==len(lookup_stack)-1,'lookup return nesting differs'
            lookup_stack.pop().update(result=int(result,16),output=int(output,16))

    registers = None
    for line in lines[index+1:]:
        if line.startswith('registers '):
            registers = [int(value,16) for value in line.split()[1:]]
        elif line.startswith('memory '):
            _,location,raw = line.split()
            memory[int(location,16)] = bytes.fromhex(raw)
        elif line.startswith('mapped-view '):
            _,view,length,base,region,protect,committed,kind,ordinal=line.split()
            mapped_views[int(view,16)]=dict(length=int(length),base=int(base,16),region=int(region),
                protection=int(protect),state=int(committed),type=int(kind),file='events-view-%d.bin'%int(ordinal))
        elif line.startswith('mapped-view-failed ') or line.startswith('file-handle-failed '):
            raise AssertionError('incomplete original typed resource observation')
        elif line.startswith('file-handle '):
            _,handle,kind,size,high,position=line.split()
            handles[int(handle,16)]=dict(type=int(kind),size=int(size)+(int(high)<<32),position=int(position))
        elif line.startswith('string '):
            _,location,raw = line.split()
            assert raw not in ('FAILED','UNBOUNDED'), 'unreadable string argument'
            value = bytes.fromhex(raw)
            assert value[-1:] == b'\0' and b'\0' not in value[:-1]
            strings[int(location,16)] = value
    assert registers and len(registers) == 8
    stack = memory[registers[-1]]
    assert len(stack) == 128
    return dict(address=address,memory=memory,strings=strings,lookup_calls=lookup_calls,lookup_active_depth=len(lookup_stack),type_compare_calls=type_compare_calls,
                registers=dict(zip(('eax','ebx','ecx','edx','esi','edi','ebp','esp'),registers)),esp=registers[-1],stack=struct.unpack('<32I',stack),file_calls=file_calls,file_handles=handles,last_error=last_error,mapping_calls=mapping_calls,mapped_views=mapped_views,file_reads=file_reads,directory_allocation=directory_allocation,subsection_allocations=subsection_allocations,image_allocations=image_allocations,checked_allocations=checked_allocations,module_allocations=module_allocations,cleared_allocations=cleared_allocations)


def compare(config,data,events,native,artifacts=None,views=None):
    original = decode(events)
    record = next(r for r in config['functions'] if r['original']==original['address'])
    assert native['function'] == record['native_symbol']
    parameters = {};object_parameters=[];arena_parameters=[]
    assert 'parameters' in record, 'frontier lacks explicit ABI/parameter schema'
    if config.get('require_native_argument_inventory'):
        actual=native['observed_argument_names'];expected=[item['name'] for item in record['parameters']]
        assert len(actual)==len(expected) and set(actual)==set(expected), 'actual frontier formal-argument inventory differs'
        types=native['observed_argument_types']
        assert len(types)==len(expected) and {item['name'] for item in types}==set(expected), 'actual frontier formal-type inventory differs'
        for item in record['parameters']:
            actual=next(row for row in types if row['name']==item['name'])
            integer=item['kind']=='integer'
            assert actual['kind']==('integer' if integer else 'pointer'), 'actual frontier formal type differs'
            assert actual['width']==(item.get('native_width',4) if integer else 4), 'actual frontier formal width differs'
            required=item.get('width') if item['kind']=='caller-local-pointer' else item.get('element_size') if item['kind']=='blob-element-pointer' else 4 if item['kind']=='integer-pointer' else item.get('native_pointee_width')
            if required is not None:assert actual.get('pointee_width')==required, 'actual frontier pointee extent differs'

    for item in record['parameters']:
        value = original['registers'][item['register']] if 'register' in item else original['stack'][item['slot']]
        if item['kind'] == 'integer':
            width=item.get('native_width',4)
            assert width in (1,2,4),'unsupported integer ABI width'
            parameters[item['name']] = value & ((1 << (width*8))-1)
        elif item['kind'] == 'string':
            parameters[item['name']] = dict(null=value==0,bytes=original['strings'][value].hex() if value else None)
        elif item['kind'] == 'argv':
            count = original['stack'][item['slot']-1]
            assert count < 64
            array = original['memory'][value]
            pointers = struct.unpack('<%dI'%(count+1),array)
            assert pointers[-1] == 0
            parameters[item['name']] = [original['strings'][pointer].hex() for pointer in pointers[:-1]]
        elif item['kind'] == 'integer-pointer':
            parameters[item['name']]=dict(null=value==0,bytes=original['memory'][value].hex() if value else None)
            assert not value or len(original['memory'][value])==4
        elif item['kind'] == 'global-pointer':
            if not value:
                assert item.get('nullable');parameters[item['name']]=dict(null=True)
            else:
                names=[row['name'] for row in data if row['name'] in item['allowed_globals'] and (int.from_bytes(original['memory'][row['original']],'little') if item.get('global_value') else row['original'])==value]
                assert len(names)==1,'argument is not the specified original global address'
                parameters[item['name']]=dict(null=False,global_name=names[0])
                if 'buffer_count_slot' in item:
                    size=original['stack'][item['buffer_count_slot']];row=next(row for row in data if row['name']==item['capacity_global'])
                    capacity=int.from_bytes(original['memory'][row['original']],'little');pointer=int.from_bytes(original['memory'][value],'little')
                    assert 0<=size<=capacity and len(original['memory'][pointer])==size
                    parameters[item['name']].update(size=size,capacity=capacity,bytes=original['memory'][pointer].hex())
        elif item['kind']=='global-buffer':
            names=[row['name'] for row in data if row['name'] in item['allowed_globals'] and int.from_bytes(original['memory'][row['original']],'little')==value]
            assert len(names)==1,'buffer argument does not alias specified original global'
            if 'count_slot' in item:size=original['stack'][item['count_slot']]
            else:
                row=next(row for row in data if row['name']=='directory_entries');root=int.from_bytes(original['memory'][row['original']],'little');entry=original['stack'][item['entry_slot']]
                assert entry>=root and (entry-root)%12==0
                payload=original['memory'][root][entry-root:entry-root+12];assert len(payload)==12
                size=int.from_bytes(payload[item['count_offset']:item['count_offset']+4],'little')
            assert 0<=size<=65536
            assert len(original['memory'][value])==size
            parameters[item['name']]=dict(global_name=names[0],size=size,bytes=original['memory'][value].hex())
        elif item['kind']=='blob-element-pointer':
            row=next(row for row in data if row['name']==item['root']);root=int.from_bytes(original['memory'][row['original']],'little')
            row=next(row for row in data if row['name']==item['count']);count=int.from_bytes(original['memory'][row['original']],'little')
            assert root<=value<root+count*item['element_size'] and (value-root)%item['element_size']==0,'argument is not an original directory element'
            offset=value-root;payload=original['memory'][root][offset:offset+item['element_size']]
            assert len(payload)==item['element_size']
            parameters[item['name']]=dict(root=item['root'],index=offset//item['element_size'],bytes=payload.hex())
        elif item['kind']=='type-record-pointer':
            row=next(row for row in data if row['name']==item['root']);root=int.from_bytes(original['memory'][row['original']],'little')
            row=next(row for row in data if row['name']==item['count']);count=int.from_bytes(original['memory'][row['original']],'little')
            assert root<=value<root+count*32 and (value-root)%32==0,'argument is not an owned original local type record'
            parameters[item['name']]=dict(root=item['root'],index=(value-root)//32)
        elif item['kind']=='current-symbol-record':
            def observed(name):
                row=next(row for row in data if row['name']==name)
                return int.from_bytes(original['memory'][row['original']],'little')
            base=observed(item['root']);capacity=observed(item['capacity']);offset=value-base
            assert value==observed(item['cursor']) and 0<=offset<=capacity-4,'symbol record does not alias active owned input cursor'
            buffer=original['memory'][base];length=int.from_bytes(buffer[offset:offset+2],'little')+2
            assert len(buffer)==capacity and 4<=length<=capacity-offset,'symbol record exceeds observed input capacity'
            parameters[item['name']]=dict(root=item['root'],offset=offset,bytes=buffer[offset:offset+length].hex())
        elif item['kind']=='type-record-name':
            def observed(name):
                row=next(row for row in data if row['name']==name)
                return int.from_bytes(original['memory'][row['original']],'little')
            base=int.from_bytes(original['memory'][observed('type_segments')][:4],'little');segment=original['memory'][base]
            count=observed('type_record_count');records=original['memory'][observed('type_records')]
            assert 0<=count<=32768 and len(records)>=count*32,'invalid type-name owner inventory'
            indices=[]
            for i in range(count):
                pointer=int.from_bytes(records[i*32:i*32+4],'little');offset=pointer-base
                if not 0<=offset<=len(segment)-4:continue
                kind=int.from_bytes(segment[offset+2:offset+4],'little')
                if kind not in (0x1004,0x1005,0x1006,0x1007):continue
                name_offset=16 if kind==0x1007 else 12 if kind==0x1006 else 20
                if kind!=0x1007:
                    if offset+name_offset+2>len(segment):continue
                    numeric=int.from_bytes(segment[offset+name_offset:offset+name_offset+2],'little')
                    if numeric>=0x8000:continue # Needs an encountered numeric-leaf contract.
                    name_offset+=2
                if pointer+name_offset!=value:continue
                length=segment[value-base]+1;record_size=int.from_bytes(segment[offset:offset+2],'little')+2
                assert name_offset+length<=record_size and offset+record_size<=len(segment),'type name exceeds its initialized owner record'
                indices.append(i)
            assert indices,'type name has no observed immediate-size local-record owner'
            length=segment[value-base]+1
            parameters[item['name']]=dict(segment=0,offset=value-base,record_indices=indices,bytes=segment[value-base:value-base+length].hex())
        elif item['kind']=='arena-type-record':
            arena_parameters.append((item,value))
        elif item['kind']=='type-record-data':
            def observed(name):
                row=next(row for row in data if row['name']==name)
                return int.from_bytes(original['memory'][row['original']],'little')
            base=int.from_bytes(original['memory'][observed('type_segments')][:4],'little')
            segment=original['memory'][base];root=observed('type_records');count=observed('type_record_count')
            records=original['memory'][root]
            indices=[i for i in range(count) if int.from_bytes(records[i*32:i*32+4],'little')==value]
            assert indices and base<=value<=base+len(segment)-4,'original type payload has no observed local-record/segment owner'
            offset=value-base;length=int.from_bytes(segment[offset:offset+2],'little')+2
            assert 4<=length<=len(segment)-offset,'original type payload exceeds segment capacity'
            parameters[item['name']]=dict(segment=0,offset=offset,record_indices=indices,bytes=segment[offset:offset+length].hex())
        elif item['kind']=='output-buffer':
            assert original['stack'][item['owner_return_slot']]==item['owner_return'],'output buffer original caller site differs'
            assert value==original['esp']+item['original_delta'],'output buffer original stack-local identity differs'
            size=original['stack'][item['count_slot']];assert 0<=size<=item['capacity']
            parameters[item['name']]=dict(owner=item['owner'],local=item['local'],capacity=item['capacity'],size=size)
        elif item['kind']=='local-record':
            assert original['stack'][0]==item['owner_return'] and value==original['esp']+item['original_delta'],'original record owner/caller-local identity differs'
            raw=original['memory'][value];size=int.from_bytes(raw[:2],'little')+2
            assert len(raw)==size and 2<=size<=item['capacity']
            parameters[item['name']]=dict(owner=item['owner'],local=item['local'],capacity=item['capacity'],bytes=raw.hex())
        elif item['kind']=='caller-local-pointer':
            assert original['stack'][0]==item['owner_return'] and value==original['esp']+item['original_delta'],'original output argument caller-local identity differs'
            parameters[item['name']]=dict(owner=item['owner'],local=item['local'],width=item['width'])
            if item.get('initialized'):
                assert len(original['memory'][value])==item['width']
                parameters[item['name']]['bytes']=original['memory'][value].hex()
        elif item['kind']=='function-pointer':
            assert value==item['original'],'original function-pointer argument differs'
            parameters[item['name']]=dict(function=item['function'])
        elif item['kind'] == 'object-pointer':
            object_parameters.append((item,value))
        elif item['kind']=='allocation-output':
            owned=[row for row in original['image_allocations'] if row['site']==item['allocation_site']]
            assert len(owned)==1,'symbol output allocation lifetime differs'
            allocation=owned[0]
            row=next(row for row in data if row['name']==item['cursor'])
            cursor=int.from_bytes(original['memory'][row['original']],'little')
            offset=value-allocation['address']
            if item.get('record_argument'):
                expected_offset=original['stack'][item['offset_slot']]
                assert offset==expected_offset and 0<=offset<=cursor-allocation['address']-4,'original reference record offset/owner differs'
                raw=original['memory'][allocation['address']];length=int.from_bytes(raw[offset:offset+2],'little')+2
                assert 4<=length<=cursor-value and len(raw)>=offset+length,'original reference record exceeds initialized output'
                parameters[item['name']]=dict(owner=item['owner'],offset=offset,capacity=allocation['size'],bytes=raw[offset:offset+length].hex())
            elif item.get('base_argument'):
                size=original['stack'][item['size_slot']]
                assert offset==0 and 0<=size<=allocation['size'] and value+size==cursor,'original initialized symbol buffer ownership/extent differs'
                assert len(original['memory'][value])>=size,'incomplete initialized symbol buffer'
                parameters[item['name']]=dict(owner=item['owner'],offset=0,capacity=allocation['size'],size=size,bytes=original['memory'][value][:size].hex())
            else:
                assert value==cursor,'symbol output argument cursor alias differs'
                assert 0<=offset<=allocation['size']-item['write_size'],'symbol output argument extent differs'
                parameters[item['name']]=dict(owner=item['owner'],offset=offset,capacity=allocation['size'],write_size=item['write_size'])
        else:
            raise AssertionError('unimplemented parameter kind: '+item['kind'])
    caller_state={}
    for item in record.get('caller_state',[]):
        assert item['width'] in (1,2,4)
        value=original['registers'][item['original_register']] & ((1 << (item['width']*8))-1)
        caller_state[item['name']]=dict(owner=item['owner'],local=item['local'],width=item['width'],native_type=item['native_type'],value=value)
    assert caller_state==native.get('caller_state',{}),'typed caller state differs'
    reads=[]
    for captured in original['file_reads']:
        row=dict(captured)
        pc=row.pop('pc');sites=[site for site in config.get('read_sites',[]) if site['return_pc']==pc]
        assert len(sites)==1,'unknown actual original read caller'
        site=sites[0];row.pop('buffer')
        if site.get('segment_input'):
            item=next(item for item in data if item['name']=='type_segments')
            table=int.from_bytes(original['memory'][item['original']],'little')
            base=int.from_bytes(original['memory'][table][:4],'little')
            item=next(item for item in data if item['name']=='type_segment_bytes')
            capacity=min(int.from_bytes(original['memory'][item['original']],'little'),0xffe8)
            assert any(allocation['address']==base and allocation['size']==capacity for allocation in original['image_allocations']), 'type read has no owned segment capacity'
            assert base<=captured['buffer'] and captured['buffer']+row['size']<=base+capacity, 'type read exceeds first segment'
            row.update(segment_index=0,segment_offset=captured['buffer']-base)
        elif site.get('allocated_global'):
            item=next(item for item in data if item['name']==site['allocated_global']);address=int.from_bytes(original['memory'][item['original']],'little')
            item=next(item for item in data if item['name']==site['capacity_global']);capacity=int.from_bytes(original['memory'][item['original']],'little')
            if 'capacity_max' in site:capacity=min(capacity,site['capacity_max'])
            if 'count_literal' in site:assert row['size']==site['count_literal']
            assert captured['buffer']==address and any(row['address']==address and row['size']==capacity for row in original['image_allocations']),'read buffer allocation/global ownership differs'
        elif site.get('count_parameter'):
            allocations=[allocation for allocation in original['module_allocations' if site.get('allocation_kind')=='module' else 'subsection_allocations'] if allocation['address']==captured['buffer']]
            assert len(allocations)==1,'subsection allocation ownership missing/ambiguous'
            allocation=allocations[0];capacity=allocation['size']
            item=next(item for item in data if item['name']=='directory_entries');root=int.from_bytes(original['memory'][item['original']],'little')
            entry=allocation['entry'];assert entry>=root and (entry-root)%12==0
            payload=original['memory'][root][entry-root:entry-root+12];assert len(payload)==12
            assert int.from_bytes(payload[8:12],'little')==capacity==row['size'],'actual subsection allocation size differs'
        elif site.get('heap_buffer'):
            item=next(item for item in data if item['name']==site['count'])
            count=int.from_bytes(original['memory'][item['original']],'little')
            capacity=(count+site['spare_elements'])*site['element_size']
            assert row['size']==count*site['element_size']
            assert original['directory_allocation']==dict(address=captured['buffer'],size=capacity),'original directory allocation capacity/ownership differs'
        else:capacity=site['capacity']
        row.update(owner=site['owner'],local=site['local'],capacity=capacity)
        assert 0<=row['size']<=capacity
        if 'returned' in row:assert row['returned']==row['size'],'partial read needs a defined-tail contract'
        reads.append(row)
    assert reads==native.get('file_reads',[]),'actual read caller/extent/position/result/initialized output differs'
    if original['directory_allocation']:
        row=next(row for row in data if row['name']=='directory_entries')
        address=int.from_bytes(original['memory'][row['original']],'little');other=int.from_bytes(bytes.fromhex(native['globals'][row['name']]),'little')
        assert original['directory_allocation']['address']==address and native['directory_allocation']['address']==other
        assert original['directory_allocation']['size']==native['directory_allocation']['size'],'actual directory allocation capacity differs'
    allocations=original['subsection_allocations'];actual_allocations=native.get('subsection_allocations',[]);allocation_aliases={}
    assert len(allocations)==len(actual_allocations),'subsection allocation inventory differs'
    if allocations:
        item=next(item for item in data if item['name']=='directory_entries');root=int.from_bytes(original['memory'][item['original']],'little')
        for left,right in zip(allocations,actual_allocations):
            assert left['entry']>=root and (left['entry']-root)%12==0
            assert left['size']==right['size'] and (left['entry']-root)//12==right['entry_index'],'subsection allocation source/size differs'
            assert left['address'] and right['address'] and left['address'] not in allocation_aliases and right['address'] not in allocation_aliases.values(),'live subsection allocation identity reused'
            allocation_aliases[left['address']]=right['address']
    module_aliases={};module_sizes={}
    assert len(original['module_allocations'])==len(native.get('module_allocations',[])),'module allocation inventory differs'
    if original['module_allocations']:
        row=next(row for row in data if row['name']=='directory_entries');root=int.from_bytes(original['memory'][row['original']],'little')
        for left,right in zip(original['module_allocations'],native['module_allocations']):
            assert left['entry']>=root and (left['entry']-root)%12==0
            assert left['size']==right['size'] and (left['entry']-root)//12==right['entry_index'],'module allocation source/size differs'
            assert left['address'] and right['address'] and left['address'] not in module_aliases and right['address'] not in module_aliases.values(),'live module buffer identity reused'
            module_aliases[left['address']]=right['address'];module_sizes[left['address']]=left['size']
    image_aliases={};image_sizes={}
    assert len(original['image_allocations'])==len(native.get('image_allocations',[])),'image allocation inventory differs'
    for left,right in zip(original['image_allocations'],native.get('image_allocations',[])):
        sites={0x4062b0:('debug_directory','module_capacity'),0x4062d3:('symbol_data','symbol_capacity'),
               0x4062f7:('type_data','line_capacity'),0x40631b:('module_data','type_capacity'),0x40639d:None,0x40a7d1:None}
        assert left['site'] in sites,'unknown actual image allocator caller'
        if sites[left['site']] is not None:
            name,size_name=sites[left['site']]
            row=next(row for row in data if row['name']==name)
            assert int.from_bytes(original['memory'][row['original']],'little')==left['address'],'actual allocation not assigned to its evidenced output'
            row=next(row for row in data if row['name']==size_name)
            assert int.from_bytes(original['memory'][row['original']],'little')==left['size'],'actual allocation request differs from capacity source'
        if left['site']==0x40a7d1:
            def value(name):
                item=next(row for row in data if row['name']==name)
                return int.from_bytes(original['memory'][item['original']],'little')
            entries=original['memory'][value('directory_entries')];index=value('line_entry_index')
            size=int.from_bytes(entries[index*12+8:index*12+12],'little')
            assert size+value('module_symbol_count')-value('module_reference_count')==left['size'],'symbol rewrite allocation expression differs'
            assert right.get('owner')=='rewrite_module_symbols','symbol output allocation owner differs'
        assert left['size']==right['size'] and left['address'] and right['address'],'actual image allocation request/result differs'
        assert left['address'] not in image_aliases and right['address'] not in image_aliases.values(),'live image allocation identity reused'
        image_aliases[left['address']]=right['address'];image_sizes[left['address']]=left['size']
    assert len(original['checked_allocations'])==len(native.get('checked_allocations',[])),'checked allocator request inventory differs'
    for left,right in zip(original['checked_allocations'],native.get('checked_allocations',[])):
        assert left['size']==right['size'] and left['address'] and right['address'],'checked allocator request/result differs'
        assert left['address'] not in image_aliases and right['address'] not in image_aliases.values(),'live checked allocation identity reused'
        image_aliases[left['address']]=right['address'];image_sizes[left['address']]=left['size']
    assert len(original['cleared_allocations'])==len(native.get('cleared_allocations',[])), 'cleared allocator inventory differs'
    for left,right in zip(original['cleared_allocations'],native.get('cleared_allocations',[])):
        assert left['size']==right['size'] and left['address'] and right['address'], 'cleared allocation request/result differs'
        assert left['address'] not in image_aliases and right['address'] not in image_aliases.values(), 'live cleared allocation identity reused'
        image_aliases[left['address']]=right['address'];image_sizes[left['address']]=left['size']
    used_uninitialized=set()
    aliases,reverse={},{}
    handle_aliases,handle_reverse={},{}
    original_calls=original['file_calls'];native_calls=native.get('file_calls',[])
    assert len(original_calls)==len(native_calls),'actual file-open call inventory differs'
    for left,right in zip(original_calls,native_calls):
        assert {key:value for key,value in left.items() if key!='handle'}=={key:value for key,value in right.items() if key!='handle'},'actual file-open ABI/input/LastError differs'
        assert bool(left['handle'])==bool(right['handle'])
        assert left['handle'] not in handle_aliases and right['handle'] not in handle_reverse,'file HANDLE lifetime needs explicit model'
        handle_aliases[left['handle']]=right['handle'];handle_reverse[right['handle']]=left['handle']
    def file_handle(left,right):
        assert bool(left)==bool(right),'file HANDLE NULL state differs'
        if not left:return
        assert handle_aliases[left]==right and handle_reverse[right]==left,'file HANDLE ownership/alias differs'
        expected=original['file_handles'][left];actual=native['file_handles'][str(right)]
        assert expected=={key:actual[key] for key in ('type','size','position')},'file HANDLE type/size/position differs'
        call=next(row for row in original_calls if row['handle']==left)
        pathname=bytes.fromhex(call['pathname']).rstrip(b'\0').decode('latin1')
        assert actual['pathname']==pathname,'file HANDLE frozen pathname differs'
        access=call['access'];mode=2 if access&0x40000000 and access&0x80000000 else 1 if access&0x40000000 else 0
        assert actual['access_mode']==mode,'file HANDLE descriptor access differs'
        assert artifacts is not None and pathname in artifacts,'missing actual original file contents'
        content=artifacts[pathname]
        assert len(content)==expected['size'] and hashlib.sha256(content).hexdigest()==actual['sha256'],'file HANDLE complete contents differ'

    mapping_aliases={};view_roles={};live_mappings=set()
    assert len(original['mapping_calls'])==len(native.get('mapping_calls',[])),'actual mapping call inventory differs'
    for index,(left,right) in enumerate(zip(original['mapping_calls'],native.get('mapping_calls',[]))):
        assert left['kind']==right['kind'] and left['last_error']==right['last_error'],'mapping call kind/LastError differs'
        kind=left['kind'];source=left['args'];actual=right['args']
        if kind=='create':
            assert handle_aliases[source[0]]==actual[0] and source[1:]==actual[1:],'mapping source/ABI/input differs'
            assert bool(left['result'])==bool(right['result'])
            if left['result']:
                assert left['result'] not in mapping_aliases,'mapping HANDLE generation needs explicit reuse model'
                mapping_aliases[left['result']]=right['result'];live_mappings.add(left['result'])
                assert right['registry']==dict(length=source[4] or original['file_handles'][source[0]]['size'],writable=int(source[2]==4),source_owned=True),'typed mapping registry state differs'
        elif kind=='map':
            assert source[0] in live_mappings and mapping_aliases[source[0]]==actual[0] and source[1:]==actual[1:],'view source/ABI/input differs'
            assert not source[5],'fixed mapping base needs explicit address-role proof'
            assert bool(left['result'])==bool(right['result'])
            if left['result']:
                assert left['result'] not in view_roles,'mapped view generation needs explicit reuse model'
                creation=next(row for row in original['mapping_calls'] if row['kind']=='create' and row['result']==source[0])
                view_roles[left['result']]=dict(address=right['result'],index=index,creation=creation,arguments=source)
        elif kind=='close':
            assert source[0] in live_mappings and mapping_aliases[source[0]]==actual[0] and left['result']==right['result']==1,'mapping ownership/close result differs'
            live_mappings.remove(source[0])
        else:raise AssertionError('unknown mapping call kind')
    def mapped_view(address,other,length):
        assert bool(address)==bool(other),'mapped view NULL state differs'
        if not address:return
        role=view_roles[address];assert role['address']==other,'mapped view alias/generation differs'
        left=original['mapped_views'][address];right=native['mapped_views'][str(other)]
        assert left['length']==right['length']==length and left['base']==address and left['region']>=length,'mapped view extent/base differs'
        assert left['state']==0x1000 and left['type']==0x40000,'original view is not committed mapped memory'
        creation=role['creation'];args=role['arguments']
        assert left['protection']==creation['args'][2],'original actual view protection differs'
        assert right['permissions']==('rw-s' if left['protection']==4 else 'r--s'),'native actual view protection/sharing differs'
        offset=(args[2]<<32)+args[3]
        assert right['offset']==offset and right['call_index']==role['index'],'mapped view offset/generation differs'
        owner=next(row for row in original_calls if row['handle']==creation['args'][0])
        pathname=bytes.fromhex(owner['pathname']).rstrip(b'\0').decode('latin1')
        assert right['pathname']==pathname,'mapped view backing pathname differs'
        assert views is not None,'missing complete mapped-memory observations'
        before=views['original'][left['file']];actual=views['native'][right['file']]
        assert len(before)==len(actual)==length and before==actual,'complete mapped bytes differ'
        assert hashlib.sha256(actual).hexdigest()==right['sha256'],'native mapped bytes do not bind raw observation'
        assert artifacts is not None and before==artifacts[pathname][offset:offset+length],'view contents differ from observed backing file'
    def pair(address,other):
        assert bool(address)==bool(other),'pointer NULL state differs'
        if not address:return False
        if address in aliases:
            assert aliases[address]==other and reverse[other]==address,'object alias differs'
            return False
        assert other not in reverse,'native object aliases distinct original objects'
        aliases[address]=other;reverse[other]=address
        return True
    def allocation(address,other,spec):
        assert bool(address)==bool(other),'uninitialized allocation NULL state differs'
        if not address:return
        item=next(item for item in data if item['name']==spec['size_global'])
        size=int.from_bytes(original['memory'][item['original']],'little')*spec.get('element_size',1)
        if 'maximum' in spec:size=min(size,spec['maximum'])
        assert image_aliases[address]==other and image_sizes[address]==size,'uninitialized allocation ownership/capacity differs'
        used_uninitialized.add(address)
        pair(address,other)
    def module_buffer(address,other,length=None):
        assert bool(address)==bool(other),'module buffer NULL state differs'
        if not address:return
        assert module_aliases[address]==other,'module payload allocation alias differs'
        if length is None:length=module_sizes[address]
        assert module_sizes[address]==length,'module payload capacity differs'
        left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
        assert len(left)==len(right)==length and left==right,'complete initialized module payload differs'
        pair(address,other)
    def linked(address,other,shape):
        count=0
        while pair(address,other):
            count+=1
            assert count<=128,'unbounded linked object chain'
            left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
            assert len(left)==len(right)==shape['size']
            offset=shape['link'];assert 0<=offset<=shape['size']-4
            excluded=set(range(offset,offset+4))
            if 'string' in shape:
                text=shape['string'];assert 0<=text<=shape['size']-4
                excluded.update(range(text,text+4))
                source=int.from_bytes(left[text:text+4],'little');destination=int.from_bytes(right[text:text+4],'little')
                pair(source,destination)
                if source:assert original['strings'][source].hex()==native['strings'][str(destination)],'target object string differs'
            if 'handle' in shape:
                handle=shape['handle'];excluded.update(range(handle,handle+4))
                file_handle(int.from_bytes(left[handle:handle+4],'little'),int.from_bytes(right[handle:handle+4],'little'))
            if 'view' in shape:
                spec=shape['view'];offset_view=spec['offset'];excluded.update(range(offset_view,offset_view+4))
                mapped_view(int.from_bytes(left[offset_view:offset_view+4],'little'),int.from_bytes(right[offset_view:offset_view+4],'little'),int.from_bytes(left[spec['length_offset']:spec['length_offset']+4],'little'))
            if 'payload' in shape:
                spec=shape['payload'];payload_offset=spec['offset'];excluded.update(range(payload_offset,payload_offset+4))
                module_buffer(int.from_bytes(left[payload_offset:payload_offset+4],'little'),int.from_bytes(right[payload_offset:payload_offset+4],'little'),int.from_bytes(left[spec['length_offset']:spec['length_offset']+4],'little'))
            assert all(a==b for i,(a,b) in enumerate(zip(left,right)) if i not in excluded),'target object bytes differ'
            address=int.from_bytes(left[offset:offset+4],'little')
            other=int.from_bytes(right[offset:offset+4],'little')
    def global_integer(name):
        item=next(item for item in data if item['name']==name)
        return int.from_bytes(original['memory'][item['original']],'little')
    # Arena capacities are captured in full, but only each block's used prefix
    # is initialized. Preserve allocation and chain identity independently.
    arena_payloads={};arena_nodes=set()
    for arena_item in data:
        if not arena_item.get('type_arena'):continue
        a=original['memory'][arena_item['original']];b=bytes.fromhex(native['globals'][arena_item['name']])
        assert len(a)==len(b)==24,'incomplete arena header'
        av=struct.unpack('<6I',a);bv=struct.unpack('<6I',b)
        assert av[0:2]==bv[0:2] and av[4]==bv[4],'arena capacity/count/remaining differs'
        assert av[0] in (0x1000,0x10000) and av[1]<=1024,'invalid arena capacity/block count'
        source=av[5];destination=bv[5];visited=[]
        while source or destination:
            assert source and destination and source not in arena_nodes,'arena chain NULL/identity/cycle differs'
            arena_nodes.add(source);visited.append(source);pair(source,destination)
            assert image_aliases[source]==destination and image_sizes[source]==12,'arena block header allocation differs'
            assert any(row['address']==source for row in original['cleared_allocations']),'arena header was not cleared'
            left=original['memory'][source];right=bytes.fromhex(native['memory'][str(destination)])
            assert len(left)==len(right)==12,'incomplete arena block header'
            x=struct.unpack('<3I',left);y=struct.unpack('<3I',right)
            assert x[0]==y[0] and 0<=x[0]<=av[0],'arena initialized extent differs'
            assert x[1] in image_aliases and image_aliases[x[1]]==y[1] and image_sizes[x[1]]==av[0],'arena payload allocation differs'
            before=original['memory'][x[1]];after=bytes.fromhex(native['memory'][str(y[1])])
            assert len(before)==len(after)==av[0],'incomplete arena payload capacity'
            assert before[:x[0]]==after[:y[0]],'initialized arena payload bytes differ'
            assert x[1] not in arena_payloads,'arena payload identity reused'
            arena_payloads[x[1]]=dict(native=y[1],used=x[0],capacity=av[0],bytes=before,
                arena=arena_item['name'],block_index=len(visited)-1)
            used_uninitialized.add(x[1]);pair(x[1],y[1])
            source,destination=x[2],y[2]
        assert len(visited)==av[1],'arena chain inventory differs'
        if visited:
            last=visited[-1];actual_last=aliases[last];last_fields=struct.unpack('<3I',original['memory'][last])
            assert av[2]==last and bv[2]==actual_last,'arena last-block alias differs'
            assert av[3]==last_fields[1]+last_fields[0] and bv[3]==image_aliases[last_fields[1]]+last_fields[0],'arena cursor alias/offset differs'
            assert av[4]==av[0]-last_fields[0],'arena remaining/used extent differs'
        else:assert not any(av[2:]) and not any(bv[2:]),'empty arena has live state'
    def arena_payload(source,destination,length=None):
        owners=[(base,row) for base,row in arena_payloads.items() if base<=source<base+row['used']]
        assert len(owners)==1,'type payload has no initialized arena owner'
        base,row=owners[0];offset=source-base
        assert destination==row['native']+offset,'type payload arena/offset alias differs'
        if length is None:
            assert offset+2<=row['used'],'incomplete arena record length'
            length=int.from_bytes(row['bytes'][offset:offset+2],'little')+2
            assert length>=4,'invalid arena type record length'
        assert 0<=length and offset+length<=row['used'],'type payload exceeds initialized arena prefix'
        return row['bytes'][offset:offset+length]
    for spec,address in arena_parameters:
        owners=[(base,row) for base,row in arena_payloads.items() if row['arena']==spec['arena'] and base<=address<base+row['used']]
        assert len(owners)==1,'arena argument has no initialized owned block'
        base,row=owners[0];offset=address-base
        assert offset+2<=row['used'],'arena argument lacks initialized length'
        length=int.from_bytes(row['bytes'][offset:offset+2],'little')+2
        assert 4<=length<=row['used']-offset,'arena argument exceeds initialized record extent'
        index=parameters[spec['index_parameter']]
        assert 0x1000<=index<global_integer('next_global_type'),'arena argument index out of range'
        records=original['memory'][global_integer('type_records')];count=global_integer('type_record_count')
        indices=[i for i in range(count) if int.from_bytes(records[i*32:i*32+4],'little')==address and int.from_bytes(records[i*32+4:i*32+8],'little')==index]
        assert indices,'arena argument has no mapped local-record owner'
        root=next(item['original'] for item in data if item['name']=='global_type_blocks')
        assert global_integer('type_table_used')==512 and index-0x1000<512,'arena argument block inventory needs explicit contract'
        block=int.from_bytes(original['memory'][root][:4],'little');slot=(index&511)*4
        assert int.from_bytes(original['memory'][block][slot:slot+4],'little')==address,'arena argument/global index slot alias differs'
        parameters[spec['name']]=dict(arena=spec['arena'],block_index=row['block_index'],offset=offset,
            record_indices=indices,index=index,bytes=row['bytes'][offset:offset+length].hex())
    typed_records_checked=0
    extended_references_checked=0
    recursive_type_nodes_checked=0
    interned_type_nodes_checked=0
    pooled_interned_nodes_checked=0
    cached_type_names_checked=0
    globals_checked = []
    for item in data:
        raw = original['memory'][item['original']]
        assert len(raw)==item['size']
        actual = bytes.fromhex(native['globals'][item['name']])
        assert len(actual)==item['size']
        if item['kind']=='pointer-array':
            shape=item['shape'];count=shape['count_literal']
            assert len(raw)==len(actual)==count*4
            for index in range(count):
                linked(int.from_bytes(raw[index*4:index*4+4],'little'),int.from_bytes(actual[index*4:index*4+4],'little'),shape)
        elif item['kind'] == 'pointer':
            if item.get('recursive_types'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                if pair(address,other):
                    assert image_aliases[address]==other and image_sizes[address]==16384,'recursive-type bucket allocation differs'
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==16384,'incomplete recursive-type bucket observation'
                    counts=original['memory'][global_integer('type_table_primary_counts')]
                    assert len(counts)==8192,'incomplete recursive-type bucket counts'
                    records=original['memory'][global_integer('type_records')];visited=set()
                    for offset in range(0,16384,4):
                        source=int.from_bytes(left[offset:offset+4],'little');destination=int.from_bytes(right[offset:offset+4],'little');count=0
                        while source or destination:
                            assert source and destination and source not in visited and len(visited)<4096,'recursive-type node NULL/identity/chain differs'
                            visited.add(source);pair(source,destination);count+=1
                            owners=[base for base,size in image_sizes.items() if size==0x3000 and base<=source and source+12<=base+size and (source-base)%12==0]
                            assert len(owners)==1,'recursive-type node has no actual pool ownership'
                            owner=owners[0]
                            assert destination==image_aliases[owner]+source-owner,'recursive-type pool/offset alias differs'
                            a=original['memory'][source];b=bytes.fromhex(native['memory'][str(destination)])
                            assert len(a)==len(b)==12,'incomplete initialized recursive-type node'
                            index=int.from_bytes(a[4:8],'little')
                            assert a[4:8]==b[4:8] and 0x1000<=index<global_integer('next_global_type'),'recursive-type index differs'
                            payload=int.from_bytes(a[:4],'little');actual_payload=int.from_bytes(b[:4],'little')
                            arena_payload(payload,actual_payload)
                            matches=[records[i:i+32] for i in range(0,global_integer('type_record_count')*32,32) if int.from_bytes(records[i:i+4],'little')==payload and int.from_bytes(records[i+4:i+8],'little')==index]
                            assert matches and any(int.from_bytes(row[14:16],'little')==offset//4 for row in matches),'recursive-type record/hash bucket differs'
                            source=int.from_bytes(a[8:12],'little');destination=int.from_bytes(b[8:12],'little')
                            used_uninitialized.add(owner);recursive_type_nodes_checked+=1
                        assert count==int.from_bytes(counts[offset//2:offset//2+2],'little'),'recursive-type bucket count differs'
            elif item.get('interned_types'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                if pair(address,other):
                    assert image_aliases[address]==other and image_sizes[address]==16384,'interned-type bucket allocation differs'
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==16384,'incomplete interned-type bucket observation'
                    visited=set();counts_address=global_integer('type_table_secondary_counts')
                    counts=original['memory'][counts_address]
                    for offset in range(0,16384,4):
                        source=int.from_bytes(left[offset:offset+4],'little');destination=int.from_bytes(right[offset:offset+4],'little');count=0
                        while source or destination:
                            assert source and destination and source not in visited and len(visited)<4096,'interned-type node NULL/identity/chain differs'
                            visited.add(source);count+=1;pair(source,destination)
                            if image_sizes.get(source)==12:
                                assert image_aliases[source]==destination,'interned-type node allocation differs'
                            else:
                                assert any(row['name']=='interned_node_cursor' for row in data),'interned-type pool needs an observed contract'
                                owners=[base for base,size in image_sizes.items() if size==0x3000 and base<=source and source+12<=base+size and (source-base)%12==0]
                                assert len(owners)==1,'interned-type node has no actual pool ownership'
                                owner=owners[0]
                                remaining=global_integer('interned_nodes_remaining')
                                cursor=global_integer('interned_node_cursor')
                                assert 0<=remaining<=1024 and cursor-owner==(1024-remaining)*12,'interned-type active pool/cursor ownership differs'
                                assert source+12<=cursor,'interned-type node exceeds initialized pool prefix'
                                assert destination==image_aliases[owner]+source-owner,'interned-type pool/offset alias differs'
                                used_uninitialized.add(owner);pooled_interned_nodes_checked+=1
                            a=original['memory'][source];b=bytes.fromhex(native['memory'][str(destination)])
                            assert len(a)==len(b)==12 and a[:4]==b[:4],'interned-type index/extent differs'
                            index=int.from_bytes(a[:4],'little');assert 0x1000<=index<global_integer('next_global_type'),'interned-type index out of range'
                            payload=int.from_bytes(a[4:8],'little');actual_payload=int.from_bytes(b[4:8],'little')
                            arena_payload(payload,actual_payload)
                            block_root=next(row['original'] for row in data if row['name']=='global_type_blocks')
                            assert index-0x1000<512,'interned-type block inventory needs an explicit contract'
                            block=int.from_bytes(original['memory'][block_root][:4],'little')
                            assert int.from_bytes(original['memory'][block][(index&511)*4:(index&511)*4+4],'little')==payload,'interned-type/global slot alias differs'
                            source=int.from_bytes(a[8:12],'little');destination=int.from_bytes(b[8:12],'little')
                            interned_type_nodes_checked+=1
                        assert count==int.from_bytes(counts[offset//2:offset//2+2],'little'),'interned-type bucket count differs'
            elif item.get('type_name_cache'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                if pair(address,other):
                    assert image_aliases[address]==other and image_sizes[address]==2048,'type-name bucket allocation differs'
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==2048,'incomplete type-name bucket observation'
                    visited=set();cleared={row['address'] for row in original['cleared_allocations']}
                    checked={row['address'] for row in original['checked_allocations']}
                    table=global_integer('type_segments');base=int.from_bytes(original['memory'][table][:4],'little')
                    for offset in range(0,2048,4):
                        source=int.from_bytes(left[offset:offset+4],'little');destination=int.from_bytes(right[offset:offset+4],'little')
                        while source or destination:
                            assert source and destination and source not in visited and len(visited)<512,'type-name node NULL/identity/chain differs'
                            visited.add(source);pair(source,destination)
                            assert source in cleared and image_aliases[source]==destination and image_sizes[source]==20,'type-name node allocation differs'
                            a=original['memory'][source];b=bytes.fromhex(native['memory'][str(destination)])
                            assert len(a)==len(b)==20,'incomplete type-name node observation'
                            assert a[8:16]==b[8:16],'type-name index/reserved fields differ'
                            name=int.from_bytes(a[:4],'little');actual_name=int.from_bytes(b[:4],'little')
                            pair(name,actual_name)
                            if name:
                                text=original['memory'][name];actual_text=bytes.fromhex(native['memory'][str(actual_name)])
                                assert len(text)==len(actual_text)==text[0]+1==actual_text[0]+1,'incomplete type-name counted string observation'
                                if item['type_name_cache']=='global':
                                    assert arena_payload(name,actual_name,len(text))==text,'global counted name bytes/owner differ'
                                else:assert name in checked and image_aliases[name]==actual_name and image_sizes[name]==len(text),'type-name counted string allocation differs'
                                assert text==actual_text,'type-name counted string bytes differ'
                                assert sum(text)&511==offset//4,'type-name bucket/hash differs'
                            payload=int.from_bytes(a[4:8],'little');actual_payload=int.from_bytes(b[4:8],'little')
                            assert bool(payload)==bool(actual_payload),'type-name record NULL state differs'
                            if payload:
                                if item['type_name_cache']=='global':arena_payload(payload,actual_payload)
                                else:
                                    displacement=payload-base;segment=original['memory'][base]
                                    assert 0<=displacement<=len(segment)-4,'type-name record has no observed segment owner'
                                    length=int.from_bytes(segment[displacement:displacement+2],'little')+2
                                    assert length>=4 and any(row.get('segment_index')==0 and row['segment_offset']<=displacement and displacement+length<=row['segment_offset']+row['size'] for row in reads),'type-name record exceeds initialized segment'
                                    assert actual_payload==image_aliases[base]+displacement,'type-name record segment alias differs'
                            source=int.from_bytes(a[16:20],'little');destination=int.from_bytes(b[16:20],'little')
                            cached_type_names_checked+=1
            elif item.get('type_record_table'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                if pair(address,other):
                    assert global_integer('type_module_count')==1, 'reused type-table lifetime needs an explicit state contract'
                    capacity=global_integer('type_record_capacity');count=global_integer('type_record_count')
                    assert 0<=count<=capacity<=32768, 'type record count/capacity differs'
                    assert image_aliases[address]==other and image_sizes[address]==capacity*32, 'type record table allocation differs'
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==capacity*32, 'incomplete initialized type record table'
                    table=global_integer('type_segments');base=int.from_bytes(original['memory'][table][:4],'little')
                    segment=original['memory'][base];destination=image_aliases[base]
                    initialized=[(row['segment_offset'],row['segment_offset']+row['size']) for row in reads if row.get('segment_index')==0]
                    for index in range(capacity):
                        a=left[index*32:index*32+32];b=right[index*32:index*32+32]
                        if index>=count:
                            assert not any(a) and not any(b), 'unused type record capacity is not cleared'
                            continue
                        source=int.from_bytes(a[:4],'little');target=int.from_bytes(b[:4],'little')
                        assert bool(source)==bool(target), 'type record payload NULL state differs'
                        if source:
                            offset=source-base
                            if 0<=offset<=len(segment)-4:
                                length=int.from_bytes(segment[offset:offset+2],'little')+2
                                assert length>=4 and any(first<=offset and offset+length<=last for first,last in initialized), 'type record exceeds initialized segment input'
                                assert target==destination+offset, 'type record segment/payload offset alias differs'
                            else:arena_payload(source,target)
                        assert a[4:16]==b[4:16] and a[20:24]==b[20:24] and a[28:]==b[28:], 'initialized type record fields differ'
                        if a[29]&1:
                            references=int.from_bytes(a[12:14],'little')
                            cleared={row['address'] for row in original['cleared_allocations']}
                            for field,minimum in [(16,references*2),(24,(references+7)//8)]:
                                child=int.from_bytes(a[field:field+4],'little');actual_child=int.from_bytes(b[field:field+4],'little')
                                assert child in cleared and child in image_aliases and image_aliases[child]==actual_child,'extended type reference allocation/alias differs'
                                size=image_sizes[child]
                                assert minimum<=size<=65536,'extended type reference allocation capacity differs'
                                before=original['memory'][child];after=bytes.fromhex(native['memory'][str(actual_child)])
                                assert len(before)==len(after)==size,'incomplete extended type reference allocation'
                                assert before==after,'initialized extended type reference bytes differ'
                                assert not any(before[minimum:]) and not any(after[minimum:]),'unused cleared type reference capacity differs'
                                pair(child,actual_child)
                            extended_references_checked+=1
                        else:
                            assert a[16:28]==b[16:28], 'initialized inline type reference bytes differ'
                    typed_records_checked=count
            elif item.get('segment_buffer'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                assert bool(address)==bool(other),'type segment buffer NULL state differs'
                if address:
                    row=next(row for row in data if row['name']=='type_segments');table=int.from_bytes(original['memory'][row['original']],'little')
                    base=int.from_bytes(original['memory'][table][:4],'little')
                    assert address==base and image_aliases[base]==other,'type segment buffer/allocator alias differs'
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    if item.get('segment_contents'):
                        capacity=min(global_integer('type_segment_bytes'),0xffe8)
                        assert len(left)==len(right)==capacity, 'incomplete type segment capacity observation'
                        defined=[(0,4)]+[(row['segment_offset'],row['segment_offset']+row['size']) for row in reads if row.get('segment_index')==0]
                        for first,last in defined:
                            assert 0<=first<=last<=capacity and left[first:last]==right[first:last], 'initialized type segment bytes differ'
                    else:
                        assert len(left)==len(right)==item['blob']['size'] and left==right,'initialized type segment prefix differs'
                    pair(address,other)
            elif item.get('public_buckets'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                if pair(address,other):
                    # Earlier public-only captures predate the complete cleared-allocator inventory.
                    if any(row['name']=='global_symbol_count' for row in data):
                        assert image_aliases[address]==other and image_sizes[address]==16384,'symbol bucket allocation ownership/capacity differs'
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==4096*4,'incomplete symbol bucket observation'
                    visited=set()
                    for offset in range(0,len(left),4):
                        source=int.from_bytes(left[offset:offset+4],'little');destination=int.from_bytes(right[offset:offset+4],'little')
                        while source or destination:
                            assert source and destination and source not in visited and len(visited)<512,'symbol node NULL/identity/chain differs'
                            visited.add(source);pair(source,destination)
                            owners=[base for base,size in image_sizes.items() if size==0x1fa4 and base<=source<base+size]
                            assert len(owners)==1,'public node has no actual arena ownership'
                            owner=owners[0];assert destination==image_aliases[owner]+source-owner,'public node arena/offset differs'
                            a=original['memory'][source];b=bytes.fromhex(native['memory'][str(destination)])
                            assert 20<=len(a)==len(b)==18+int.from_bytes(a[16:18],'little')<=1042 and source+len(a)<=owner+image_sizes[owner],'incomplete or unowned initialized symbol node extent'
                            assert a[4:]==b[4:],'initialized public node header/record/padding differs'
                            assert int.from_bytes(a[4:8],'little')%4096==offset//4,'symbol node hash/bucket differs'
                            name=16+int.from_bytes(a[12:14],'little')
                            assert 20<=name<len(a) and name+1+a[name]<=len(a),'symbol node counted name exceeds initialized record'
                            source=int.from_bytes(a[:4],'little');destination=int.from_bytes(b[:4],'little')
            elif item.get('written_allocation'):
                spec=item['written_allocation'];owned=[row for row in original['image_allocations'] if row['site']==spec['site']]
                assert len(owned)==1,'symbol output allocation lifetime differs'
                owner=owned[0];address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                offset=address-owner['address'];capacity=owner['size'];destination=image_aliases[owner['address']]
                assert 0<=offset<=capacity and other==destination+offset,'symbol output cursor allocation/offset differs'
                left=original['memory'][owner['address']];right=bytes.fromhex(native['memory'][str(destination)])
                assert len(left)==len(right)==capacity,'incomplete symbol output capacity observation'
                assert left[:offset]==right[:offset],'initialized symbol output prefix differs'
                used_uninitialized.add(owner['address']);pair(owner['address'],destination)
            elif item.get('buffer_cursor'):
                spec=item['buffer_cursor'];base=global_integer(spec['root']);capacity=global_integer(spec['capacity'])
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                destination=int.from_bytes(bytes.fromhex(native['globals'][spec['root']]),'little')
                assert 0<=address-base<=capacity and other==destination+address-base,'symbol input cursor buffer/offset differs'
            elif item.get('interior_allocation'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little');spec=item['interior_allocation']
                assert bool(address)==bool(other),'arena cursor NULL state differs'
                if address:
                    owners=[base for base,size in image_sizes.items() if size==spec['size'] and base<=address<=base+size]
                    assert len(owners)==1,'arena cursor ownership differs'
                    owner=owners[0];row=next(row for row in data if row['name']==spec['remaining'])
                    remaining=int.from_bytes(original['memory'][row['original']],'little')
                    assert address-owner+remaining*spec.get('remaining_element_size',1)==spec['size'] and other==image_aliases[owner]+address-owner,'arena cursor/capacity/offset differs'
                    used_uninitialized.add(owner)
            elif item.get('module_buffer'):
                module_buffer(int.from_bytes(raw,'little'),int.from_bytes(actual,'little'))
            elif 'buffer_alias' in item:
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                alias=item['buffer_alias'];source=global_integer(alias)
                target=int.from_bytes(bytes.fromhex(native['globals'][alias]),'little')
                assert address==source and other==target, 'retained subsection buffer alias differs'
                pair(address,other)
            elif 'allocation' in item:
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                allocation(address,other,item['allocation'])
                if address and item.get('read_contents'):
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==image_sizes[address], 'incomplete read-buffer capacity observation'
                    for read in original['file_reads']:
                        if address<=read['buffer']<address+image_sizes[address]:
                            offset=read['buffer']-address;end=offset+read['returned']
                            assert end<=image_sizes[address] and left[offset:end]==right[offset:end], 'initialized subsection buffer bytes differ'
            elif 'blob' in item:
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                if address in allocation_aliases:assert allocation_aliases[address]==other,'subsection output/global allocation alias differs'
                if pair(address,other):
                    spec=item['blob'];left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    size=spec.get('size')
                    if size is None:
                        count_item=next(row for row in data if row['name']==spec['count'])
                        size=int.from_bytes(original['memory'][count_item['original']],'little')*spec['element_size']
                    assert len(left)==len(right)==size,'allocated initialized byte extent differs'
                    excluded=set()
                    for pointer in spec.get('pointers',[]):
                        for offset in range(pointer['offset'],size,pointer.get('stride',size+1)):
                            excluded.update(range(offset,offset+4))
                            source=int.from_bytes(left[offset:offset+4],'little');destination=int.from_bytes(right[offset:offset+4],'little')
                            if 'allocation' in pointer:allocation(source,destination,pointer['allocation'])
                            elif 'shape' in pointer:linked(source,destination,pointer['shape'])
                            elif pair(source,destination):
                                if source in image_aliases:
                                    assert image_aliases[source]==destination and image_sizes[source]==pointer['size'],'initialized checked allocation ownership/capacity differs'
                                child=original['memory'][source];actual_child=bytes.fromhex(native['memory'][str(destination)])
                                assert len(child)==len(actual_child)==pointer['size'] and child==actual_child,'nested initialized allocation bytes differ'
                    for relocation in spec.get('relocations',[]):
                        for offset in range(relocation['offset'],size,relocation['stride']):
                            source=int.from_bytes(left[offset:offset+4],'little');destination=int.from_bytes(right[offset:offset+4],'little')
                            if source in module_aliases or destination in module_aliases.values():
                                excluded.update(range(offset,offset+4));module_buffer(source,destination)
                    assert all(a==b for i,(a,b) in enumerate(zip(left,right)) if i not in excluded),'allocated initialized bytes differ'
            elif 'shape' not in item:
                assert not any(raw) and not any(actual), ('non-NULL pointer needs an evidenced role',item['name'])
            else:
                shape=item['shape'];address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                if 'count' not in shape:linked(address,other,shape)
                elif pair(address,other):
                    count_item=next(row for row in data if row['name']==shape['count'])
                    count=int.from_bytes(original['memory'][count_item['original']],'little');assert count<=128
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==count*4
                    for index in range(count):
                        linked(int.from_bytes(left[index*4:index*4+4],'little'),int.from_bytes(right[index*4:index*4+4],'little'),shape)
        elif item.get('global_type_blocks'):
            capacity=global_integer('type_table_used');used=global_integer('next_global_type')-0x1000
            assert capacity in (0,512) and 0<=used<=capacity,'global type block contents need an observed initialized-prefix contract'
            for offset in range(0,item['size'],4):
                source=int.from_bytes(raw[offset:offset+4],'little');destination=int.from_bytes(actual[offset:offset+4],'little')
                if offset==0 and capacity:
                    assert source in image_aliases and image_aliases[source]==destination and image_sizes[source]==2048,'global type block allocation/alias/capacity differs'
                    used_uninitialized.add(source);pair(source,destination)
                    if isinstance(item['global_type_blocks'],dict):
                        left=original['memory'][source];right=bytes.fromhex(native['memory'][str(destination)])
                        assert len(left)==len(right)==2048,'incomplete global type pointer-block capacity'
                        for index in range(used):
                            arena_payload(int.from_bytes(left[index*4:index*4+4],'little'),int.from_bytes(right[index*4:index*4+4],'little'))
                    else:assert used==0,'historical global type block has no initialized-prefix observation'
                else:assert not source and not destination,'unused global type block slot is not zero'
        elif item.get('type_arena'):pass # Proven above, including pointer roles.
        elif item.get('function_table'):
            spec=item['function_table'] if isinstance(item['function_table'],dict) else {}
            stride=spec.get('stride',8);pointer=spec.get('pointer_offset',4)
            assert stride>=4 and 0<=pointer<=stride-4 and item['size']%stride==0
            assert len(raw)==len(actual)==item['size'],'symbol callback table extent differs'
            for offset in range(0,item['size'],stride):
                a=raw[offset:offset+stride];b=actual[offset:offset+stride]
                assert a[:pointer]==b[:pointer] and a[pointer+4:]==b[pointer+4:],'symbol callback kind/reserved differs'
                address=int.from_bytes(a[pointer:pointer+4],'little');other=int.from_bytes(b[pointer:pointer+4],'little')
                if address:
                    function=next(row for row in config['functions'] if row['original']==address)
                    assert other==native['function_addresses'][function['native_symbol']],'symbol callback logical identity differs'
                else:assert other==0,'symbol callback NULL identity differs'
        else:
            assert raw == actual, ('application global differs',item['name'],raw.hex(),actual.hex())
        globals_checked.append(item['name'])
    native_parameters=dict(native['parameters'])
    for item,value in object_parameters:
        actual=native_parameters[item['name']]
        assert actual['null']==(value==0)
        linked(value,actual['address'],item['shape'])
        if value:
            identities=list(aliases)
            parameters[item['name']]=dict(null=False,object_id=identities.index(value)+1)
            native_parameters[item['name']]=dict(null=False,object_id=identities.index(reverse[actual['address']])+1)
        else:
            parameters[item['name']]=native_parameters[item['name']]=dict(null=True)
    assert parameters == native_parameters, ('typed ABI arguments differ',parameters,native_parameters)
    if original_calls:assert original['last_error']==native['last_error'],'frontier LastError differs'
    type_compare_calls_checked=0
    if config.get('observe_recursive_type_compare'):
        before=original['type_compare_calls'];after=native['type_compare_calls']
        assert before and len(before)==len(after),'recursive comparison call inventory differs'
        root=global_integer('type_records');count=global_integer('type_record_count')
        for left,right in zip(before,after):
            offset=left['record']-root
            assert 0<=offset<count*32 and offset%32==0,'recursive comparison record ownership differs'
            expected={key:value for key,value in left.items() if key!='record'}
            expected['record_index']=offset//32
            assert 'different' in expected and expected==right,'recursive comparison operands/zero-result differ'
            assert expected['different']==(expected['left']!=expected['right']),'recursive comparison result contradicts complete inputs'
            type_compare_calls_checked+=1
    lookup_calls_checked=0
    if config.get('observe_global_type_lookup'):
        assert original['lookup_active_depth']==native['lookup_type_active_depth']==0,'unfinished global-type lookup observation'
        before=original['lookup_calls'];after=native['lookup_type_calls']
        assert before and len(before)==len(after),'global-type lookup call inventory differs'
        root=global_integer('type_records');count=global_integer('type_record_count')
        other=int.from_bytes(bytes.fromhex(native['globals']['type_records']),'little')
        sentinel=global_integer('empty_type_record')
        actual_sentinel=int.from_bytes(bytes.fromhex(native['globals']['empty_type_record']),'little')
        for left,right in zip(before,after):
            assert all(left[key]==right[key] for key in ('depth','index','parent_argument_alias','output')),'global-type lookup ABI/output sequence differs'
            result=left['result'];actual_result=right['result']
            if result==sentinel:assert actual_result==actual_sentinel,'global-type lookup sentinel return differs'
            else:
                offset=result-root
                assert 0<=offset<count*32 and offset%32==0 and actual_result==other+offset,'global-type lookup returned record identity differs'
            lookup_calls_checked+=1
    return dict(parameters=parameters,caller_state=caller_state,globals=globals_checked,lookup_type_calls_checked=lookup_calls_checked,type_compare_calls_checked=type_compare_calls_checked,aliased_objects_checked=len(aliases),file_handles_checked=len(handle_aliases),mapped_views_checked=len(view_roles),reads_checked=len(reads),uninitialized_allocations_checked=len(used_uninitialized),allocator_requests_checked=len(image_aliases),initialized_module_buffers_checked=len(module_aliases),typed_record_entries_checked=typed_records_checked,extended_type_references_checked=extended_references_checked,recursive_type_nodes_checked=recursive_type_nodes_checked,interned_type_nodes_checked=interned_type_nodes_checked,pooled_interned_nodes_checked=pooled_interned_nodes_checked,cleared_allocator_requests_checked=len(original['cleared_allocations']),cached_type_names_checked=cached_type_names_checked)
