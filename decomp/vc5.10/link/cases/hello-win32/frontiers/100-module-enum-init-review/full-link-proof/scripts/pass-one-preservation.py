"""Two finite ESI ABI witnesses; the common Rust auditor discovers all calls."""
import hashlib
import importlib
import json
from pathlib import Path
import re
from capstone import Cs,CS_ARCH_X86,CS_MODE_32

BINARY=Path(__file__).resolve().parents[1]
PACKET=BINARY/'cases/hello-win32/pass-one-import-preservation.json'
OWNER='LinkContext::PassOne'


def sha(data):return hashlib.sha256(data).hexdigest()


def check(original,rebuilt,map_text,packet=None):
    packet=json.loads(PACKET.read_text())if packet is None else packet
    provider=importlib.import_module('options-preservation').check(original,packet,{'fputc'})
    assert packet['owner']==OWNER
    assert packet['original_register_sites']==[0x44bc8e,0x44bcab]
    assert packet['original_direct_sites']==[0x44b87d]
    contract=packet['function_contract']
    assert contract['name']=='_link_message'and contract['preserves']==['esi']
    cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
    expected=[('link_message',0x4302b0,0x4302c4,[],[0x4302c3]),
              ('emit_diagnostic',0x430020,0x4300f2,[0x43005e,0x4300ec],[0x4300ef]),
              ('message_is_suppressed',0x42ffa0,0x42ffea,[0x42ffaf,0x42ffc1,0x42ffc3,0x42ffda,0x42ffe0,0x42ffe6],[0x42ffb0,0x42ffdb,0x42ffe1,0x42ffe7])]
    assert len(contract['original_witnesses'])==len(expected)
    for witness,(name,begin,end,writes,returns)in zip(contract['original_witnesses'],expected):
        assert(witness['name'],witness['begin'],witness['end'])==(name,begin,end)
        data=original.read(begin,end-begin)
        assert sha(data)==witness['sha256']
        instructions=list(cs.disasm(data,begin));assert sum(i.size for i in instructions)==len(data)
        actual_writes=[i.address for i in instructions if 'esi'in {i.reg_name(r)for r in i.regs_access()[1]}]
        actual_returns=[i.address for i in instructions if i.mnemonic=='ret']
        assert actual_writes==[i['site']for i in witness['esi_writes']]==writes
        assert actual_returns==[i['site']for i in witness['normal_returns']]==returns
        for item in witness['anchors']+witness['esi_writes']+witness['normal_returns']:
            raw=bytes.fromhex(item['bytes'])
            assert begin<=item['site']<item['site']+len(raw)<=end
            assert original.read(item['site'],len(raw))==raw
    # The normal branch saves ESI before its sole write; the early branch first
    # calls the finite filter, whose four exits all restore its saved ESI.
    for address,raw in [(0x430039,'56'),(0x4300ec,'5e'),(0x43002d,'0f85bb000000'),
                        (0x42ffa4,'56'),(0x42ffaf,'5e'),(0x42ffda,'5e'),(0x42ffe0,'5e'),(0x42ffe6,'5e')]:
        assert original.read(address,len(bytes.fromhex(raw)))==bytes.fromhex(raw)
    assert original.read(0x4302be,5)==b'\xe8'+(0x430020-0x4302c3).to_bytes(4,'little',signed=True)
    assert original.read(0x430026,5)==b'\xe8'+(0x42ffa0-0x43002b).to_bytes(4,'little',signed=True)
    symbol=contract['rebuilt']['symbol'];assert symbol=='_link_message'
    addresses=[int(a,16)for name,a in re.findall(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',map_text,re.M)if name==symbol]
    assert len(addresses)==1
    stub=bytes.fromhex(contract['rebuilt']['bytes']);assert rebuilt.read(addresses[0],len(stub))==stub
    instructions=list(cs.disasm(stub,addresses[0]))
    assert sum(i.size for i in instructions)==len(stub)
    assert any(i.mnemonic=='int3'for i in instructions)and instructions[-1].mnemonic=='ret'and not instructions[-1].op_str
    assert not any('esi'in {i.reg_name(r)for r in i.regs_access()[1]}and not(i.mnemonic=='pop'and i.op_str=='esi')for i in instructions)
    assert sum(i.mnemonic=='push'and i.op_str=='esi'for i in instructions)==sum(i.mnemonic=='pop'and i.op_str=='esi'for i in instructions)
    return dict(provider=provider,ordinary_function_contracts=1,original_application_witness_bytes=304,
                qualification='Finite diagnostic wrapper/filter save/restore witness and public qualified CRT ABI only; no application acceptance or CRT source.')


def check_resolution(report):
    assert report['discovery']=='cfg'and set(report['functions'])=={OWNER}
    bound=report['register_import_inputs'];contracts=bound['preservation_contracts']
    assert len(contracts)==2
    targets=[]
    for item in contracts:
        assert item['preserves']==['esi']
        assert item['evidence']==dict(path='cases/hello-win32/pass-one-import-preservation.json',sha256=sha(PACKET.read_bytes()))
        targets.append(item['target'])
    assert{'function':'_link_message'}in targets and{'import':{'dll':'msvcrt.dll','name':'fputc'}}in targets
    sites=report['functions'][OWNER]['register_imports'];identity=dict(dll='msvcrt.dll',name='fputc')
    for side in ['original','rebuilt']:
        assert sites[side]['unresolved_register_calls']==[]and sites[side]['other_unresolved_calls']==[]
    assert sites['original']['resolved_register_calls']=={str(site):identity for site in [0x44bc8e,0x44bcab]}
    assert sites['original']['direct_import_calls']=={str(0x44b87d):identity}
    assert sites['rebuilt']['resolved_register_calls']=={}
    assert len(sites['rebuilt']['direct_import_calls'])==3
    assert all(i==identity for i in sites['rebuilt']['direct_import_calls'].values())
    return dict(original_register_calls=2,original_direct_import_calls=1,rebuilt_direct_import_calls=3,
                required_ordinary_preservation_contracts=2,unresolved=0)
