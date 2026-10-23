"""Clock fields and structural evidence for native-produced cross import archives."""
import struct


def coff_info(data):
    assert len(data)>=20
    machine,count,stamp,symbols,nsyms,optional,flags=struct.unpack_from('<HHIIIHH',data)
    assert machine in (0x14d,0x162) and count
    assert 20+optional+40*count<=len(data)
    sections=[]
    for i in range(count):
        pos=20+optional+i*40
        name=data[pos:pos+8].rstrip(b'\0').decode('ascii')
        size,raw,relocs=struct.unpack_from('<III',data,pos+16)
        nrelocs=struct.unpack_from('<H',data,pos+32)[0]
        assert raw+size<=len(data) and relocs+nrelocs*10<=len(data)
        relocation_records=[struct.unpack_from('<IIH',data,relocs+j*10) for j in range(nrelocs)]
        sections.append(dict(name=name,bytes=size,offset=raw,relocations=relocation_records))
    assert symbols+nsyms*18+4<=len(data)
    strings=symbols+nsyms*18
    length=struct.unpack_from('<I',data,strings)[0]
    assert length>=4 and strings+length<=len(data)
    names=[];i=0
    while i<nsyms:
        pos=symbols+i*18
        if data[pos:pos+4]==bytes(4):
            offset=struct.unpack_from('<I',data,pos+4)[0]
            assert 4<=offset<length
            begin=strings+offset;end=data.index(0,begin,strings+length)
            name=data[begin:end]
        else:name=data[pos:pos+8].split(b'\0')[0]
        names.append(name.decode('latin1'))
        i+=1+data[pos+17]
    assert i==nsyms
    return dict(machine=machine,stamp=stamp,flags=flags,sections=sections,symbols=names)


def archive_members(data):
    assert data[:8]==b'!<arch>\n'
    cursor=8;members=[]
    while cursor<len(data):
        assert cursor+60<=len(data) and data[cursor+58:cursor+60]==b'`\n'
        name=data[cursor:cursor+16].rstrip(b' ')
        size=int(data[cursor+48:cursor+58].strip(b' \0'))
        start=cursor+60
        assert start+size<=len(data)
        members.append(dict(header=cursor,name=name.decode('ascii'),offset=start,bytes=size))
        cursor=start+size+(size&1)
    assert cursor==len(data)
    return members


def timestamp_fields(data,kind):
    fields=[]
    def coff(base,size):
        info=coff_info(data[base:base+size])
        fields.append(dict(offset=base+4,bytes=4,name='COFF TimeDateStamp'))
        for section in info['sections']:
            if section['name']=='.edata':
                assert section['bytes']>=40
                pos=base+section['offset']+4
                assert struct.unpack_from('<I',data,pos)[0]==info['stamp']
                fields.append(dict(offset=pos,bytes=4,name='Export directory TimeDateStamp'))
    if kind=='coff':coff(0,len(data))
    else:
        assert kind=='archive'
        for member in archive_members(data):
            pos=member['header']
            # Native build_archive_header writes the ten-digit time into the
            # fixed "0         0 " template. The final0/space are not clocks.
            assert data[pos+16:pos+26].isdigit() and data[pos+26:pos+28]==b'0 '
            fields.append(dict(offset=pos+16,bytes=10,name='Archive member time digits'))
            if member['name'] not in ('/','//'):coff(member['offset'],member['bytes'])
    return sorted(fields,key=lambda f:f['offset'])


def normalized(data,fields):
    result=bytearray(data)
    for field in fields:
        lo,size=field['offset'],field['bytes']
        assert 0<=lo<=lo+size<=len(result)
        result[lo:lo+size]=bytes(size)
    return bytes(result)
