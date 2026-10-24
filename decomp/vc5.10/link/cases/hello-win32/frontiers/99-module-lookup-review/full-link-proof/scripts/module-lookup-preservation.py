"""Finite next-module/canonical CRT EBX contracts for one cached lookup import."""
import hashlib,importlib,json,re
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
BINARY=Path(__file__).resolve().parents[1]
PACKET=BINARY/'cases/hello-win32/module-lookup-import-preservation.json'
OWNER='OutputLibrary::FindModule'
def sha(data):return hashlib.sha256(data).hexdigest()
def check(original,rebuilt,map_text,packet=None):
    packet=json.loads(PACKET.read_text())if packet is None else packet
    provider=importlib.import_module('options-preservation').check(original,packet,{'_mbsicmp'})
    assert packet['owner']==OWNER
    contract=packet['function_contract'];assert contract['name']=='_next_module@4'and contract['preserves']==['ebx']
    body=contract['original'];assert(body['begin'],body['end'])==(0x40fb90,0x40fbcc)
    raw=original.read(body['begin'],60);assert sha(raw)==body['sha256']
    cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
    instructions=list(cs.disasm(raw,body['begin']));assert sum(i.size for i in instructions)==60
    assert[i.address for i in instructions if {'ebx','bx','bl','bh'}&{i.reg_name(r)for r in i.regs_access()[1]}]==body['own_ebx_writes']==[]
    assert[i.address for i in instructions if i.mnemonic=='call']==body['calls']==[]
    assert[dict(site=i.address,bytes=i.bytes.hex())for i in instructions if i.mnemonic=='ret']==body['returns']==[dict(site=0x40fbb4,bytes='c20400'),dict(site=0x40fbc9,bytes='c20400')]
    publics={name:int(address,16)for name,address in re.findall(r'^\s+[0-9a-fA-F]+:[0-9a-fA-F]+\s+(\S+)\s+([0-9a-fA-F]{8})\b',map_text,re.M)}
    counterpart=contract['rebuilt'];assert counterpart['symbol']=='_next_module@4'
    code=bytes.fromhex(counterpart['bytes']);assert rebuilt.read(publics[counterpart['symbol']],len(code))==code
    emitted=list(cs.disasm(code,0));assert sum(i.size for i in emitted)==len(code)
    assert any(i.mnemonic=='int3'for i in emitted),'next_module reconstructed: refresh actual complete counterpart witness first'
    assert[i.address for i in emitted if {'ebx','bx','bl','bh'}&{i.reg_name(r)for r in i.regs_access()[1]}and not(i.mnemonic=='pop'and i.op_str=='ebx')]==counterpart['own_ebx_writes']==[]
    assert code.hex()=='558bec535657cc32c05f5e5b5dc20400'
    assert counterpart['ebx_save_restore']==dict(save_offset=3,restore_offset=11,save='53',restore='5b')
    # Exact complete straight-line body pushes EBX once, changes neither saved
    # stack slot nor EBX, then restores that same slot before sole RET4.
    assert[i.address for i in emitted if i.mnemonic=='call']==counterpart['calls']==[]
    assert[dict(offset=i.address,bytes=i.bytes.hex())for i in emitted if i.mnemonic=='ret']==counterpart['returns']
    assert emitted[-1].mnemonic=='ret'and emitted[-1].op_str=='4'
    assert packet['original_cached_sites']==[0x40f7ea]and packet['original_direct_sites']==[]
    return dict(provider=provider,finite_helper_bytes=60,helper_calls=0,preserved_registers=['ebx'])
def contracts():
    evidence=dict(path=str(PACKET.relative_to(BINARY)),sha256=sha(PACKET.read_bytes()))
    return[dict(target={'import':dict(dll='msvcrt.dll',name='_mbsicmp')}if name=='_mbsicmp'else dict(function=name),preserves=['ebx'],evidence=evidence)for name in['_mbsicmp','_next_module@4']]
def check_resolution(report,rebuilt):
    packet=json.loads(PACKET.read_text());bound=report['register_import_inputs']
    assert bound['original_sha256']==packet['original_sha256']and bound['rebuilt_sha256']==sha(rebuilt.data)
    assert bound['preservation_contracts']==contracts()
    left=report['functions'][OWNER]['register_imports']['original'];right=report['functions'][OWNER]['register_imports']['rebuilt']
    for side in[left,right]:assert side['unresolved_register_calls']==side['other_unresolved_calls']==[]
    assert left['resolved_register_calls']=={str(0x40f7ea):dict(dll='msvcrt.dll',name='_mbsicmp')}and not left['direct_import_calls']
    assert not right['resolved_register_calls']and len(right['direct_import_calls'])==1
    assert list(right['direct_import_calls'].values())==[dict(dll='msvcrt.dll',name='_mbsicmp')]
    return dict(original_cached=1,original_direct=0,rebuilt_direct=1,opaque=0)
