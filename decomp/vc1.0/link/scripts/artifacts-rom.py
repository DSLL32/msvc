"""Pure parser for captured i386 ROM COFF and MAP clock fields."""
import struct,re

def timestamp_fields(data,kind):
    if kind=='raw':return []
    if kind=='map':
        matches=list(re.finditer(rb'^ Timestamp is ([0-9a-f]{8}) \(([^\r\n]{24})\)\r?$',data,re.M))
        assert len(matches)==1
        return [dict(offset=matches[0].start(i),bytes=matches[0].end(i)-matches[0].start(i),name=n) for i,n in [(1,'Map COFF timestamp hexadecimal'),(2,'Map timestamp ctime rendering')]]
    assert kind=='i386-rom' and len(data)>=244
    machine,sections,stamp,symbols,count,optional,flags=struct.unpack_from('<HHIIIHH',data)
    assert machine==0x14c and sections and optional==224
    assert struct.unpack_from('<H',data,20)[0]==0x10b
    assert 20+optional+sections*40<=len(data)
    for index in range(sections):
        entry=20+optional+index*40
        size,offset=struct.unpack_from('<II',data,entry+16)
        assert offset+size<=len(data)
    return [dict(offset=4,bytes=4,name='ROM COFF TimeDateStamp')]

def normalized(data,fields):
    result=bytearray(data)
    for f in fields:result[f['offset']:f['offset']+f['bytes']]=bytes(f['bytes'])
    return bytes(result)
