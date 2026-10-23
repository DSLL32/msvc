"""Canonical dynamic CRT data exports used by the reached case."""
from binary import PE,u16,u32
from mask import ROOT,sha

PROVIDER=ROOT/'toolchains/5.10/bin/MSVCRT.DLL'
EXPECTED='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'

def verify(records):
    pe=PE(PROVIDER);assert len(pe.data)==277776 and sha(pe.data)==EXPECTED
    directory=pe.base+u32(pe.data,pe.optional+96)
    header=pe.read(directory,40)
    count=u32(header,24)
    functions=pe.base+u32(header,28);names=pe.base+u32(header,32);ordinals=pe.base+u32(header,36)
    exports={}
    for i in range(count):
        name=pe.string(pe.base+u32(pe.read(names+4*i,4),0))
        slot=u16(pe.read(ordinals+2*i,2),0)
        exports[name]=u32(pe.read(functions+4*slot,4),0)
    for item in records:
        offset=32 if item['kind']=='file' else 0
        assert exports[item['export']]+offset==item['rva'],('wrong CRT export data address',item)
    return dict(provider_sha256=EXPECTED,exports={r['export']:exports[r['export']] for r in records})
