"""Pure byte parser for resource PE and map clocks; never execute a program."""
import struct,re

def timestamp_fields(data,kind):
    fields=[]
    if kind=='map':
        matches=list(re.finditer(rb'^ Timestamp is ([0-9a-f]{8}) \(([^\r\n]{24})\)\r?$',data,re.M))
        assert len(matches)==1
        for group,name in [(1,'Map PE timestamp hexadecimal'),(2,'Map timestamp ctime rendering')]:
            fields.append(dict(offset=matches[0].start(group),bytes=matches[0].end(group)-matches[0].start(group),name=name))
        return fields
    assert kind=='resource-pe'
    word=lambda offset:struct.unpack_from('<H',data,offset)[0]
    dword=lambda offset:struct.unpack_from('<I',data,offset)[0]
    assert data[:2]==b'MZ';pe=dword(60);assert data[pe:pe+4]==b'PE\0\0'
    optional=pe+24;assert word(optional)==0x10b
    sections=optional+word(pe+20)
    def raw(rva):
        for index in range(word(pe+6)):
            start=sections+40*index
            if dword(start+12)<=rva<dword(start+12)+dword(start+16):
                return dword(start+20)+rva-dword(start+12)
        raise AssertionError('Resource RVA outside file-backed sections')
    fields.append(dict(offset=pe+8,bytes=4,name='PE COFF TimeDateStamp'))
    rva,size=struct.unpack_from('<II',data,optional+96+2*8)
    assert rva and size>=16;root=raw(rva);seen=set()
    def directory(relative):
        assert relative not in seen and 0<=relative and relative+16<=size
        seen.add(relative);offset=root+relative
        fields.append(dict(offset=offset+4,bytes=4,name='Resource directory TimeDateStamp'))
        count=word(offset+12)+word(offset+14)
        assert relative+16+8*count<=size
        for index in range(count):
            target=dword(offset+16+8*index+4)
            if target&0x80000000:directory(target&0x7fffffff)
            else:assert target+16<=size
    directory(0)
    return sorted(fields,key=lambda x:x['offset'])

def normalized(data,fields):
    result=bytearray(data)
    for field in fields:result[field['offset']:field['offset']+field['bytes']]=bytes(field['bytes'])
    return bytes(result)
