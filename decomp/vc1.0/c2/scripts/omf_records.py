"""Pure structural checks for the original case's checksummed OMF records."""
def records(data):
    result=[];position=0
    while position<len(data):
        assert position+3<=len(data)
        kind=data[position];length=int.from_bytes(data[position+1:position+3],'little');end=position+3+length
        assert length and end<=len(data),(position,kind,length)
        record=data[position:end];assert not(sum(record)&255),(position,kind,'OMF checksum')
        result.append(dict(offset=position,type=hex(kind),bytes=len(record)));position=end
    assert result and result[0]['type']=='0x80'and result[-1]['type']in ['0x8a','0x8b']
    return result

