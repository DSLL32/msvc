"""Bind six reviewed CRT ABI contracts; the common Rust tool owns all CFG work."""
import hashlib
import json
from pathlib import Path
import struct
import re
from binary import PE, u16, u32

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]
PACKET = BINARY / 'cases/help/options-import-preservation.json'
REQUIRED = {'__p__pgmptr','_makepath','_splitpath','_stricmp','atoi','fopen'}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def check(original, packet=None, required=None):
    """Bind the provider/SDK review; no unrelated helper snapshots are consumed."""
    packet = json.loads(PACKET.read_text()) if packet is None else packet
    required=REQUIRED if required is None else required
    assert packet['schema'] == 1
    assert packet['original_sha256'] == sha(original.data), 'stale ABI witness original'
    provider = packet['canonical_provider']
    provider_bytes = (ROOT/provider['path']).read_bytes()
    assert len(provider_bytes) == provider['bytes'] and sha(provider_bytes) == provider['sha256'], 'ABI provider differs'
    pe = PE(ROOT/provider['path'])
    receipt_path = BINARY/provider['owned_receipt']
    receipt_bytes = receipt_path.read_bytes()
    assert sha(receipt_bytes) == provider['actual_loaded_receipt_sha256'], 'ABI module receipt changed'
    receipt = json.loads(receipt_bytes)
    assert receipt['actual_loaded_crt_verified']
    assert {Path(i['path']).name for i in provider['owned_hfile_copies']} == set(receipt['actual_loaded_crt_hfile_copies'])
    assert provider['owned_hfile_copies']
    for item in provider['owned_hfile_copies']:
        data = (BINARY/item['path']).read_bytes()
        assert len(data) == item['bytes'] and sha(data) == item['sha256'] == provider['sha256'], 'actual ABI provider differs'
        assert receipt['files'][Path(item['path']).name] == dict(bytes=len(data),sha256=sha(data))
    directory = pe.base+u32(pe.data,pe.optional+96)
    base,_,count,functions,names,ordinals = struct.unpack_from('<6I',pe.read(directory,40),16)
    exports = {}
    for index in range(count):
        name = pe.string(pe.base+u32(pe.read(pe.base+names+4*index,4),0))
        ordinal = u16(pe.read(pe.base+ordinals+2*index,2),0)
        exports[name] = dict(ordinal=base+ordinal,rva=u32(pe.read(pe.base+functions+4*ordinal,4),0))
    assert {i['identity'] for i in packet['imports']} == {'msvcrt.dll!'+name for name in required}
    assert len(packet['imports']) == len(required)
    for imported in packet['imports']:
        name = imported['identity'].split('!',1)[1]
        assert imported['provider_export'] == exports[name], 'ABI import identity changed'
        assert pe.read(pe.base+exports[name]['rva'],16).hex() == imported['entry_bytes']
        assert imported['declarations']
        for declaration in imported['declarations']:
            data = (ROOT/declaration['path']).read_bytes()
            assert sha(data) == declaration['sha256'], 'ABI SDK header changed'
            line = data.decode('latin1').splitlines()[declaration['line']-1].strip()
            assert line == declaration['declaration'] and '__cdecl' in line
    return dict(qualified_import_contracts=len(required),application_helper_contracts=0,
        qualification='Only required canonical-provider/SDK ABI witnesses. Unknown application calls remain barriers; no helper source/object snapshots or target CFG/dataflow walk.')


def check_resolution(report, rebuilt):
    """Compare independent common-tool sites with the reviewed original inventory."""
    packet = json.loads(PACKET.read_text())
    assert report['discovery'] == 'cfg'
    bound = report['register_import_inputs']
    assert bound['original_sha256'] == packet['original_sha256']
    assert bound['rebuilt_sha256'] == sha(rebuilt.data)
    contracts = bound['preservation_contracts']
    expected = {i['identity'] for i in packet['imports']}
    parser_checked='_parse_switch@8' in report['functions']
    if parser_checked:expected.add('msvcrt.dll!strchr')
    assert len(contracts) == len(expected)+(1 if parser_checked else 0)
    assert {c['target']['function']for c in contracts if 'function'in c['target']}==({'_link_fatal'}if parser_checked else set())
    assert {c['target']['import']['dll']+'!'+c['target']['import']['name'] for c in contracts if 'import' in c['target']} == expected
    evidence = dict(path='cases/help/options-import-preservation.json',sha256=sha(PACKET.read_bytes()))
    parser_path=BINARY/'cases/help/parser-import-preservation.json'
    parser_evidence=dict(path='cases/help/parser-import-preservation.json',sha256=sha(parser_path.read_bytes())) if parser_checked else None
    for contract in contracts:
        required_evidence=parser_evidence if 'function' in contract['target'] or contract['target']['import']['name']=='strchr' else evidence
        assert contract['evidence']==required_evidence and contract['preserves']==['ebx','esi','edi']
    function = report['functions']['_configure_link_options@12']
    sites = function['register_imports']
    for side in ['original','rebuilt']:
        assert sites[side]['unresolved_register_calls'] == []
        assert sites[side]['other_unresolved_calls'] == []
    def identities(items):
        return {str(i['site']):dict(zip(['dll','name'],i['identity'].split('!',1))) for i in items}
    inventory = packet['original_inventory']
    assert sites['original']['resolved_register_calls'] == identities(inventory['register_calls']), 'cached import site inventory differs'
    assert sites['original']['direct_import_calls'] == identities(inventory['direct_import_calls']), 'direct import site inventory differs'
    assert len(sites['original']['resolved_register_calls']) == 147 and len(sites['original']['direct_import_calls']) == 50
    assert not sites['rebuilt']['resolved_register_calls']
    assert len(sites['rebuilt']['direct_import_calls']) == 197
    expected = set(packet['declared_imports'])
    assert {i['dll']+'!'+i['name'] for i in function['imports']} == expected
    assert {i['dll']+'!'+i['name'] for i in sites['rebuilt']['direct_import_calls'].values()} == expected
    parser_inventory=None
    if parser_checked:
        parser_packet=json.loads(parser_path.read_text())
        parser_function=report['functions']['_parse_switch@8']
        parser_sites=parser_function['register_imports']
        assert parser_sites['original']['resolved_register_calls']=={str(site):dict(dll='msvcrt.dll',name='strchr')for site in parser_packet['register_sites']}
        assert not parser_sites['original']['direct_import_calls']
        for side in ['original','rebuilt']:
            assert parser_sites[side]['unresolved_register_calls']==[] and parser_sites[side]['other_unresolved_calls']==[]
        assert not parser_sites['rebuilt']['resolved_register_calls']
        assert len(parser_sites['rebuilt']['direct_import_calls'])==5
        assert all(i==dict(dll='msvcrt.dll',name='strchr')for i in parser_sites['rebuilt']['direct_import_calls'].values())
        parser_inventory=dict(original_register_calls=5,rebuilt_direct_calls=5,qualified_imports=1)
    return dict(parser=parser_inventory,original_register_calls=147,original_direct_calls=50,rebuilt_direct_calls=197,qualified_imports=19,required_preservation_contracts=7 if parser_checked else 6,application_helper_contracts=1 if parser_checked else 0,unresolved=0)


def check_parser_helper(original,rebuilt,map_text):
    packet=json.loads((BINARY/'cases/help/parser-import-preservation.json').read_text())
    function=packet['function_contract']
    assert function['name']=='_link_fatal'
    assert sha(original.read(function['original_begin'],function['original_end']-function['original_begin']))==function['original_body_sha256']
    assert all(not function['preservation'][register]['writes']for register in ['ebx','esi','edi'])
    assert function['standard_abi']=='cdecl, ret '
    symbol=function['rebuilt_named_stub']['symbol']
    addresses=[int(address,16)for name,address in re.findall(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',map_text,re.M)if name==symbol]
    assert len(addresses)==1
    data=bytes.fromhex(function['rebuilt_named_stub']['bytes'])
    assert rebuilt.read(addresses[0],len(data))==data,'named stub changed; refresh actual callee ABI review'
    return dict(function='_link_fatal',classification='needed parser-only ordinary callee preservation; no body acceptance')
