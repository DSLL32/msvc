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
    contract=None
    if config.get('directive_callback_contract'):
        raw=(BINARY/config['directive_callback_contract']).read_bytes()
        assert sha(raw)==config['directive_callback_contract_sha256']
        contract=json.loads(raw)
        assert contract['slot']['original']==0x49971c
        assert contract['target_sha256']==sha(TARGET.read_bytes())
        assert contract['historical_identity_boundary_sha256']==config['output_callback_boundary_sha256']
        assert contract['slot']==next(r for r in value['slots']if r['original']==0x49971c)
    storage=None
    if config.get('section_callback_storage_contract'):
        content=(BINARY/config['section_callback_storage_contract']).read_bytes()
        assert sha(content)==config['section_callback_storage_contract_sha256']
        storage=json.loads(content)
        assert storage['entry']==0x41ec20 and storage['slot']==0x497ce4 and storage['logical']=='_output_callback_41ec20'
        assert storage['target_sha256']==sha(TARGET.read_bytes())
    definitions={r['name']:r for r in config['driver']+config['stubs']}

    typed_body=False
    for r in value['targets']:
        item=definitions[r['logical']]
        assert item['start']==r['original'] and item['kind']=='application'
        if contract and r['logical']=='_output_callback_453e02':
            assert not item['pending_signature']
            assert item['symbol']==contract['actual_source']['target_symbol']
            assert item['native']==contract['actual_source']['target_native']
            assert 'C2OutputDirective*directiveECX' in item['abi']
            typed_body=r['logical']in config['accepted']+config['trial']
            if typed_body:
                assert not item['pending_extent'] and r['logical']in graph['functions'],'typed callback body requires its ordinary wholefunction audit'
            else:assert item['end']==r['original']+1 and item['pending_extent']
        elif storage and r['logical']=='_output_callback_41ec20':
            assert item['native']==storage['target_native']and item['symbol']==storage['target_symbol']and not item['pending_signature']
            assert item['end']==r['original']+1 and item['pending_extent']
            assert r['logical']not in config['accepted']+config['trial'],'direct target body still pending'
        else:
            assert item['native']==r['native'] and item['end']==r['original']+1
            assert item['pending_extent'] and item['pending_signature']
            assert r['logical']not in config['accepted']+config['trial']
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(BINARY/'build/c2.map').read_text(),re.M)}
    original,rebuilt=reader(TARGET),reader(BINARY/'build/c2.exe')
    assert len({maps['_'+r['native']]for r in value['slots']})==20
    assert len({maps[definitions[r['logical']].get('symbol',r['logical'])]for r in value['targets']})==23
    for row in value['slots']:
        assert row['initial']==0 and original(row['original'],4)==bytes(4)
        assert rebuilt(maps['_'+row['native']],4)==bytes(4)
    if contract:
        assert original(0x41eb89,1)==b'\xc3','shared noop is not exact RET0'
        assert original(0x453df4,6)==b'\xff\x15'+struct.pack('<I',0x49971c)
        owner=maps['_write_output_directives'];slot=maps['_output_slot_49971c']
        assert rebuilt(owner+28,9)==b'\x8b\x4d\xfc\xff\x15'+struct.pack('<I',slot),'canonical typed callback must load ECX and call exact qualified slot'
        header=(BINARY/'src/output-callbacks.h').read_text()
        assert 'typedef void (__fastcall *C2OutputDirectiveWriter)(C2OutputDirective *directive);'in header
        assert 'typedef struct C2DirectiveCallback { C2OutputDirectiveWriter target; } C2DirectiveCallback;'in header
    assert original(0x49973c,4)==rebuilt(maps['_output_group_name'],4)==bytes(4)
    assert original(0x497c74,4)==rebuilt(maps['_output_enabled'],4)==struct.pack('<I',1)
    owned=graph['original_accepted_graphs']['_initialize_pass_functions']
    assert owned['references']==sorted([4,0x497fdc,0x4996c4,0x49d4ec]+[r['original']for r in value['targets']]),'whole original callback/literal reference inventory differs'
    assert owned['calls']==[0x453ab2,0x453ad4,0x453b0c,0x457e07]
    # This finite source guard protects the pending field namespace. It is
    # deliberately not a C parser, graph walker or substitute for Rust audits.
    source=(BINARY/'src/output-callbacks.c').read_text()
    store_source=source
    if storage:
        exact='output_slot_497ce4.target = (void (__cdecl *)()) select_output_section;'
        assert source.count(exact)==1
        store_source=source.replace('(void (__cdecl *)()) select_output_section','select_output_section')
    stores=re.findall(r'\b(output_slot_[0-9a-f]+)\.target\s*=\s*((?:\(C2OutputDirectiveWriter\) )?[A-Za-z_][A-Za-z_0-9]*)\s*;',store_source)
    if contract:
        assert ('output_slot_49971c','(C2OutputDirectiveWriter) output_callback_41eb89')in stores
        stores=[(slot,target.replace('(C2OutputDirectiveWriter) ',''))for slot,target in stores]
        exact='''void write_output_directives(void)
{
    C2OutputDirective *directive;
    for (directive = output_directives; directive; directive = directive->next)
        output_slot_49971c.target(directive);
}'''
        assert source.count(exact)==1 and source.count('output_slot_49971c.target(directive);')==1,'typed consumer ownership/body changed'
        stub=(BINARY/'src/stubs.c').read_text()
        if not typed_body:
            assert ('void __fastcall '+definitions['_output_callback_453e02']['native']+'(C2OutputDirective *directive)\n{\n    __asm { int 3 }\n}')in stub,'typed target must still be a genuine named INT3'

    aliases={r['native']:definitions[r['logical']]['native']for r in value['targets']}
    expected={(r['native'],aliases[target])for r in value['slots']for target in r['native_targets']}
    assert set(stores)==expected,'callback store alternatives missing or invented'
    names=[r['native']for r in value['slots']]
    targets=[definitions[r['logical']]['native']for r in value['targets']]
    for path in (BINARY/'src').glob('*'):
        if path.suffix not in ('.c','.h'):continue
        for line in path.read_text().splitlines():
            slot=[name for name in names if re.search(r'\b'+name+r'\b',line)]
            if slot:
                assert len(slot)==1
                name=slot[0]
                typed=contract and name=='output_slot_49971c'
                allowed_type='C2DirectiveCallback'if typed else'C2PendingCallback'
                allowed_stores={name+'.target = '+target+';'for slot,target in expected if slot==name}
                if storage and name=='output_slot_497ce4':
                    allowed_stores.remove(name+'.target = select_output_section;')
                    allowed_stores.add(name+'.target = (void (__cdecl *)()) select_output_section;')
                if typed:
                    allowed_stores.remove(name+'.target = output_callback_41eb89;')
                    allowed_stores.add(name+'.target = (C2OutputDirectiveWriter) output_callback_41eb89;')
                    allowed_stores.add(name+'.target(directive);')
                assert (path.name=='output-callbacks.c'and(line.strip()==allowed_type+' '+name+';'or line.strip()in allowed_stores))or(path.name=='output-callbacks.h'and line.strip()=='extern '+allowed_type+' '+name+';'),'pending callback use/escape requires reviewed actual callable ABI'
            target=[name for name in targets if re.search(r'\b'+name+r'\b',line)]
            if target:
                assert len(target)==1
                name=target[0]
                typed=contract and name==definitions['_output_callback_453e02']['native']
                direct=storage and name==storage['target_native']
                declaration='void __fastcall '+name+'(C2Section *section)'if direct else'void __fastcall '+name+'(C2OutputDirective *directive)'if typed else'void '+name+'()'
                noop_cast=contract and name=='output_callback_41eb89'and line.strip()=='output_slot_49971c.target = (C2OutputDirectiveWriter) output_callback_41eb89;'
                storage_use=direct and((path.name=='output-callbacks.c'and line.strip()=='output_slot_497ce4.target = (void (__cdecl *)()) select_output_section;')or(path.name=='coff-output.c'and line.strip()=='select_output_section(directive_section);'))
                assert storage_use or noop_cast or(path.name=='output-callbacks.c'and re.fullmatch(r'output_slot_[0-9a-f]+\.target = '+name+r';',line.strip()))or(path.name=='output-callbacks.h'and line.strip()==declaration+';')or(path.name=='stubs.c'and line.strip()==declaration)or(typed and typed_body and path.name=='output-callbacks.c'and line.strip()==declaration),'pending target invocation/escape requires actual ABI review'
    if storage:
        assert original(0x453eb8,5)==b'\xe8'+struct.pack('<i',0x41ec20-0x453ebd)
        assert original(0x453eb0,6)==b'\x8b\x0d'+struct.pack('<I',0x4973fc)
        slot=maps['_output_slot_497ce4'];target=maps[storage['target_symbol']]
        pattern=b'\xc7\x05'+struct.pack('<II',slot,target)
        chunks=graph['owned_code_chunks']['rebuilt']['functions']['_initialize_pass_functions']
        assert sum(rebuilt(start,end-start).count(pattern)for start,end in chunks)==1,'typed target STORAGE must retain exact MAPslot/address without code adapter'
        stub=(BINARY/'src/stubs.c').read_text()
        assert 'void __fastcall select_output_section(C2Section *section)\n{\n    __asm { int 3 }\n}'in stub
        caller=(BINARY/'src/coff-output.c').read_text()
        assert caller.count('select_output_section(directive_section);')==1
    from directive_data import verify as verify_directive_data
    directive_data=verify_directive_data(BINARY,config,graph)
    return {'directive_data':directive_data,'kind':'finite callback identities; only reviewed directive slot callable; its body credit requires separate wholefunction audit'if contract else'stores-only pending function-address identities; zero callback body/ABI credit',
            'directive_contract_sha256':config.get('directive_callback_contract_sha256'),
            'section_storage_contract_sha256':config.get('section_callback_storage_contract_sha256'),
            'boundary_sha256':config['output_callback_boundary_sha256'],
            'original_references':owned['references'],'original_calls':owned['calls'],
            'slots':[{'original':r['original'],'rebuilt':maps['_'+r['native']],'native':r['native'],'initial':0,'alternatives':r['alternatives']}for r in value['slots']],
            'targets':[{'logical':r['logical'],'original':r['original'],'rebuilt':maps[definitions[r['logical']].get('symbol',r['logical'])],'native':definitions[r['logical']]['native'],'signature':(('fastcall C2OutputDirective*ECX RET0; body separately audited'if typed_body else'fastcall C2OutputDirective*ECX RET0; body pending')if contract and r['logical']=='_output_callback_453e02'else'UNRESOLVED; exact noarg RET0 view only at49971c'if contract and r['logical']=='_output_callback_41eb89'else'fastcall C2Section*ECX RET0 only direct view; stored slot NOT callable'if storage and r['logical']=='_output_callback_41ec20'else'UNRESOLVED; invocation forbidden')}for r in value['targets']]}
