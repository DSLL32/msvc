"""Verify contextual option records/aliases against both original and rebuilt data."""
import json
import re
import struct
from state import BINARY, TARGET, sha


def reader(path):
    data=path.read_bytes();header=struct.unpack_from('<I',data,60)[0];optional=header+24
    base=struct.unpack_from('<I',data,optional+28)[0];count=struct.unpack_from('<H',data,header+6)[0]
    sections=optional+struct.unpack_from('<H',data,header+20)[0]
    def read(address,size):
        for n in range(count):
            virtual,rva,raw,file=struct.unpack_from('<IIII',data,sections+40*n+8)
            delta=address-base-rva
            if 0<=delta and delta+size<=max(virtual,raw):
                available=max(0,min(size,raw-delta))
                return data[file+delta:file+delta+available]+b'\0'*(size-available)
        raise AssertionError('unmapped contextual data')
    return read


def cstring(read,address):
    result=bytearray()
    for n in range(4096):
        value=read(address+n,1)
        if value==b'\0':return result.decode('ascii')
        result.extend(value)
    raise AssertionError('unbounded contextual string')


def model(config):
    for record in config['driver']:
        if record['name']in config['accepted']and record.get('requires_data_model'):
            assert config.get('data_model')==record['requires_data_model'],'required contextual data absent'
    if not config.get('data_model'):return None
    value=json.loads((BINARY/config['data_model']).read_text())
    assert value['target_sha256']==sha(TARGET.read_bytes())
    assert len(value['options'])==53
    return value


def plan(value):
    result=[('option-table',value['original'],54*12)]
    result += [(row['native'],row['original'],(len(row['entries'])+1)*12)for row in value['selectors']]
    result += [(row['native'],row['original'],4)for row in value['objects']if row['kind']!='selector']
    return result


def verify(config):
    value=model(config)
    if value is None:return None
    from intrinsic_evidence import verify as verify_intrinsics
    intrinsics=verify_intrinsics(config)
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(BINARY/'build/c2.map').read_text(),re.M)}
    node_sizes=[]
    if '@c2_node_size@4'in config['accepted']:
        original_targets=[0x40d54d,0x40d547,0x40d54d,0x40d553,0x46719d,0x4671a3,0x40d547,0x46719d,
                          0x40d553,0x4671a3,0x40d54d,0x4671a3,0x4671a3,0x40d559,0x4671a3,0x40d55f]
        rebuilt_offsets=[40,47,40,54,61,82,47,61,54,82,40,82,82,68,82,75]
        sizes=[64,68,64,86,70,None,68,70,86,None,64,None,None,110,None,90]
        for side,path in [('original',TARGET),('rebuilt',BINARY/'build/c2.exe')]:
            read=reader(path);start=0x40d534 if side=='original'else maps['@c2_node_size@4']
            table=0x40d568 if side=='original'else start+103
            targets=struct.unpack('<16I',read(table,64))
            expected=original_targets if side=='original'else[start+n for n in rebuilt_offsets]
            assert list(targets)==expected,'node-size dispatch target identities differ'
            for target,size in zip(targets,sizes):
                if size is not None:
                    assert read(target,5)==b'\xb8'+struct.pack('<I',size),'node-size case result differs'
                    if side=='original':assert read(target+5,1)==b'\xc3'
                else:
                    assert read(target,5)==b'\xba'+struct.pack('<I',1514),'node-size invalid-kind line differs'
                    assert read(target+5,1)==b'\xb9'
                    source=struct.unpack('<I',read(target+6,4))[0]
                    assert cstring(read,source)==r'E:\utc\src\\P2\p2symtab.c','node-size invalid-kind source differs'
            node_sizes.append({'side':side,'table':table,'sha256':sha(read(table,64)),'cases':sizes})
    symbol_dispatch=[]
    record_dispatch=[]
    name_dispatch=[]
    if '@update_symbol_storage@4'in config['accepted']:
        for side,path in [('original',TARGET),('rebuilt',BINARY/'build/c2.exe')]:
            read=reader(path);start=0x4283e3 if side=='original'else maps['@update_symbol_storage@4'];table=0x4285cc if side=='original'else start+0x3c0
            targets=struct.unpack('<7I',read(table,28));expected=[0x42849f,0x428428,0x42849f,0x428428,0x46bf6b,0x46bf6b,0x42841a]if side=='original'else[start+n for n in (0x5e,0x6d,0x5e,0x6d,0x152,0x152,0x141)]
            assert list(targets)==expected,'symbol storage seven-way target table differs'
        for at,name in [(0x490b78,'_compiling_function'),(0x4973f8,'_default_storage_section')]:
            assert reader(TARGET)(at,4)==bytes(4)and reader(BINARY/'build/c2.exe')(maps[name],4)==bytes(4),'storage initial typed pointer differs'
    if '@record_filename@4'in config['accepted']:
        assert reader(TARGET)(0x4973b8,4)==bytes(4),'original filename registry initial pointer differs'
        assert reader(BINARY/'build/c2.exe')(maps['_filenames'],4)==bytes(4),'rebuilt filename registry initial pointer differs'
    if '@intern_symbol_name@12'in config['accepted']:
        record=next(r for r in config['driver']if r['name']=='@intern_symbol_name@12')
        raw=(BINARY/record['name_model']).read_bytes();assert sha(raw)==record['name_model_sha256'],'name model differs'
        layout=json.loads(raw);assert layout['target_sha256']==sha(TARGET.read_bytes())
        for side,path in [('original',TARGET),('rebuilt',BINARY/'build/c2.exe')]:
            read=reader(path);start=0x420ebb if side=='original'else maps['@intern_symbol_name@12']
            outer,classes,inner,inner_classes=(0x420fcc,0x420fe4,0x420ff0,0x421014)if side=='original'else(start+0x1a4,start+0x1bc,start+0x1c7,start+0x1eb)
            targets=struct.unpack('<6I',read(outer,24));expected=layout['outer']['original_targets']if side=='original'else[start+n for n in layout['outer']['rebuilt_offsets']]
            assert list(targets)==expected and list(read(classes,11))==layout['outer']['classes'],'symbol-name kind dispatch differs'
            count,size=9,29
            targets=struct.unpack('<%dI'%count,read(inner,4*count));expected=layout['inner']['original_targets']if side=='original'else[start+n for n in layout['inner']['rebuilt_offsets']]
            assert list(targets)==expected,'symbol-name storage target identities differ'
            classification=list(read(inner_classes,size));assert classification==layout['inner'][side+'_classes'],'symbol-name storage classes differ'
            actions=['underscore','at','uppercase','uppercase','underscore','at','at','underscore','plain']
            assert [actions[classification[n]]if n<size else'plain'for n in range(29)]==layout['inner']['actions'],'symbol-name storage semantics differ'
            pointer=layout['workspace']['original']if side=='original'else maps['_decorated_name']
            assert read(pointer,2059)==bytes(2059),'decoration workspace initial state differs'
            loads=layout['import_prefix'][side+'_loads'];addresses=[]
            for n,at in enumerate(loads):
                assert read(start+at,2)==bytes.fromhex(layout['import_prefix']['opcodes'][n]),'import prefix load changed'
                addresses.append(struct.unpack('<I',read(start+at+2,4))[0])
            assert addresses[1]==addresses[0]+4 and read(addresses[0],6)==b'__imp_','contextual import-name prefix bytes differ'
            name_dispatch.append({'side':side,'kind_targets_sha256':sha(read(outer,24)),'kind_classes_sha256':sha(read(classes,11)),
                'storage_targets_sha256':sha(read(inner,4*count)),'storage_classes_sha256':sha(read(inner_classes,size)),
                'workspace_minimum_bytes':2059,'import_prefix_sha256':sha(read(addresses[0],6)),'model_sha256':sha(raw)})
    if '@read_symbol@4'in config['accepted']:
        record=next(r for r in config['driver']if r['name']=='@read_symbol@4')
        raw=(BINARY/record['record_model']).read_bytes();assert sha(raw)==record['record_model_sha256'],'decoder layout model differs'
        layout=json.loads(raw)
        assert layout['target_sha256']==sha(TARGET.read_bytes())
        for side,path in [('original',TARGET),('rebuilt',BINARY/'build/c2.exe')]:
            read=reader(path);start=0x4206b7 if side=='original'else maps['@read_symbol@4']
            table=0x420e18 if side=='original'else start+0xcea
            targets=struct.unpack('<23I',read(table,92))
            expected=layout['dispatch']['original_targets']if side=='original'else[start+n for n in layout['dispatch']['rebuilt_offsets']]
            assert list(targets)==expected,'symbol record dispatch identities differ'
            for original,name,size in [(0x497308,'symbol_template',64),(0x496a00,'symbol_name',2058),(0x48e020,'c2_pools',60),
                                       (0x49739c,'last_section_serial',4),(0x4973b0,'il_option_flags',4),
                                       (0x4973a0,'auxiliary_prefix',4),(0x497390,'aux_expression_file',4),
                                       (0x497380,'aux_symbol_file',4),(0x497c6c,'c2_constant_strings',4),
                                       (0x4973c0,'c2_omit_frame_pointer',4),(0x495f1c,'current_symbol',4)]:
                address=original if side=='original'else maps['_'+name]
                assert read(address,size)==bytes(size),'symbol decoder initial contextual object differs: '+name
            record_dispatch.append({'side':side,'table':table,'sha256':sha(read(table,92)),'kinds':23,'contextual_initial_objects':11,'layout_sha256':sha(raw)})
    if '@read_symbols@4'in config['accepted']:
        classes=bytes([0,10,1,2,3,10,4,4,5,10,6,10,7,10,10,10,10,8,10,10,10,9,9])
        original_targets=[0x420415,0x4204df,0x4204c1,0x4204df,0x4204df,0x4203f0,0x42058f,0x4205ae,0x420611,0x420506,0x46a15f]
        rebuilt_offsets=[0xc5,0x1d8,0x1e8,0x1d8,0x1d8,0x2aa,0x2e6,0x314,0x39a,0x3a7,0x3ac]
        for side,path in [('original',TARGET),('rebuilt',BINARY/'build/c2.exe')]:
            read=reader(path);start=0x420394 if side=='original'else maps['@read_symbols@4']
            targets_address,classes_address=(0x42066c,0x4206a0)if side=='original'else(start+0x448,start+0x474)
            targets=struct.unpack('<11I',read(targets_address,44))
            expected=original_targets if side=='original'else[start+n for n in rebuilt_offsets]
            assert list(targets)==expected,'symbol reader case target identities differ'
            assert read(classes_address,23)==classes,'symbol reader kind/class mapping differs'
            symbol_dispatch.append({'side':side,'targets':targets_address,'target_sha256':sha(read(targets_address,44)),'classes':classes_address,'class_sha256':sha(classes),'kinds':23,'blocks':11})
    for side,path in [('original',TARGET),('rebuilt',BINARY/'build/c2.exe')]:
        read=reader(path)
        def address(name):
            if side=='rebuilt':return maps['_'+name]
            return next(row['original']for row in value['objects']if row['native']==name)
        start=value['original']if side=='original'else maps['_'+value['native']]
        for n,row in enumerate(value['options']):
            pattern,destination,result,kind,reserved=struct.unpack('<IIBBH',read(start+n*12,12))
            assert cstring(read,pattern)==row['pattern']
            assert destination==address(row['value'])
            assert (result,kind,reserved)==(row['result'],row['kind'],row['reserved'])
        assert read(start+53*12,12)==b'\0'*12
        for table in value['selectors']:
            start=table['original']if side=='original'else maps['_'+table['native']]
            for n,row in enumerate(table['entries']):
                selector,kind,destination=struct.unpack('<III',read(start+n*12,12))
                assert selector==ord(row['selector'])and kind==row['kind']and destination==address(row['value'])
            assert read(start+len(table['entries'])*12,12)==b'\0'*12
        if any(r['name']=='_initialize_messages'and r['name']in config['accepted']for r in config['driver']):
            import struct as _struct
            table=0x49e1d0 if side=='original'else maps['_messages']
            flag=0x49e650 if side=='original'else maps['_messages_initialized']
            buffer=0x49e4d0 if side=='original'else maps['_message_buffer']
            assert read(table,56)==b''.join(_struct.pack('<II',n,0)for n in (301,302,303,310,311,312,0)),'contextual message table differs'
            assert read(flag,4)==bytes(4)and read(buffer,384)==bytes(384),'message cache initial storage differs'
        original_read=reader(TARGET)
        for row in value['objects']:
            if row['kind']=='selector':continue
            actual=struct.unpack('<I',read(address(row['native']),4))[0]
            if row['kind']=='int':assert actual==row['initial'],row['native']+' initial scalar differs'
            elif row['initial']==0:assert actual==0
            else:assert cstring(read,actual)==cstring(original_read,row['initial'])
    return {'model_sha256':sha((BINARY/config['data_model']).read_bytes()),'option_records':53,
            'selector_records':sum(len(t['entries'])for t in value['selectors']),
            'objects':len(value['objects']),'node_size_dispatch':node_sizes,'symbol_dispatch':symbol_dispatch,'record_dispatch':record_dispatch,'name_dispatch':name_dispatch,'intrinsics':intrinsics,'sides':['original PE','rebuilt Win32 PE'],
            'checks':['pattern text','destination logical aliases','result/kind/record ABI','selector contents','initial scalar/string values']}


def verify_original_state(value,data,overrides=None):
    read=reader(TARGET);cursor=0
    for name,address,size in plan(value):
        expected=struct.pack('<I',overrides[name])if overrides and name in overrides else read(address,size)
        assert data[cursor:cursor+size]==expected,'original runtime contextual input changed: '+name
        cursor+=size
    assert cursor==len(data),'unexpected contextual input tail'


def verify_native_state(value,state,bias,symbols,overrides=None):
    def address(name):return symbols[name][0]+bias
    expected=[]
    for row in value['options']:
        expected.append({'pattern':row['pattern'],'value':address(row['value']),
                         'result':row['result'],'kind':row['kind'],'reserved':row['reserved']})
    expected.append({'pattern':None,'value':0,'result':0,'kind':0,'reserved':0})
    assert state['options']==expected,'native contextual option rows or destination aliases differ'
    selectors={}
    for table in value['selectors']:
        rows=[{'selector':ord(row['selector']),'kind':row['kind'],'value':address(row['value'])}for row in table['entries']]
        selectors[table['native']]=rows+[{'selector':0,'kind':0,'value':0}]
    assert state['selectors']==selectors,'native contextual selectors differ'
    objects={};read=reader(TARGET)
    for row in value['objects']:
        if row['kind']=='selector':continue
        objects[row['native']]=(cstring(read,row['initial'])if row['initial']else None)if row['kind']=='string'else row['initial']
    if overrides:objects.update(overrides)
    assert state['objects']==objects,'native contextual destination initial values differ'
