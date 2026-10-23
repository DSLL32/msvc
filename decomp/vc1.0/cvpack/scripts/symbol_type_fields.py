"""Compile-time read/convert/write identity where primitive outputs are unchanged."""
import hashlib
from capstone import Cs,CS_ARCH_X86,CS_MODE_32

def validate(config,original,rebuilt,linked):
    if not any(r['name']=='convert_bp_relative32'and r['status']!='stub'for r in config['functions']):return
    assert original.read(0x2550e,0x17)==bytes.fromhex('8b35e4350300668b4e0883c608e8c00f00006689065ec3'),'original BP-relative source field/CX/store differs'
    start=linked['_convert_bp_relative32'];end=min(v for v in linked.values()if v>start);raw=rebuilt.read(start,end-start);code=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(raw,start));calls=[r for r in code if r.mnemonic=='call']
    assert len(calls)==1 and calls[0].op_str=='0x%x'%linked['@convert_new_type@4'],'BP-relative conversion callee differs'
    call=calls[0];before=rebuilt.read(call.address-19,19);source=linked['_symbol_input'].to_bytes(4,'little');assert before[:9]==b'\xa1'+source+b'\x83\xc0\x08\x89','BP-relative field is not original symbol+8'
    slot=before[10:11]
    assert before==b'\xa1'+source+b'\x83\xc0\x08\x89\x45'+slot+b'\x8b\x45'+slot+b'\x66\x8b\x00\x8b\xc8','BP-relative field->same local->CX differs'
    assert rebuilt.read(call.address+5,6)==b'\x8b\x4d'+slot+b'\x66\x89\x01','BP-relative return AX not stored through same field'
    depth=linked['_suppressed_symbol_depth'].to_bytes(4,'little');assert raw.count(b'\x83\x3d'+depth+b'\x00')==2 and b'\xa1'+source+b'\x66\xc7\x40\x02\x08\x00'in raw,'BP-relative suppressed marker/reload differs'
    return dict(function='convert_bp_relative32',field=8,bits=16,source_owner='symbol_input',original=[0x2550e,0x25525],rebuilt_call=call.address,field_creation=call.address-19,frame_slot=int.from_bytes(slot,'little',signed=True),rebuilt_sha256=hashlib.sha256(rebuilt.data).hexdigest())
