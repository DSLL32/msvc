"""Scoped ABI proof for three standalone, nonvirtual callback globals.

This is not a class-layout or general pointer normalization rule. The original
and Win32 reconstruction store single code DWORDs; the native compiler uses a
code DWORD plus zero this adjustment. Unknown code or adjustment is rejected.
Common Rust still discovers and audits the complete function graph.
"""
import re
import struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_MEM,X86_OP_IMM

CONTRACTS={
    'image_header_size':dict(slot=0x47f14c,methods={'pe':'LinkContext::PeHeaderSize','vxd':'LinkContext::VxdHeaderSize'},addresses={'pe':0x459d00,'vxd':0x463c90},symbol='?image_header_size@@3P8LinkContext@@AEKPAK@ZQ1@',assignments=[0x4338bc,0x4338e1],calls=[(0x443a35,'8d5424508bcd52ff154cf14700')],cleanup=4),
    'image_write_section':dict(slot=0x47f0f8,methods={'pe':'LinkContext::PeWriteSection','vxd':'LinkContext::VxdWriteSection'},addresses={'pe':0x459d50,'vxd':0x463cd0},symbol='?image_write_section@@3P8LinkContext@@AEXHPAX0@ZQ1@',assignments=[0x4338c6,0x4338eb],calls=[(0x4401a4,'8d5424205256508bcfff15f8f04700'),(0x4424bb,'8d4424385051528bcbff15f8f04700')],cleanup=12),
    'image_write_headers':dict(slot=0x47eed0,methods={'pe':'LinkContext::PeWriteHeaders','vxd':'LinkContext::VxdWriteHeaders'},addresses={'pe':0x459d70,'vxd':0x464470},symbol='?image_write_headers@@3P8LinkContext@@AEXH@ZQ1@',assignments=[0x4338d0,0x4338f5],calls=[(0x437320,'518bcdff15d0ee4700'),(0x444792,'8bcd52ff15d0ee4700'),(0x461989,'528bcfff15d0ee4700')],cleanup=4),
}


def project(original_bytes,native_bytes,expected,contract,methods):
    assert len(original_bytes)==4 and len(native_bytes)==8,'member-pointer width differs'
    original,=struct.unpack('<I',original_bytes)
    code,adjustment=struct.unpack('<Ii',native_bytes)
    assert adjustment==0,'nonzero native this adjustment is outside the reviewed scope'
    if expected is None:
        assert original==code==0,'ambiguous or non-null uninitialized member pointer'
        return None
    assert expected in contract['methods'],'unknown callback mode'
    method=contract['methods'][expected]
    assert original==contract['addresses'][expected],'unknown or swapped original callback'
    assert code==methods[method] and code,'unknown or swapped native callback'
    return method


def check_static(original,rebuilt,map_text,configuration):
    publics={name:int(address,16) for name,address in re.findall(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',map_text,re.M)}
    records={r['name']:r for r in configuration['driver']}
    cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
    start=publics[records['LinkContext::Initialize']['symbol']]
    stop=publics[records['LinkContext::InitializeVxd']['symbol']]
    instructions=list(cs.disasm(rebuilt.read(start,stop-start),start))
    assert sum(i.size for i in instructions)==stop-start
    result={}
    for name,contract in CONTRACTS.items():
        slot=contract['slot'];wanted={contract['addresses'][m] for m in ['pe','vxd']}
        assert original.data.count(struct.pack('<I',slot))==len(contract['assignments'])+len(contract['calls']),'unreviewed slot reference/alias'
        for site,address in zip(contract['assignments'],[contract['addresses']['vxd'],contract['addresses']['pe']]):
            assert original.read(site,10)==b'\xc7\x05'+struct.pack('<II',slot,address),'original callback assignment differs'
        for site,encoded in contract['calls']:
            assert original.read(site,len(bytes.fromhex(encoded)))==bytes.fromhex(encoded),'member receiver/stack/call sequence differs'
        rebuilt_slot=publics[contract['symbol']]
        assigned=[]
        for i in instructions:
            if i.mnemonic!='mov' or len(i.operands)!=2:continue
            left,right=i.operands
            if left.type==X86_OP_MEM and not left.mem.base and not left.mem.index and left.mem.disp==rebuilt_slot:
                assert left.size==4 and right.type==X86_OP_IMM,'Win32 callback is not one direct code DWORD'
                assigned.append(right.imm&0xffffffff)
        expected={publics[records[contract['methods'][m]]['symbol']] for m in ['pe','vxd']}
        assert len(assigned)==2 and set(assigned)==expected,'Win32 callback identities differ'
        for mode in ['pe','vxd']:
            address=publics[records[contract['methods'][mode]]['symbol']]
            # These are named trapping stubs, not accepted reconstructions.
            body=list(cs.disasm(rebuilt.read(address,32),address))
            ret=next(i for i in body if i.mnemonic=='ret')
            assert int(ret.op_str,0)==contract['cleanup'],'callback stub stack ABI differs'
        result[name]=dict(original_slot=slot,rebuilt_slot=rebuilt_slot,assigned_methods=contract['methods'],original_call_sequences=contract['calls'],stack_cleanup=contract['cleanup'])
    # The full encountered DOS payload is independently typed in source.
    assert rebuilt.read(publics['_default_dos_image'],128)==original.read(0x407058,128),'typed DOS output payload differs'
    return dict(callbacks=result,qualification='Three separate standalone globals only; no embedded664-byte context field, array stride, serialization or nonzero this adjustment is covered. These actual callback bodies remain named traps until reached.')
