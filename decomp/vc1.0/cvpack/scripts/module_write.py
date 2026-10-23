"""Bind read_types' distinct buffer publication to canonical global owners."""
import hashlib,json
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_MEM,X86_OP_REG
from state import BINARY

def stores(pe,start,end,address):
    decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    return [i for i in decoder.disasm(pe.read(start,end-start),start)
            if i.mnemonic=='mov' and i.operands[0].type==X86_OP_MEM
            and not i.operands[0].mem.base and not i.operands[0].mem.index
            and i.operands[0].mem.disp==address]

def validate(config,original,rebuilt,linked):
    record=next(r for r in config['functions']if r['name']=='read_types')
    if record['status']=='stub':return
    ref=config['module_buffer_publication'];path=BINARY/ref['evidence']
    assert hashlib.sha256(path.read_bytes()).hexdigest()==ref['evidence_sha256'],'stale module publication proof'
    proof=json.loads(path.read_text())
    assert hashlib.sha256((BINARY/proof['source']).read_bytes()).hexdigest()==proof['source_sha256'],'module publication source changed'
    data=json.loads((BINARY/'data.json').read_text())
    owners={name:next(r for r in data if r['name']==name)for name in ('type_data','current_type_map','module_type_buffer')}
    assert {name:(r['original'],r['symbol'],r['size'])for name,r in owners.items()}=={
        'type_data':(0x3305c,'_type_data',4),'current_type_map':(0x3304c,'_current_type_map',4),
        'module_type_buffer':(0x33b50,'_module_type_buffer',4)},'module publication owner differs'
    assert original.read(0x17dfc,10).hex()=='a15c300300a3503b0300','original module publication operand differs'
    assert original.read(0x17d80,0x92).hex()==proof['original_body'],'original read_types body differs'
    for pe,start,end,addresses in ((original,0x17d80,0x17e12,{n:r['original']for n,r in owners.items()}),
             (rebuilt,linked['_read_types'],min(v for v in linked.values()if v>linked['_read_types']),{n:linked[r['symbol']]for n,r in owners.items()})):
        writes=stores(pe,start,end,addresses['module_type_buffer'])
        assert len(writes)==1 and writes[0].operands[1].type==X86_OP_REG,'module buffer publication store missing'
        assert not stores(pe,start,end,addresses['current_type_map']),'read_types writes wrong current_type_map global'
        code=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(pe.read(start,end-start),start));index=next(j for j,i in enumerate(code)if i.address==writes[0].address)
        predecessor=code[index-1]
        assert predecessor.mnemonic=='mov' and predecessor.op_str=='eax, dword ptr [0x%x]'%addresses['type_data'],'module buffer publication source differs'
