"""Finite CRT/diagnostic preservation binding; all CFG discovery stays common."""
import hashlib
import importlib
import json
from pathlib import Path
import re
from capstone import Cs,CS_ARCH_X86,CS_MODE_32

BINARY=Path(__file__).resolve().parents[1]
PACKET=BINARY/'cases/hello-win32/collision-import-preservation.json'
OWNER='_check_output_input_collision@8'
MASKS=[('_link_fatal',['edi','ebp']),('_fullpath',['edi','ebp']),('_mbsicmp',['edi','ebp'])]


def sha(data):return hashlib.sha256(data).hexdigest()


def contracts():
    evidence=dict(path=str(PACKET.relative_to(BINARY)),sha256=sha(PACKET.read_bytes()))
    return[dict(target=dict(function=name)if name=='_link_fatal'else{'import':dict(dll='msvcrt.dll',name=name)},preserves=mask,evidence=evidence)for name,mask in MASKS]


def check(original,rebuilt,map_text,packet=None):
    packet=json.loads(PACKET.read_text())if packet is None else packet
    provider=importlib.import_module('options-preservation').check(original,packet,{'_fullpath','_mbsicmp'})
    assert packet['owner']==OWNER
    assert packet['original_register_sites']=={'0x461862':'_fullpath','0x46189d':'_fullpath','0x4618c4':'_mbsicmp'}
    contract=packet['function_contract'];assert contract['name']=='_link_fatal'and contract['preserves']==['edi','ebp']
    cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
    expected=[('link_fatal',0x430230,0x430249,{'edi':[],'ebp':[]},[0x430248]),('emit_fatal_diagnostic',0x430160,0x43022c,{'edi':[0x43016d,0x430225],'ebp':[0x430167,0x430227]},[0x430229])]
    assert len(contract['original_witnesses'])==len(expected)
    for witness,(name,begin,end,writes,returns)in zip(contract['original_witnesses'],expected):
        assert(witness['name'],witness['begin'],witness['end'])==(name,begin,end)
        data=original.read(begin,end-begin);assert sha(data)==witness['sha256']
        ins=list(cs.disasm(data,begin));assert sum(i.size for i in ins)==len(data)
        assert[i.address for i in ins if i.mnemonic=='ret']==[i['site']for i in witness['normal_returns']]==returns
        for reg in ('edi','ebp'):
            assert[i.address for i in ins if reg in{i.reg_name(r)for r in i.regs_access()[1]}]==[i['site']for i in witness['writes'][reg]]==writes[reg]
        anchors=witness['saves']+witness['normal_returns']+witness['anchors']+sum(witness['writes'].values(),[])
        for anchor in anchors:
            raw=bytes.fromhex(anchor['bytes']);assert begin<=anchor['site']<anchor['site']+len(raw)<=end
            assert original.read(anchor['site'],len(raw))==raw
    # All actual own writes are between these real saves/restores; normal exit
    # retains matching stack depth and RET20. Wrapper calls this helper thenRET0.
    for site,raw in [(0x430166,'55'),(0x43016c,'57'),(0x430225,'5f5e5d5bc21400'),(0x430243,'e818ffffff')]:
        assert original.read(site,len(bytes.fromhex(raw))).hex()==raw
    actual=dict((symbol,int(va,16))for symbol,va in re.findall(r'^\s+[0-9A-Fa-f]+:[0-9A-Fa-f]+\s+(\S+)\s+([0-9A-Fa-f]{8})\b',map_text,re.M))
    stub=bytes.fromhex(contract['rebuilt']['bytes']);assert contract['rebuilt']['symbol']=='_link_fatal'
    assert stub==bytes.fromhex('558bec535657cc5f5e5b5dc3')
    assert rebuilt.read(actual['_link_fatal'],len(stub))==stub
    return dict(provider=provider,ordinary_function_contracts=1,finite_application_witness_bytes=229,normal_return_only=True)


def check_resolution(report):
    assert report['discovery']=='cfg'and set(report['functions'])=={OWNER}
    assert report['register_import_inputs']['preservation_contracts']==contracts()
    detail=report['functions'][OWNER]['register_imports'];inventory={}
    for side in ('original','rebuilt'):
        item=detail[side];assert item['unresolved_register_calls']==item['other_unresolved_calls']==[]
        resolved=item['resolved_register_calls'];direct=item['direct_import_calls']
        if side=='original':
            assert{int(k):v['name']for k,v in resolved.items()}=={0x461862:'_fullpath',0x46189d:'_fullpath',0x4618c4:'_mbsicmp'}
            assert not direct
        else:
            assert not resolved and len(direct)==3
            assert sorted(v['name']for v in direct.values())==['_fullpath','_fullpath','_mbsicmp']
        for target in list(resolved.values())+list(direct.values()):assert target['dll']=='msvcrt.dll'
        inventory[side]=dict(cached=len(resolved),direct=len(direct),unresolved=0)
    return inventory
