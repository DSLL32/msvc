"""Structural clock fields for the original-backed Mac loader/MAP workflow."""
import re
import struct

def timestamp_fields(data,kind):
    if kind=='m68k-pe':
        assert data[:2]==b'MZ';pe=struct.unpack_from('<I',data,60)[0]
        assert data[pe:pe+4]==b'PE\0\0'
        assert struct.unpack_from('<H',data,pe+4)[0]==0x268
        assert struct.unpack_from('<H',data,pe+20)[0]==224
        assert struct.unpack_from('<H',data,pe+24)[0]==0x10b
        return [dict(offset=pe+8,bytes=4,name='PE COFF TimeDateStamp')]
    assert kind=='map'
    match=list(re.finditer(rb'^ Timestamp is ([0-9a-f]{8}) \(([^\r\n]{24})\)\r?$',data,re.M))
    assert len(match)==1
    return [dict(offset=match[0].start(i),bytes=match[0].end(i)-match[0].start(i),name=name) for i,name in [(1,'MAP timestamp hexadecimal'),(2,'MAP timestamp ctime rendering')]]

def normalized(data,fields):
    result=bytearray(data)
    for f in fields:
        start,size=f['offset'],f['bytes'];assert 0<=start<=start+size<=len(result)
        result[start:start+size]=bytes(size)
    return bytes(result)
