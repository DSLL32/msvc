"""Compare contextual callback records by exact fields and target identity."""
def describe(record,raw,functions):
    stride,pointer,size=record['stride'],record['pointer_offset'],record['size']
    assert (stride,pointer,size)in((8,4,256),(4,0,8)),'unreviewed callback table layout'
    assert len(raw)==size
    targets={r['original']:r['native']for r in functions};result=[]
    for offset in range(0,len(raw),stride):
        value=int.from_bytes(raw[offset+pointer:offset+pointer+4],'little')
        assert value==0 or value in targets,'unknown callback table target'
        result.append(dict(fields=raw[offset:offset+pointer].hex(),function=targets[value]if value else None))
    return result

def check_rebuilt(record,original,rebuilt,functions,linked):
    expected=describe(record,original,functions)
    by_native={r['native']:r for r in functions}
    assert len(rebuilt)==len(original)
    stride,pointer=record['stride'],record['pointer_offset']
    for index,item in enumerate(expected):
        raw=rebuilt[index*stride:index*stride+stride]
        assert raw[:pointer].hex()==item['fields'],'callback record nonpointer fields differ'
        target=int.from_bytes(raw[pointer:pointer+4],'little')
        assert target==(linked[by_native[item['function']]['symbol']]if item['function']else 0),'callback table target identity differs'
    return expected
