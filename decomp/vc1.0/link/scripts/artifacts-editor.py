"""Pure PE clock normalization with independently checked stored checksums."""
import struct
import sys
from array import array

def layout(data):
    assert data[:2]==b'MZ' and len(data)>=64
    pe=struct.unpack_from('<I',data,60)[0]
    assert data[pe:pe+4]==b'PE\0\0'
    assert struct.unpack_from('<H',data,pe+4)[0]==0x14c
    optional=pe+24
    assert struct.unpack_from('<H',data,optional)[0]==0x10b
    assert optional+struct.unpack_from('<H',data,pe+20)[0]<=len(data)
    return pe,optional

def checksum(data):
    image=bytearray(data)
    _,optional=layout(image)
    image[optional+64:optional+68]=bytes(4)
    if len(image)&1:image.append(0)
    words=array('H',image)
    if sys.byteorder!='little':words.byteswap()
    value=sum(words)
    while value>>16:value=(value&65535)+(value>>16)
    return value+len(data)

def timestamp_fields(data,kind):
    if kind=='raw':return []
    if kind=='compact-release-defect':
        assert len(data)==2097152, 'Original physical growth must remain exact'
        _,optional=layout(data)
        stored=struct.unpack_from('<I',data,optional+64)[0]
        assert stored==checksum(data[:3072]), 'Original checksum is over logical3072 span'
        assert stored!=checksum(data), 'Original physical-file checksum defect must remain visible'
        return []
    assert kind=='edited-pe'
    pe,optional=layout(data)
    assert struct.unpack_from('<I',data,optional+64)[0]==checksum(data), 'Raw PE checksum is incorrect'
    return [dict(offset=pe+8,bytes=4,name='PE COFF TimeDateStamp')]

def normalize_artifact(data,kind):
    image=bytearray(data)
    for field in timestamp_fields(data,kind):
        image[field['offset']:field['offset']+field['bytes']]=bytes(field['bytes'])
    if kind=='edited-pe':
        _,optional=layout(data)
        struct.pack_into('<I',image,optional+64,checksum(image))
    return bytes(image)

def inspect_image(data):
    """Structural evidence of actual relocation, import and export work."""
    pe,optional=layout(data)
    u16=lambda p:struct.unpack_from('<H',data,p)[0]
    u32=lambda p:struct.unpack_from('<I',data,p)[0]
    sections=[]
    for index in range(u16(pe+6)):
        p=optional+u16(pe+20)+index*40
        sections.append(dict(name=data[p:p+8].rstrip(b'\0').decode('ascii'),
                             virtual_size=u32(p+8),rva=u32(p+12),size=u32(p+16),offset=u32(p+20)))
    def file_offset(rva):
        if rva<u32(optional+60):return rva
        section=next(s for s in sections if s['rva']<=rva<s['rva']+max(s['size'],s['virtual_size']))
        offset=rva-section['rva']+section['offset']
        assert offset<len(data)
        return offset
    def string(rva):return data[file_offset(rva):].split(b'\0')[0].decode('ascii')
    directories=[(u32(optional+96+i*8),u32(optional+100+i*8)) for i in range(16)]
    relocations=[]
    rva,size=directories[5]
    if size:
        cursor=file_offset(rva);end=cursor+size
        while cursor<end:
            page,block=u32(cursor),u32(cursor+4)
            assert block>=8 and cursor+block<=end
            for p in range(cursor+8,cursor+block,2):
                word=u16(p)
                if word>>12:
                    assert word>>12==3
                    target=page+(word&4095);offset=file_offset(target)
                    relocations.append(dict(type=3,rva=target,file_offset=offset,value=u32(offset)))
            cursor+=block
    imports=[]
    rva,size=directories[1]
    if size:
        cursor=file_offset(rva)
        while any(struct.unpack_from('<IIIII',data,cursor)):
            lookup,stamp,chain,name,iat=struct.unpack_from('<IIIII',data,cursor)
            p=file_offset(lookup or iat);q=file_offset(iat);names=[];values=[]
            while u32(p):
                item=u32(p)
                names.append('ordinal:'+str(item&65535) if item&0x80000000 else string(item+2))
                values.append(u32(q));p+=4;q+=4
            imports.append(dict(dll=string(name),names=names,iat_values=values,timestamp=stamp,forwarder_chain=chain))
            cursor+=20
    exports=[]
    rva,size=directories[0]
    if size:
        p=file_offset(rva);count=u32(p+24);functions=u32(p+28);names=u32(p+32);ordinals=u32(p+36)
        for i in range(count):
            ordinal=u16(file_offset(ordinals)+2*i)
            exports.append(dict(name=string(u32(file_offset(names)+4*i)),rva=u32(file_offset(functions)+4*ordinal),ordinal=u32(p+16)+ordinal))
    return dict(bytes=len(data),image_base=u32(optional+28),timestamp=u32(pe+8),checksum=u32(optional+64),
                independently_computed_checksum=checksum(data),stack_reserve=u32(optional+72),stack_commit=u32(optional+76),
                relocations=relocations,imports=imports,exports=exports,sections=sections)
