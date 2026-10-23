"""Bind six reviewed CRT ABI contracts; the common Rust tool owns all CFG work."""
import hashlib
import json
from pathlib import Path
import struct
from binary import PE, u16, u32

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]
PACKET = BINARY / 'cases/help/options-import-preservation.json'
REQUIRED = {'__p__pgmptr','_makepath','_splitpath','_stricmp','atoi','fopen'}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def check(original, packet=None):
    """Bind the provider/SDK review; no unrelated helper snapshots are consumed."""
    packet = json.loads(PACKET.read_text()) if packet is None else packet
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
    assert {i['identity'] for i in packet['imports']} == {'msvcrt.dll!'+name for name in REQUIRED}
    assert len(packet['imports']) == 6
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
    return dict(packet_sha256=sha(PACKET.read_bytes()),qualified_import_contracts=6,application_helper_contracts=0,
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
    assert len(contracts) == 6
    assert {c['target']['import']['dll']+'!'+c['target']['import']['name'] for c in contracts} == expected
    evidence = dict(path='cases/help/options-import-preservation.json',sha256=sha(PACKET.read_bytes()))
    assert all(c['evidence']==evidence and c['preserves']==['ebx','esi','edi'] for c in contracts)
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
    return dict(original_register_calls=147,original_direct_calls=50,rebuilt_direct_calls=197,qualified_imports=19,required_preservation_contracts=6,application_helper_contracts=0,unresolved=0)
