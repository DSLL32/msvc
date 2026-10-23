"""Finite initialized writer storage and pending callback identities only."""
import json
import re
from state import BINARY,TARGET,sha
from data_evidence import reader


def model(config):
    if '_initialize_object_output'not in config['accepted']:return None
    raw=(BINARY/config['writer_callback_boundary']).read_bytes()
    assert sha(raw)==config['writer_callback_boundary_sha256']
    value=json.loads(raw);assert value['target_sha256']==sha(TARGET.read_bytes())
    assert value['record']['size']==32 and len(value['targets'])==3
    return value


def verify(config,graph):
    value=model(config)
    if value is None:return None
    definitions={r['name']:r for r in config['stubs']}
    for target in value['targets']:
        row=definitions[target['logical']]
        assert row['native']==target['native']and row['start']==target['original']and row['end']==row['start']+1
        assert row['pending_signature']and row['pending_extent']and row['kind']=='application'
        assert row['name']not in config['accepted']+config['trial']
    assert graph['original_accepted_graphs'][value['owner']]==value['original_graph']
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(BINARY/'build/c2.map').read_text(),re.M)}
    original,rebuilt=reader(TARGET),reader(BINARY/'build/c2.exe')
    assert original(0x48e020,96)==rebuilt(maps['_c2_pools'],96)==bytes(96)
    for key,address in [('object_output',0x497e08),('object_output_open',0x497e1c),('incremental_data',0x49e31c),('directive_section',0x4973fc),('fpo_section',0x497dc0)]:
        assert original(address,4)==rebuilt(maps['_'+key],4)==bytes(4)
    fields={r['name']:r for r in value['record']['fields']if r['name'].startswith('output_')}
    assert {r['offset']for r in fields.values()}=={16,20,24}
    target_names=[r['native']for r in value['targets']]
    for path in (BINARY/'src').glob('*'):
        if path.suffix not in ('.c','.h'):continue
        for line in path.read_text().splitlines():
            stripped=line.strip()
            for name,row in fields.items():
                if not re.search(r'\b'+name+r'\b',line):continue
                target='writer_callback_'+format(row['target'],'x')
                assert (path.name=='coff-output.c'and stripped=='writer->'+name+'.target = '+target+';')or(path.name=='coff-output.h'and(stripped=='C2PendingCallback '+name+';'or re.fullmatch(r'typedef char C2ObjectOutput\w+Check\[offsetof\(C2ObjectOutput, '+name+r'\) == '+str(row['offset'])+r' \? 1 : -1\];',stripped))),'pending writer callback invocation/escape requires recovered actual signature'
            for name in target_names:
                if not re.search(r'\b'+name+r'\b',line):continue
                assert (path.name=='coff-output.c'and re.fullmatch(r'writer->output_\w+\.target = '+name+r';',stripped))or(path.name=='coff-output.h'and stripped=='void '+name+'();')or(path.name=='stubs.c'and stripped=='void '+name+'() { __asm { int 3 } }'),'pending writer target may only be declared, stored, or trap'
    return {'scope':'stores-only initialized writer callback identities; no callback ABI/body credit','boundary_sha256':config['writer_callback_boundary_sha256'],'fields':list(fields.values()),'targets':[{'original':r['original'],'rebuilt':maps['_'+r['native']],'native':r['native']}for r in value['targets']],'original_graph':value['original_graph']}
