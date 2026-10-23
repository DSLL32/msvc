"""Compare contextual callback records by exact fields and target identity."""
def describe(record,raw,functions):
    assert record['stride']==8 and record['pointer_offset']==4 and record['size']==256
    assert len(raw)==256
    targets={r['original']:r['native']for r in functions};result=[]
    for offset in range(0,len(raw),8):
        value=int.from_bytes(raw[offset+4:offset+8],'little')
        assert value==0 or value in targets,'unknown callback table target'
        result.append(dict(fields=raw[offset:offset+4].hex(),function=targets[value]if value else None))
    return result

def check_rebuilt(record,original,rebuilt,functions,linked):
    expected=describe(record,original,functions)
    by_native={r['native']:r for r in functions}
    assert len(rebuilt)==len(original)
    for index,item in enumerate(expected):
        raw=rebuilt[index*8:index*8+8]
        assert raw[:4].hex()==item['fields'],'callback record nonpointer fields differ'
        pointer=int.from_bytes(raw[4:8],'little')
        assert pointer==(linked[by_native[item['function']]['symbol']]if item['function']else 0),'callback table target identity differs'
    return expected
