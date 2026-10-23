"""Shared metadata/masking rules. Contains no original execution transport."""
import hashlib
import importlib.util
import json
import struct
from pathlib import Path

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]
TARGET = ROOT / 'toolchains/5.10/bin/C2.EXE'
CANONICAL_CRT = 'e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'

def sha(data):
    return hashlib.sha256(data).hexdigest()

def load():
    config = json.loads((BINARY/'functions.json').read_text())
    original = TARGET.read_bytes()
    assert sha(original) == config['target']['sha256'], 'target identity changed'
    spec = importlib.util.spec_from_file_location('c2_pe_inventory', BINARY/'scripts/inspect.py')
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    return config, original, module.inventory()

def offset(inventory, address, size=1):
    rva = address-int(inventory['image_base'],16)
    for section in inventory['sections']:
        delta = rva-section['rva']
        if 0 <= delta and delta+size <= section['raw_size']:
            return section['raw_offset']+delta
    raise AssertionError('unbacked range %x+%x' % (address,size))

def ranges(record):
    return ([[record['start'],record['end']]]+record.get('code_chunks',[]))if 'end'in record else []

def validate(config, original, inventory):
    assert sha(original) == config['target']['sha256']
    assert config['case']['arguments'] == ['/?']
    assert config['case']['environment'] == {'MSC_CMD_FLAGS':None}
    seen=[]
    for record in config['driver']+config['support']+config['text_data']:
        if record.get('kind')=='import':
            assert record['end']-record['start']==6
            p=offset(inventory,record['start'],6)
            assert original[p:p+6]==b'\xff\x25'+struct.pack('<I',record['iat']),'import support is not the declared exact IAT jump'
            assert record.get('abi')and record.get('dll')and record.get('symbol')
        assert record.get('evidence'), 'range lacks explicit evidence: '+record['name']
        assert ranges(record), 'empty range ownership'
        for start,end in ranges(record):
            assert start < end
            offset(inventory,start,end-start)
            assert any(int(s['characteristics'],16)&0x20 and
                       int(inventory['image_base'],16)+s['rva'] <= start and
                       end <= int(inventory['image_base'],16)+s['rva']+s['virtual_size']
                       for s in inventory['sections']), 'range outside target code'
            assert all(end <= a or b <= start for a,b,_ in seen), 'overlapping ownership'
            seen.append((start,end,record['name']))
    names=[r['name'] for r in config['driver']+config['support']+config['stubs']]
    assert len(names)==len(set(names)), 'duplicate logical function'
    assert len(config['trial'])==len(set(config['trial']))
    assert set(config['trial']) <= {r['name']for r in config['driver']+config['support']}
    assert len(config['accepted'])==len(set(config['accepted']))
    assert set(config['accepted']) <= {r['name'] for r in config['driver']+config['support']}
    return seen

def masked(config, original, inventory, candidates=()):
    validate(config,original,inventory)
    allowed=set(config['accepted'])|set(candidates)
    definitions={r['name']:r for r in config['driver']+config['support']}
    assert allowed <= definitions.keys(), 'unknown candidate'
    image=bytearray(original)
    for section in inventory['sections']:
        if int(section['characteristics'],16)&0x20:
            start=section['raw_offset'];size=section['raw_size']
            image[start:start+size]=b'\xcc'*size
    for record in config['text_data']+[definitions[n] for n in sorted(allowed)]:
        for start,end in ranges(record):
            p=offset(inventory,start,end-start)
            image[p:p+end-start]=original[p:p+end-start]
    return bytes(image)

def source_hashes():
    paths=list((BINARY/"src").glob("*"))+[BINARY/"scripts/native-host.c"]
    return {str(p.relative_to(BINARY)):sha(p.read_bytes())for p in sorted(paths)if p.is_file()}

def trial_configuration(base_bytes):
    base=json.loads(base_bytes)
    trial=json.loads(base_bytes)
    assert len(base['trial'])==len(set(base['trial']))
    known={r['name']for r in base['driver']+base['support']}
    assert set(base['trial'])<=known,'trial includes unknown definition'
    trial['accepted']=base['accepted']+[n for n in base['trial']if n not in base['accepted']]
    trial['trial_provenance']={'base_sha256':sha(base_bytes),'base_accepted':base['accepted'],'candidates':base['trial']}
    return trial,(json.dumps(trial,indent=2)+'\n').encode()

def audit_input_hashes():
    names=['runtime.json','build/runtime.json','build/c2.map','build/graph-metadata.json']
    out={name:sha((BINARY/name).read_bytes())for name in names}
    out['graph-audit']=sha((ROOT/'tooling/graph-audit/target/release/graph-audit').read_bytes())
    return out

def receipt_bound(receipt, snapshot, native=False):
    base_bytes=(BINARY/'functions.json').read_bytes()
    trial,trial_bytes=trial_configuration(base_bytes)
    assert snapshot==trial_bytes,'stale/different immutable trial configuration'
    assert receipt['base_configuration_sha256']==sha(base_bytes)
    assert receipt['configuration_sha256']==sha(trial_bytes)
    assert receipt['candidates']==trial['trial']==trial['trial_provenance']['candidates']
    assert receipt['target_sha256']==sha(TARGET.read_bytes())
    assert receipt['environment']=={'MSC_CMD_FLAGS':None}
    if native:
        assert receipt['source_hashes']==source_hashes(),'reconstructed source changed since native capture'
        assert receipt['native_sha256']==sha((BINARY/'build/native/c2').read_bytes())
    else:
        assert receipt['provider_sha256']==CANONICAL_CRT
        assert receipt['debugger_sha256']==sha((BINARY/'build/debug-run.exe').read_bytes()),'original debugger changed'
