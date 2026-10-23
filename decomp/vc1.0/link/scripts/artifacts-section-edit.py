"""Exact section-edit artifacts: no clock fields are ignored in this workflow."""
from array import array
import struct
import sys


def checksum(data, optional):
    image=bytearray(data)
    image[optional+64:optional+68]=bytes(4)
    if len(image)&1:image.append(0)
    words=array('H',image)
    if sys.byteorder!='little':words.byteswap()
    total=sum(words)
    while total>>16:total=(total&65535)+(total>>16)
    return total+len(data)


def timestamp_fields(data, kind):
    assert kind in ('raw','section-release') and len(data)==4194304
    assert data[:2]==b'MZ'
    pe=struct.unpack_from('<I',data,60)[0]
    assert data[pe:pe+4]==b'PE\0\0'
    assert struct.unpack_from('<H',data,pe+4)[0]==0x14c
    optional=pe+24
    assert struct.unpack_from('<H',data,optional)[0]==0x10b
    if kind=='section-release':
        assert struct.unpack_from('<I',data,optional+64)[0]==checksum(data,optional), 'Raw whole-file checksum is incorrect'
    return []


def normalized(data, fields):
    assert fields==[]
    return bytes(data)
