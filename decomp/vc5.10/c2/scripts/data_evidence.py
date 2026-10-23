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
            'objects':len(value['objects']),'node_size_dispatch':node_sizes,'intrinsics':intrinsics,'sides':['original PE','rebuilt Win32 PE'],
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
