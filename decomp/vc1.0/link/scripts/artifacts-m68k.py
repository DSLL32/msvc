"""Captured Mac PE output: identify only the structural COFF clock field."""
import struct

def timestamp_fields(data,kind):
    assert kind=='m68k-pe',kind
    assert len(data)>=64 and data[:2]==b'MZ'
    pe=struct.unpack_from('<I',data,60)[0]
    assert pe+24<=len(data) and data[pe:pe+4]==b'PE\0\0'
    assert struct.unpack_from('<H',data,pe+4)[0]==0x268
    optional=struct.unpack_from('<H',data,pe+20)[0]
    assert optional==224 and pe+24+optional<=len(data)
    assert struct.unpack_from('<H',data,pe+24)[0]==0x10b
    return [dict(offset=pe+8,bytes=4,name='PE COFF TimeDateStamp')]

def normalize_artifact(data,kind):
    image=bytearray(data)
    for field in timestamp_fields(image,kind):
        image[field['offset']:field['offset']+field['bytes']]=bytes(field['bytes'])
    return bytes(image)
