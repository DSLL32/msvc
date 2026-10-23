"""Clock fields in captured M68K full-COFF-debug output; no target execution."""
import struct


def layout(data):
    assert data[:2]==b'MZ'
    pe=struct.unpack_from('<I',data,60)[0]
    assert data[pe:pe+4]==b'PE\0\0'
    machine,count=struct.unpack_from('<HH',data,pe+4)
    assert machine==0x268
    optional=pe+24
    assert struct.unpack_from('<H',data,optional)[0]==0x10b
    table=optional+struct.unpack_from('<H',data,pe+20)[0]
    sections=[]
    for index in range(count):
        pos=table+40*index
        name=data[pos:pos+8].rstrip(b'\0')
        virtual_size,rva,size,raw=struct.unpack_from('<IIII',data,pos+8)
        assert raw+size<=len(data)
        sections.append((name,rva,virtual_size,size,raw))
    return pe,optional,sections


def timestamp_fields(data,kind):
    assert kind=='mac-debug-pe',kind
    pe,optional,sections=layout(data)
    debug_rva,debug_size=struct.unpack_from('<II',data,optional+96+6*8)
    assert debug_size==28
    debug_section=next(s for s in sections if s[1]<=debug_rva<s[1]+s[3])
    debug=debug_section[4]+debug_rva-debug_section[1]
    assert struct.unpack_from('<I',data,debug+12)[0]==1  # IMAGE_DEBUG_TYPE_COFF
    mac=next(s for s in sections if s[0]==b'.mscv')
    assert data[mac[4]:mac[4]+2]==b'\0\2'
    timestamp=struct.unpack_from('<I',data,pe+8)[0]
    assert struct.unpack_from('<I',data,debug+4)[0]==timestamp
    assert struct.unpack_from('>I',data,mac[4]+2)[0]==timestamp
    return [dict(offset=pe+8,bytes=4,name='PE COFF TimeDateStamp'),
            dict(offset=debug+4,bytes=4,name='COFF debug directory TimeDateStamp'),
            dict(offset=mac[4]+2,bytes=4,name='Mac debug resource big-endian TimeDateStamp')]


def normalize_artifact(data,kind):
    image=bytearray(data)
    for field in timestamp_fields(data,kind):
        image[field['offset']:field['offset']+field['bytes']]=bytes(field['bytes'])
    return bytes(image)
