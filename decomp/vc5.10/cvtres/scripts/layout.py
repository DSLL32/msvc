"""Verified PE loader data ranges, independently of any guessed code layout."""
from binary import PE, u16, u32

def loader_data(pe):
    result=[]
    def add(start,size,kind):
        assert size > 0
        pe.read(start,size)
        result.append(dict(start=start,end=start+size,kind=kind))
    def text(va,prefix=0):
        n=len(pe.string(va))+1
        add(va-prefix,n+prefix,'import-name')
    descriptor=pe.base+u32(pe.data,pe.optional+104)
    declared=u32(pe.data,pe.optional+108)
    add(descriptor,declared,'import-descriptors')
    cursor=descriptor;count=0
    while pe.read(cursor,20)!=b'\0'*20:
        original,_,_,name,iat=__import__('struct').unpack('<IIIII',pe.read(cursor,20))
        text(pe.base+name)
        n=0
        while True:
            item=u32(pe.read(pe.base+(original or iat)+4*n,4),0)
            if not item:break
            if not(item&0x80000000):text(pe.base+item+2,2)
            n+=1
            assert n<1024
        add(pe.base+(original or iat),4*(n+1),'import-lookup')
        add(pe.base+iat,4*(n+1),'import-address-table')
        cursor+=20;count+=1;assert count<64
    assert cursor+20<=descriptor+declared
    debug=pe.base+u32(pe.data,pe.optional+96+6*8)
    size=u32(pe.data,pe.optional+100+6*8)
    if size:add(debug,size,'debug-directory')
    return result

def import_addresses(pe):
    out={};cursor=pe.base+u32(pe.data,pe.optional+104)
    while pe.read(cursor,20)!=b'\0'*20:
        original,_,_,name,iat=__import__('struct').unpack('<IIIII',pe.read(cursor,20))
        dll=pe.string(pe.base+name)
        n=0
        while True:
            item=u32(pe.read(pe.base+(original or iat)+4*n,4),0)
            if not item:break
            function='#%d'%(item&65535) if item&0x80000000 else pe.string(pe.base+item+2)
            out[pe.base+iat+4*n]=(dll,function)
            n+=1
        cursor+=20
    return out
