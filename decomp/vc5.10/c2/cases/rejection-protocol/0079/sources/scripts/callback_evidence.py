"""Finite stores-only callback identities; no instruction/CFG discovery."""
import json
import re
import struct
from pathlib import Path
from state import BINARY, TARGET, sha
from data_evidence import reader


def model(config):
    if '_initialize_pass_functions' not in config['accepted']:
        return None
    path=BINARY/config['output_callback_boundary']
    content=path.read_bytes();assert sha(content)==config['output_callback_boundary_sha256']
    value=json.loads(content)
    assert value['target_sha256']==sha(TARGET.read_bytes())
    assert len(value['slots'])==20 and len(value['targets'])==23
    assert len({r['original']for r in value['slots']})==20
    assert len({r['native']for r in value['slots']})==20
    return value


def verify(config,graph):
    value=model(config)
    if value is None:return None
    assert {'function_references','whole_extent_boundaries','accepted_index_extents'} <= set(graph['checks'])
    definitions={r['name']:r for r in config['stubs']}
    for r in value['targets']:
        item=definitions[r['logical']]
        assert item['start']==r['original'] and item['end']==r['original']+1
        assert item['native']==r['native'] and item['kind']=='application'
        assert item['pending_extent'] and item['pending_signature']
        assert r['logical'] not in config['accepted']+config['trial']
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(BINARY/'build/c2.map').read_text(),re.M)}
    original,rebuilt=reader(TARGET),reader(BINARY/'build/c2.exe')
    assert len({maps['_'+r['native']]for r in value['slots']})==20
    assert len({maps['_'+r['native']]for r in value['targets']})==23
    for row in value['slots']:
        assert row['initial']==0 and original(row['original'],4)==bytes(4)
        assert rebuilt(maps['_'+row['native']],4)==bytes(4)
    assert original(0x49973c,4)==rebuilt(maps['_output_group_name'],4)==bytes(4)
    assert original(0x497c74,4)==rebuilt(maps['_output_enabled'],4)==struct.pack('<I',1)
    owned=graph['original_accepted_graphs']['_initialize_pass_functions']
    assert owned['references']==sorted([4,0x497fdc,0x4996c4,0x49d4ec]+[r['original']for r in value['targets']]),'whole original callback/literal reference inventory differs'
    assert owned['calls']==[0x453ab2,0x453ad4,0x453b0c,0x457e07]
    # This finite source guard protects the pending field namespace. It is
    # deliberately not a C parser, graph walker or substitute for Rust audits.
    source=(BINARY/'src/output-callbacks.c').read_text()
    stores=re.findall(r'\b(output_slot_[0-9a-f]+)\.target\s*=\s*(output_callback_[0-9a-f]+)\s*;',source)
    expected={(r['native'],target)for r in value['slots']for target in r['native_targets']}
    assert set(stores)==expected,'callback store alternatives missing or invented'
    names=[r['native']for r in value['slots']]
    targets=[r['native']for r in value['targets']]
    for path in (BINARY/'src').glob('*'):
        if path.suffix not in ('.c','.h'):continue
        for line in path.read_text().splitlines():
            slot=[name for name in names if re.search(r'\b'+name+r'\b',line)]
            if slot:
                assert len(slot)==1
                name=slot[0]
                assert (path.name=='output-callbacks.c' and (
                    re.fullmatch(r'C2PendingCallback '+name+r';',line.strip()) or
                    re.fullmatch(re.escape(name)+r'\.target = output_callback_[0-9a-f]+;',line.strip()))) or (
                    path.name=='output-callbacks.h' and line.strip()=='extern C2PendingCallback '+name+';'), 'pending callback use/escape requires reviewed actual callable ABI'
            target=[name for name in targets if re.search(r'\b'+name+r'\b',line)]
            if target:
                assert len(target)==1
                name=target[0]
                assert (path.name=='output-callbacks.c' and re.fullmatch(r'output_slot_[0-9a-f]+\.target = '+name+r';',line.strip())) or (
                    path.name=='output-callbacks.h' and line.strip()=='void '+name+'();') or (
                    path.name=='stubs.c' and line.strip()=='void '+name+'()'), 'pending target invocation/escape requires actual ABI review'
    return {'kind':'stores-only pending function-address identities; zero callback body/ABI credit',
            'boundary_sha256':config['output_callback_boundary_sha256'],
            'original_references':owned['references'],'original_calls':owned['calls'],
            'slots':[{'original':r['original'],'rebuilt':maps['_'+r['native']],'native':r['native'],'initial':0,'alternatives':r['alternatives']}for r in value['slots']],
            'targets':[{'logical':r['logical'],'original':r['original'],'rebuilt':maps['_'+r['native']],'native':r['native'],'signature':'UNRESOLVED; invocation forbidden'}for r in value['targets']]}
