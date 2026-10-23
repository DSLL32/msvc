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
    file_calls=[];all_strings={};last_error=None;handles={}
    for line in lines:
        if line.startswith('file-open-arguments '):
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
                stack=struct.unpack('<32I',stack),file_calls=file_calls,file_handles=handles,last_error=last_error)


def compare(config,data,events,native,artifacts=None):
    original = decode(events)
    record = next(r for r in config['functions'] if r['original']==original['address'])
    assert native['function'] == record['native_symbol']
    parameters = {};object_parameters=[]
    assert 'parameters' in record, 'frontier lacks explicit ABI/parameter schema'
    for item in record['parameters']:
        value = original['stack'][item['slot']]
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
        elif item['kind'] == 'object-pointer':
            object_parameters.append((item,value))
        else:
            raise AssertionError('unimplemented parameter kind: '+item['kind'])
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

    def pair(address,other):
        assert bool(address)==bool(other),'pointer NULL state differs'
        if not address:return False
        if address in aliases:
            assert aliases[address]==other and reverse[other]==address,'object alias differs'
            return False
        assert other not in reverse,'native object aliases distinct original objects'
        aliases[address]=other;reverse[other]=address
        return True
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
            if 'shape' not in item:
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
    return dict(parameters=parameters,globals=globals_checked,aliased_objects_checked=len(aliases),file_handles_checked=len(handle_aliases))
