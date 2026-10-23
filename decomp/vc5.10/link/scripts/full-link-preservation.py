"""Bind finite FullLink callee ABI witnesses; Rust owns CFG and must-values."""
import hashlib
import importlib
import json
from pathlib import Path
import re
import struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from binary import PE,u32,u16

BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
PACKET=BINARY/'cases/hello-win32/full-link-import-preservation.json'
OWNER='?FullLink@LinkContext@@QAEHPAE@Z'
CALLBACK_TARGETS={'LinkContext::PeWriteHeaders','LinkContext::VxdWriteHeaders'}
REGISTER_SITES={0x443930:'msvcrt.dll!sprintf',0x4444ec:'msvcrt.dll!fprintf',
    0x444521:'msvcrt.dll!fprintf',0x444865:'kernel32.dll!GetTickCount',
    0x44488e:'msvcrt.dll!fprintf',0x4448c4:'msvcrt.dll!fprintf',
    0x4448d9:'kernel32.dll!GetTickCount',0x444915:'msvcrt.dll!fprintf'}
DIRECT_SITES={0x4430fe:'msvcrt.dll!_unlink',0x443142:'msvcrt.dll!fopen',
    0x4433f6:'msvcrt.dll!strncpy',0x4435cc:'msvcrt.dll!strchr',
    0x443a10:'msvcrt.dll!_tzset',0x443a2c:'msvcrt.dll!time',
    0x4444b8:'kernel32.dll!GetTickCount',0x44452d:'msvcrt.dll!fflush',
    0x4448d0:'msvcrt.dll!fflush',0x444921:'msvcrt.dll!fflush'}
CALLBACK_SITES={0x443a3c,0x444795}

def sha(data):return hashlib.sha256(data).hexdigest()

def publics(text):
    return {name:int(address,16)for name,address in re.findall(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',text,re.M)}

def check(original,rebuilt,map_text,configuration):
    packet=json.loads(PACKET.read_text())
    provider=importlib.import_module('options-preservation').check(original,packet,{'sprintf','fprintf','fflush'})
    addresses=publics(map_text)
    cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
    for function in packet['function_contracts']:
        instructions=[]
        for region in function['original_ranges']:
            data=original.read(region['begin'],region['end']-region['begin'])
            assert sha(data)==region['sha256'],'callee ABI body changed: '+function['name']
            decoded=list(cs.disasm(data,region['begin']))
            assert sum(i.size for i in decoded)==len(data)
            instructions+=decoded
        def encoded(i):return dict(address=i.address,bytes=i.bytes.hex(),instruction=i.mnemonic+' '+i.op_str)
        for register,witness in function['preservation'].items():
            aliases={'edi':{'edi','di','dil'},'esi':{'esi','si','sil'}}[register]
            own=[i for i in instructions if aliases & {i.reg_name(r)for r in i.regs_access()[1]}and not(i.mnemonic=='pop'and i.op_str==register)]
            saves=[i for i in instructions if i.mnemonic=='push'and i.op_str==register]
            restores=[i for i in instructions if i.mnemonic=='pop'and i.op_str==register]
            assert [encoded(i)for i in own]==witness['own_writes']
            assert [encoded(i)for i in saves]==witness['saves']
            assert [encoded(i)for i in restores]==witness['restores']
            assert [encoded(i)for i in instructions if i.mnemonic=='ret']==[item['return_instruction']for item in witness['returns']]
            assert all(any(s.address<w.address for s in saves)for w in own)
            if own:assert all(item['nearby_restores']for item in witness['returns'])
        compiled=function['rebuilt'];data=bytes.fromhex(compiled['bytes'])
        assert rebuilt.read(addresses[compiled['symbol']],len(data))==data,'callee compiled ABI changed; refresh review: '+function['name']
    tick=packet['get_tick_count_contract'];hfile=tick['hfile']
    provider_file=BINARY/hfile['path'];data=provider_file.read_bytes()
    assert len(data)==hfile['bytes']and sha(data)==hfile['sha256']
    receipt=(BINARY/hfile['receipt']).read_bytes();assert sha(receipt)==hfile['receipt_sha256']
    actual=json.loads(receipt);assert actual['files']['module-002.bin']==dict(bytes=len(data),sha256=sha(data))
    kernel=PE(provider_file);directory=kernel.base+u32(kernel.data,kernel.optional+96)
    assert kernel.string(kernel.base+u32(kernel.read(directory+12,4),0))==hfile['export_dll_name']
    base,_,count,functions,names,ordinals=struct.unpack_from('<6I',kernel.read(directory,40),16)
    found=[]
    for index in range(count):
        name=kernel.string(kernel.base+u32(kernel.read(kernel.base+names+4*index,4),0))
        if name=='GetTickCount':
            ordinal=u16(kernel.read(kernel.base+ordinals+2*index,2),0)
            found.append(dict(ordinal=base+ordinal,rva=u32(kernel.read(kernel.base+functions+4*ordinal,4),0)))
    assert found==[tick['provider_export']]
    code=bytes.fromhex(tick['entry_bytes'])
    assert kernel.read(kernel.base+found[0]['rva'],len(code))==code==bytes.fromhex('8bff558beca12003fe7f5dc3')
    declaration=tick['declaration'];header=(ROOT/declaration['path']).read_bytes()
    assert sha(header)==declaration['sha256']
    assert '\n'.join(header.decode('latin1').splitlines()[declaration['line_begin']-1:declaration['line_end']])==declaration['text']
    from member_pointers import check_static
    closed_slots=check_static(original,rebuilt,map_text,configuration)
    assert set(closed_slots['callbacks']['image_write_headers']['assigned_methods'].values())==CALLBACK_TARGETS
    function_names={f['name']for f in packet['function_contracts']}
    assert CALLBACK_TARGETS<=function_names and len(function_names)==30
    return dict(provider=provider,closed_member_slots=closed_slots,finite_function_witnesses=30,
        original_indirect_sites=20,qualification='Finite actual body/SDK ordinary ABI witnesses only. This checker performs no target CFG or register dataflow; exact per-site completeness is enforced on the common Rust report.')

def check_resolution(report,rebuilt):
    packet=json.loads(PACKET.read_text());function=report['functions'][OWNER]
    sites=function['register_imports']
    def identities(items):return {str(site):dict(zip(['dll','name'],identity.split('!',1)))for site,identity in items.items()}
    assert sites['original']['resolved_register_calls']==identities(REGISTER_SITES),'cached-import site identity lost or changed'
    assert sites['original']['direct_import_calls']==identities(DIRECT_SITES)
    assert sites['original']['unresolved_register_calls']==[]
    assert set(sites['original']['other_unresolved_calls'])==CALLBACK_SITES
    assert sites['rebuilt']['resolved_register_calls']=={}and sites['rebuilt']['unresolved_register_calls']==[]
    assert len(sites['rebuilt']['other_unresolved_calls'])==2
    assert len(sites['rebuilt']['direct_import_calls'])==18
    expected=set(REGISTER_SITES.values())|set(DIRECT_SITES.values())
    assert {i['dll']+'!'+i['name']for i in function['imports']}==expected
    assert {i['dll']+'!'+i['name']for i in sites['rebuilt']['direct_import_calls'].values()}==expected
    return dict(original_direct_sites=10,original_cached_sites=8,genuine_callbacks=2,
        rebuilt_direct_sites=18,qualified_imports=len(expected),unresolved_cached_sites=0,
        qualification='Both callback edges remain genuinely indirect; aggregate indirect equality alone is insufficient. Every known original cached site has its exact expected import identity.')
