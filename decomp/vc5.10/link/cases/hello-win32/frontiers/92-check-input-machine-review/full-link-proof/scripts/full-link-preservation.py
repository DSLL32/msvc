"""Bind finite FullLink callee ABI witnesses; Rust owns CFG and must-values."""
import hashlib
import importlib
import json
from pathlib import Path
import re
import struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from binary import PE,COFF,u32,u16

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
        if function['name']=='_seek_file@16'and compiled['classification']=='reconstructed application body':
            emitted=list(cs.disasm(data,0))
            assert len(data)==compiled['complete_bytes']==sum(i.size for i in emitted)==94,'rebuilt seek counterpart complete body'
            assert [i.address for i in emitted if {'edi','di','dil'}&{i.reg_name(r)for r in i.regs_access()[1]}]==compiled['own_writes']==[],'rebuilt seek counterpart writes EDI'
            assert [dict(offset=i.address,cleanup=int(i.op_str,0))for i in emitted if i.mnemonic=='ret']==compiled['returns']==[dict(offset=91,cleanup=16)],'rebuilt seek counterpart RET ABI'
            assert all(i.op_str.startswith('0x')and 0<=int(i.op_str,16)<len(data)for i in emitted if i.mnemonic.startswith('j')),'rebuilt seek counterpart local branch escapes'

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
    closed_guard=check_closed_slot(original,rebuilt,map_text)
    function_names={f['name']for f in packet['function_contracts']}
    assert CALLBACK_TARGETS<=function_names and len(function_names)==30
    return dict(provider=provider,closed_member_slots=closed_slots,closed_slot_guard=closed_guard,finite_function_witnesses=30,
        original_indirect_sites=20,qualification='Finite actual body/SDK ordinary ABI witnesses only. This checker performs no target CFG or register dataflow; exact per-site completeness is enforced on the common Rust report.')

def check_closed_slot(original,rebuilt,map_text,extra_objects=()):
    """Reject new slot aliases/writes before enabling finite call preservation."""
    packet_path=BINARY/'cases/hello-win32/full-link-closed-slot.json'
    packet=json.loads(packet_path.read_text());left=packet['original'];right=packet['rebuilt']
    assert sha(original.data)==left['target_sha256']
    assert sha(rebuilt.data)==right['pe_sha256']
    # Preserve MAP raw CRLF: Python text input normalizes newlines, so bind the
    # actual physical MAP separately from its parsed symbol view.
    assert sha((BINARY/'build/link.map').read_bytes())==right['map_sha256']
    slot=left['slot'];needle=struct.pack('<I',slot)
    offsets=[];position=0
    while True:
        position=original.data.find(needle,position)
        if position<0:break
        offsets.append(position);position+=1
    assert offsets==left['reference_file_offsets']and len(offsets)==5,'new original slot reference'
    zero_fill=left['initialization_basis']['section']
    assert zero_fill in original.sections
    assert original.base+zero_fill['rva']+zero_fill['size']<=slot
    assert slot+4<=original.base+zero_fill['rva']+zero_fill['virtual_size']
    for item in left['assignments']+left['calls']:
        data=bytes.fromhex(item['bytes']);site=item.get('site',item.get('sequence_site'))
        assert original.read(site,len(data))==data
    addresses=publics(map_text);assert addresses[right['symbol']]==right['slot']
    assert rebuilt.read(right['required_preservation_call'],6)==b'\xff\x15'+struct.pack('<I',right['slot'])
    references=[];debug_references=[]
    for path in sorted((BINARY/'build').glob('*.obj'))+list(extra_objects):
        try:obj=COFF(path.read_bytes())
        except (AssertionError,IndexError,ValueError,struct.error):continue
        for section in obj.sections:
            for offset,reference,kind in section['relocs']:
                if reference['name']!=right['symbol']:continue
                if not section['flags']&0x20:
                    assert section['name']=='.debug$S'and section['flags']==0x42100048 and kind in (10,11),'loaded-data/address-taking slot alias is outside closed contract'
                    debug_references.append(dict(object=str(path.relative_to(BINARY)),object_sha256=sha(path.read_bytes()),section=section['name'],section_flags=section['flags'],offset=offset,relocation_kind=kind))
                    continue
                prefix=section['data'][offset-2:offset]
                assert kind==6 and prefix in [b'\xff\x15',b'\xc7\x05']
                references.append(dict(object=str(path.relative_to(BINARY)),object_sha256=sha(path.read_bytes()),section=section['name'],offset=offset,relocation_kind=kind,form='call absolute memory'if prefix==b'\xff\x15'else 'store immediate function pointer',prefix=prefix.hex()))
    assert references==right['code_references'],'new/changed rebuilt slot reference or escaping address'
    assert debug_references==right['debug_metadata_references'],'new/changed debug slot relocation'
    assert not any(section['name']=='.debug$S'for section in rebuilt.sections),'discarded CodeView section unexpectedly loaded'
    for source in packet['sources']:
        assert sha((BINARY/source['path']).read_bytes())==source['sha256']
    assert packet['abi']['receiver']=='ECX'and packet['abi']['callee_cleanup']==4
    assert packet['abi']['native_member_bytes']==8 and packet['abi']['native_adjustment']==0
    assert {target['method']for target in left['allowed_targets']}==CALLBACK_TARGETS
    observation=packet['native_observation']
    assert sha((BINARY/observation['state']).read_bytes())==observation['state_sha256']
    assert sha((BINARY/observation['original_member']).read_bytes())==observation['original_member_sha256']
    state=json.loads((BINARY/observation['state']).read_text())
    assert state['member_pointers']['image_write_headers']==observation['native_member']
    assert observation['native_member']['bytes']==8
    assert bytes.fromhex(observation['native_member']['raw'])[4:]==bytes(4)
    return dict(packet_sha256=sha(packet_path.read_bytes()),original_slot_references=5,rebuilt_slot_references=3,
        discarded_debug_metadata_references=4,allowed_targets=sorted(CALLBACK_TARGETS),unknown_assignment_policy='reject before registering preservation')

def check_resolution(report,rebuilt):
    packet=json.loads(PACKET.read_text());function=report['functions'][OWNER]
    sites=function['register_imports']
    template=json.loads((BINARY/'cases/hello-win32/full-link-audit-trial/metadata.json').read_text())
    expected=template['register_imports'];bound=report['register_import_inputs']
    assert bound['original_sha256']==packet['original_sha256']
    assert bound['rebuilt_sha256']==sha(rebuilt.data)
    assert bound['preservation_contracts']==expected['preservation_contracts']
    assert bound['callback_preservation_contracts']==expected['callback_preservation_contracts']
    assert sites['require_all_register_calls_resolved']is True
    callback,=expected['callback_preservation_contracts']
    for side in ['original','rebuilt']:
        point=callback[side]
        assert sites[side]['callback_preservation']=={str(point['site']):dict(slot=point['slot'],preserves=['edi'],evidence=callback['evidence'])}

    def identities(items):return {str(site):dict(zip(['dll','name'],identity.split('!',1)))for site,identity in items.items()}
    assert sites['original']['resolved_register_calls']==identities(REGISTER_SITES),'cached-import site identity lost or changed'
    assert sites['original']['direct_import_calls']==identities(DIRECT_SITES)
    assert sites['original']['unresolved_register_calls']==[]
    assert set(sites['original']['other_unresolved_calls'])==CALLBACK_SITES
    assert sites['rebuilt']['resolved_register_calls']=={}and sites['rebuilt']['unresolved_register_calls']==[]
    assert len(sites['rebuilt']['other_unresolved_calls'])==2
    from member_pointers import CONTRACTS
    addresses=publics((BINARY/'build/link.map').read_text())
    known_slots={addresses[CONTRACTS[name]['symbol']]:name for name in ['image_header_size','image_write_headers']}
    observed_callbacks={}
    for site in sites['rebuilt']['other_unresolved_calls']:
        encoded=rebuilt.read(site,6)
        assert encoded[:2]==b'\xff\x15','unknown genuine callback call form'
        slot,=struct.unpack('<I',encoded[2:])
        assert slot in known_slots,'unreviewed opaque callback slot'
        role=known_slots[slot];assert role not in observed_callbacks
        observed_callbacks[role]=dict(site=site,slot=slot)
    assert set(observed_callbacks)=={'image_header_size','image_write_headers'}

    assert len(sites['rebuilt']['direct_import_calls'])==18
    expected=set(REGISTER_SITES.values())|set(DIRECT_SITES.values())
    assert {i['dll']+'!'+i['name']for i in function['imports']}==expected
    assert {i['dll']+'!'+i['name']for i in sites['rebuilt']['direct_import_calls'].values()}==expected
    return dict(original_direct_sites=10,original_cached_sites=8,genuine_callbacks=2,
        rebuilt_direct_sites=18,qualified_imports=len(expected),unresolved_cached_sites=0,
        rebuilt_member_callbacks=observed_callbacks,callback_preservation_contracts=1,ordinary_preservation_contracts=32,
        qualification='Both callback edges remain genuinely indirect; aggregate indirect equality alone is insufficient. Every known original cached site has its exact expected import identity.')
