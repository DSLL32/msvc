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
    file_calls=[];all_strings={};last_error=None;handles={};mapping_calls=[];mapped_views={};file_reads=[];directory_allocation={};subsection_allocations=[];image_allocations=[];checked_allocations=[];module_allocations=[]
    for line in lines:
        if line.startswith('module-allocation-call '):
            _,entry,size=line.split();module_allocations.append(dict(entry=int(entry,16),size=int(size)))
        elif line.startswith('module-allocation-return '):
            assert module_allocations and 'address' not in module_allocations[-1]
            module_allocations[-1]['address']=int(line.split()[1],16)
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
        elif line.startswith('blob-observation-failed'):raise AssertionError('incomplete initialized allocation observation')
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
    return dict(address=address,memory=memory,strings=strings,
                registers=dict(zip(('eax','ebx','ecx','edx','esi','edi','ebp','esp'),registers)),esp=registers[-1],stack=struct.unpack('<32I',stack),file_calls=file_calls,file_handles=handles,last_error=last_error,mapping_calls=mapping_calls,mapped_views=mapped_views,file_reads=file_reads,directory_allocation=directory_allocation,subsection_allocations=subsection_allocations,image_allocations=image_allocations,checked_allocations=checked_allocations,module_allocations=module_allocations)


def compare(config,data,events,native,artifacts=None,views=None):
    original = decode(events)
    record = next(r for r in config['functions'] if r['original']==original['address'])
    assert native['function'] == record['native_symbol']
    parameters = {};object_parameters=[]
    assert 'parameters' in record, 'frontier lacks explicit ABI/parameter schema'
    for item in record['parameters']:
        value = original['registers'][item['register']] if 'register' in item else original['stack'][item['slot']]
        if item['kind'] == 'integer':
            parameters[item['name']] = value
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
                names=[row['name'] for row in data if row['name'] in item['allowed_globals'] and row['original']==value]
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
            row=next(row for row in data if row['name']=='directory_entries');root=int.from_bytes(original['memory'][row['original']],'little');entry=original['stack'][item['entry_slot']]
            assert entry>=root and (entry-root)%12==0
            payload=original['memory'][root][entry-root:entry-root+12];assert len(payload)==12
            size=int.from_bytes(payload[item['count_offset']:item['count_offset']+4],'little');assert 0<=size<=65536
            assert len(original['memory'][value])==size
            parameters[item['name']]=dict(global_name=names[0],size=size,bytes=original['memory'][value].hex())
        elif item['kind']=='blob-element-pointer':
            row=next(row for row in data if row['name']==item['root']);root=int.from_bytes(original['memory'][row['original']],'little')
            row=next(row for row in data if row['name']==item['count']);count=int.from_bytes(original['memory'][row['original']],'little')
            assert root<=value<root+count*item['element_size'] and (value-root)%item['element_size']==0,'argument is not an original directory element'
            offset=value-root;payload=original['memory'][root][offset:offset+item['element_size']]
            assert len(payload)==item['element_size']
            parameters[item['name']]=dict(root=item['root'],index=offset//item['element_size'],bytes=payload.hex())
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
        else:
            raise AssertionError('unimplemented parameter kind: '+item['kind'])
    reads=[]
    for captured in original['file_reads']:
        row=dict(captured)
        pc=row.pop('pc');sites=[site for site in config.get('read_sites',[]) if site['return_pc']==pc]
        assert len(sites)==1,'unknown actual original read caller'
        site=sites[0];row.pop('buffer')
        if site.get('allocated_global'):
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
               0x4062f7:('type_data','line_capacity'),0x40631b:('module_data','type_capacity'),0x40639d:None}
        assert left['site'] in sites,'unknown actual image allocator caller'
        if sites[left['site']] is not None:
            name,size_name=sites[left['site']]
            row=next(row for row in data if row['name']==name)
            assert int.from_bytes(original['memory'][row['original']],'little')==left['address'],'actual allocation not assigned to its evidenced output'
            row=next(row for row in data if row['name']==size_name)
            assert int.from_bytes(original['memory'][row['original']],'little')==left['size'],'actual allocation request differs from capacity source'
        assert left['size']==right['size'] and left['address'] and right['address'],'actual image allocation request/result differs'
        assert left['address'] not in image_aliases and right['address'] not in image_aliases.values(),'live image allocation identity reused'
        image_aliases[left['address']]=right['address'];image_sizes[left['address']]=left['size']
    assert len(original['checked_allocations'])==len(native.get('checked_allocations',[])),'checked allocator request inventory differs'
    for left,right in zip(original['checked_allocations'],native.get('checked_allocations',[])):
        assert left['size']==right['size'] and left['address'] and right['address'],'checked allocator request/result differs'
        assert left['address'] not in image_aliases and right['address'] not in image_aliases.values(),'live checked allocation identity reused'
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
        while pair(address,other):
            assert len(aliases)<=512,'unbounded object graph'
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
            if item.get('segment_buffer'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                assert bool(address)==bool(other),'type segment buffer NULL state differs'
                if address:
                    row=next(row for row in data if row['name']=='type_segments');table=int.from_bytes(original['memory'][row['original']],'little')
                    base=int.from_bytes(original['memory'][table][:4],'little')
                    assert address==base and image_aliases[base]==other,'type segment buffer/allocator alias differs'
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==item['blob']['size'] and left==right,'initialized type segment prefix differs'
                    pair(address,other)
            elif item.get('public_buckets'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                if pair(address,other):
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==4096*4
                    for offset in range(0,len(left),4):
                        source=int.from_bytes(left[offset:offset+4],'little');destination=int.from_bytes(right[offset:offset+4],'little')
                        while pair(source,destination):
                            owners=[base for base,size in image_sizes.items() if size==0x1fa4 and base<=source<base+size]
                            assert len(owners)==1,'public node has no actual arena ownership'
                            owner=owners[0];assert destination==image_aliases[owner]+source-owner,'public node arena/offset differs'
                            a=original['memory'][source];b=bytes.fromhex(native['memory'][str(destination)])
                            assert len(a)==len(b)==18+int.from_bytes(a[16:18],'little') and source+len(a)<=owner+image_sizes[owner]
                            assert a[4:]==b[4:],'initialized public node header/record/padding differs'
                            source=int.from_bytes(a[:4],'little');destination=int.from_bytes(b[:4],'little')
            elif item.get('interior_allocation'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little');spec=item['interior_allocation']
                assert bool(address)==bool(other),'arena cursor NULL state differs'
                if address:
                    owners=[base for base,size in image_sizes.items() if size==spec['size'] and base<=address<=base+size]
                    assert len(owners)==1,'arena cursor ownership differs'
                    owner=owners[0];row=next(row for row in data if row['name']==spec['remaining'])
                    remaining=int.from_bytes(original['memory'][row['original']],'little')
                    assert address-owner+remaining==spec['size'] and other==image_aliases[owner]+address-owner,'arena cursor/capacity/offset differs'
                    used_uninitialized.add(owner)
            elif item.get('module_buffer'):
                module_buffer(int.from_bytes(raw,'little'),int.from_bytes(actual,'little'))
            elif 'allocation' in item:
                allocation(int.from_bytes(raw,'little'),int.from_bytes(actual,'little'),item['allocation'])
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
    return dict(parameters=parameters,globals=globals_checked,aliased_objects_checked=len(aliases),file_handles_checked=len(handle_aliases),mapped_views_checked=len(view_roles),reads_checked=len(reads),uninitialized_allocations_checked=len(used_uninitialized),allocator_requests_checked=len(image_aliases),initialized_module_buffers_checked=len(module_aliases))
