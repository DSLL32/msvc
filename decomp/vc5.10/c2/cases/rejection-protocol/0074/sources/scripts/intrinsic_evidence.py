"""Encountered intrinsic fields/text aliases; no instruction or CFG discovery."""
import json
import re
import struct
from state import BINARY, TARGET, sha


def model(config):
    if '_c2_initialize_opcodes'not in config['accepted']:return None
    path=BINARY/config['intrinsic_model'];data=path.read_bytes()
    assert sha(data)==config['intrinsic_model_sha256'],'intrinsic model changed'
    value=json.loads(data)
    assert value['target_sha256']==sha(TARGET.read_bytes())
    assert value['count']==344 and sorted(r['number']for r in value['rows'])==list(range(344))
    return value


def overrides(value):
    empty=value['default_text']
    return [r for r in value['rows']if (r['result_type'],r['commands'],r['runtime_name'],r['flags'],r['argument_signature'])!=(0,empty,empty,0,0)]


def verify_text(value,read,identity,pointer,aliases):
    texts={r['original']:bytes.fromhex(r['bytes'])for r in value['texts']}
    assert identity in texts
    if identity in aliases:assert aliases[identity]==pointer,'shared intrinsic text lost its address identity'
    else:
        assert pointer not in aliases.values(),'distinct intrinsic text identities collapsed'
        aliases[identity]=pointer
    expected=texts[identity]+b'\0'
    assert read(pointer,len(expected))==expected,'intrinsic recipe/runtime bytes differ'


def verify(config):
    value=model(config)
    if value is None:return None
    from data_evidence import reader
    original=reader(TARGET);rebuilt=reader(BINARY/'build/c2.exe')
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(BINARY/'build/c2.map').read_text(),re.M)}
    for i,row in enumerate(value['rows']):
        number,typ,commands,runtime,flags,signature=struct.unpack('<HHIIBB',original(value['source_table']+i*16,14))
        assert (number,typ,commands,runtime,flags,signature)==tuple(row[k]for k in ('number','result_type','commands','runtime_name','flags','argument_signature'))
    assert struct.unpack('<h',original(value['source_sentinel'],2))[0]==344
    for row in value['texts']:
        data=bytes.fromhex(row['bytes'])+b'\0'
        assert original(row['original'],len(data))==data
    aliases={};verify_text(value,rebuilt,value['default_text'],maps['_empty_intrinsic_text'],aliases)
    records=overrides(value);assert len(records)==value['nondefault_count']==247
    for i,row in enumerate(records):
        number,typ,commands,runtime,flags,signature=struct.unpack('<HHIIBB',rebuilt(maps['_intrinsic_initializers']+i*16,14))
        assert (number,typ,flags,signature)==tuple(row[k]for k in ('number','result_type','flags','argument_signature'))
        verify_text(value,rebuilt,row['commands'],commands,aliases)
        verify_text(value,rebuilt,row['runtime_name'],runtime,aliases)
    assert struct.unpack('<h',rebuilt(maps['_intrinsic_initializers']+247*16,2))[0]==344
    assert len(aliases)==len(value['texts'])==122
    assert original(value['destination_table'],344*16)==rebuilt(maps['_intrinsics'],344*16)==bytes(344*16),'initial intrinsic destination storage differs'
    return {'model_sha256':sha((BINARY/config['intrinsic_model']).read_bytes()),'original_records':344,
            'explicit_records':247,'default_records':97,'text_identities':122,
            'preserved_fields':['result_type','commands','runtime_name','flags','argument_signature'],
            'untouched_fields':['reserved_type','reserved_flags'],
            'original_ownership':{'source':[value['source_table'],value['source_sentinel']],
                'sentinel_number':[value['source_sentinel'],value['source_sentinel']+2],
                'destination':[value['destination_table'],value['destination_table']+344*16],
                'texts':[[r['original'],r['original']+len(bytes.fromhex(r['bytes']))+1]for r in value['texts']]},
            'rebuilt_source':maps['_intrinsic_initializers'],'rebuilt_destination':maps['_intrinsics']}


def verify_original_state(value,data):
    assert len(data)==344*16
    for row in value['rows']:
        typ,reserved,commands,runtime,flags,signature,tail=struct.unpack_from('<HHIIBBH',data,row['number']*16)
        assert (typ,commands,runtime,flags,signature)==tuple(row[k]for k in ('result_type','commands','runtime_name','flags','argument_signature'))
        assert reserved==tail==0,'original intrinsic reserved fields changed'


def verify_native_state(value,rows):
    assert len(rows)==344
    aliases={};texts={r['original']:bytes.fromhex(r['bytes']).hex()for r in value['texts']}
    for expected in value['rows']:
        row=rows[expected['number']]
        assert (row['result_type'],row['flags'],row['argument_signature'])==tuple(expected[k]for k in ('result_type','flags','argument_signature'))
        assert row['reserved_type']==row['reserved_flags']==0
        for field in ('commands','runtime_name'):
            identity=expected[field];pointer=row[field]
            assert row[field+'_bytes']==texts[identity]+'00'
            if identity in aliases:assert aliases[identity]==pointer,'native intrinsic text sharing differs'
            else:
                assert pointer not in aliases.values(),'native distinct intrinsic text addresses collapsed'
                aliases[identity]=pointer
    assert len(aliases)==122
