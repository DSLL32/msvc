"""One finite call-free helper and two qualified CRT EBX preservation contracts."""
import hashlib
import importlib
import json
import re
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
BINARY=Path(__file__).resolve().parents[1]
PACKET=BINARY/'cases/hello-win32/library-finder-import-preservation.json'
OWNER='_find_library@12'


def sha(data):return hashlib.sha256(data).hexdigest()


def check(original,rebuilt,map_text,packet=None):
    packet=json.loads(PACKET.read_text())if packet is None else packet
    provider=importlib.import_module('options-preservation').check(original,packet,{'_splitpath','_mbsicmp'})
    assert packet['owner']==OWNER
    contract=packet['function_contract'];assert contract['name']=='_next_library@4'and contract['preserves']==['ebx']
    body=contract['original'];assert(body['begin'],body['end'])==(0x40fb20,0x40fb52)
    data=original.read(body['begin'],50);assert sha(data)==body['sha256']
    decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    instructions=list(decoder.disasm(data,body['begin']));assert sum(i.size for i in instructions)==50
    assert[i.address for i in instructions if {'ebx','bx','bl','bh'}&{i.reg_name(r)for r in i.regs_access()[1]}]==body['own_ebx_writes']==[]
    assert[i.address for i in instructions if i.mnemonic=='call']==body['calls']==[]
    assert[dict(site=i.address,bytes=i.bytes.hex())for i in instructions if i.mnemonic=='ret']==body['returns']==[dict(site=0x40fb3d,bytes='c20400'),dict(site=0x40fb4f,bytes='c20400')]
    addresses={name:int(address,16)for name,address in re.findall(r'^\s+[0-9a-fA-F]+:[0-9a-fA-F]+\s+(\S+)\s+([0-9a-fA-F]{8})\b',map_text,re.M)}
    compiled=contract['rebuilt'];assert compiled['symbol']=='_next_library@4'
    code=bytes.fromhex(compiled['bytes']);assert code==bytes.fromhex('558bec535657cc32c05f5e5b5dc20400')
    assert rebuilt.read(addresses[compiled['symbol']],len(code))==code
    # Fully linear current stub: saved EBX at+3, restored at+11, RET4 at+13.
    assert code[3]==0x53 and code[11]==0x5b and code[13:]==bytes.fromhex('c20400')
    assert packet['original_cached_sites']==[0x40f6c2]and packet['original_direct_sites']==[0x40f73f]
    return dict(provider=provider,finite_helper_bytes=50,helper_calls=0,preserved_registers=['ebx'])


def contracts():
    evidence=dict(path=str(PACKET.relative_to(BINARY)),sha256=sha(PACKET.read_bytes()))
    return [dict(target={'import':dict(dll='msvcrt.dll',name=name)}if name!='_next_library@4'else dict(function=name),preserves=['ebx'],evidence=evidence)for name in ('_splitpath','_mbsicmp','_next_library@4')]


def check_resolution(report,rebuilt):
    packet=json.loads(PACKET.read_text());bound=report['register_import_inputs']
    assert bound['original_sha256']==packet['original_sha256']and bound['rebuilt_sha256']==sha(rebuilt.data)
    assert bound['preservation_contracts']==contracts()
    sides=report['functions'][OWNER]['register_imports'];left=sides['original'];right=sides['rebuilt']
    for side in (left,right):assert side['unresolved_register_calls']==side['other_unresolved_calls']==[]
    assert left['resolved_register_calls']=={str(0x40f6c2):dict(dll='msvcrt.dll',name='_splitpath')}
    assert left['direct_import_calls']=={str(0x40f73f):dict(dll='msvcrt.dll',name='_mbsicmp')}
    assert not right['resolved_register_calls']and len(right['direct_import_calls'])==2
    assert sorted(right['direct_import_calls'].values(),key=lambda x:x['name'])==[dict(dll='msvcrt.dll',name='_mbsicmp'),dict(dll='msvcrt.dll',name='_splitpath')]
    return dict(original_cached=1,original_direct=1,rebuilt_direct=2,opaque=0)
