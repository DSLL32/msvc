"""Structurally identified OMF archive clocks; all date suffix/padding bytes exact."""
import struct


def timestamp_fields(data,kind):
    assert kind=='archive-generated' and data[:8]==b'!<arch>\n'
    fields=[];cursor=8;members=0
    while cursor<len(data):
        assert cursor+60<=len(data) and data[cursor+58:cursor+60]==b'`\n'
        date=data[cursor+16:cursor+28]
        assert len(date)==12 and date[:10].isdigit() and date[10:]==b'0 '
        fields.append(dict(offset=cursor+16,bytes=10,name='Archive member decimal time input'))
        size=int(data[cursor+48:cursor+58].strip(b' \0'));base=cursor+60;assert base+size<=len(data)
        if data[base:base+2]==b'\x4c\x01':
            assert size>=20 and struct.unpack_from('<H',data,base+16)[0]==0
            fields.append(dict(offset=base+4,bytes=4,name='Generated COFF TimeDateStamp'));members+=1
        cursor=base+size+(size&1)
    assert cursor==len(data) and members==1 and len(fields)==4
    return fields


def normalize_artifact(data,kind):
    result=bytearray(data)
    for field in timestamp_fields(data,kind):
        result[field['offset']:field['offset']+field['bytes']]=bytes(field['bytes'])
    return bytes(result)
