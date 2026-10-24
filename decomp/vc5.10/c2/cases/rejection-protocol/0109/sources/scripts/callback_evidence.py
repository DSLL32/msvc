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
    symbol_writer=None
    if config.get('symbol_callback_contract'):
        path=BINARY/config['symbol_callback_contract'];raw=path.read_bytes()
        assert sha(raw)==config['symbol_callback_contract_sha256']
        symbol_writer=json.loads(raw)
        assert symbol_writer['target_sha256']==sha(TARGET.read_bytes())
        assert symbol_writer['historical_identity_boundary_sha256']==config['output_callback_boundary_sha256']
        assert symbol_writer['slot']==next(r for r in value['slots']if r['original']==0x497cec)
        assert symbol_writer['type']=='void (__fastcall *)(C2Symbol *symbol)'
        assert symbol_writer['fields']=={'bytes':4,'target_offset':0,'alignment':4}
        assert [(r['logical'],r['entry'],r['symbol'])for r in symbol_writer['targets']]==[('_output_callback_41fa04',0x41fa04,'@output_callback_41fa04@4'),('_output_callback_4849fa',0x4849fa,'@output_callback_4849fa@4')]
    definitions={r['name']:r for r in config['driver']+config['stubs']}

    typed_body=False; direct_body=False
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
            direct_body=r['logical']in config['accepted']+config['trial']
            if direct_body:
                assert not item['pending_extent'] and r['logical']in graph['functions'],'direct callback identity needs ordinary wholefunction audit'
            else:assert item['end']==r['original']+1 and item['pending_extent']
        elif symbol_writer and r['logical']in ('_output_callback_41fa04','_output_callback_4849fa'):
            target=next(t for t in symbol_writer['targets']if t['logical']==r['logical'])
            assert item['native']==target['native']==r['native']and item['symbol']==target['symbol']
            assert not item['pending_signature']and item['pending_extent']and item['end']==r['original']+1
            assert item['abi']=='void fastcall C2Symbol*symbolECX; no stackargs/RET0/resultignored; body pendingonebyteINT3, zero credit'
            assert r['logical']not in config['accepted']+config['trial'],'typedpending target stillneeds ownwholefunction reconstruction'
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
                typed_symbol=symbol_writer and name=='output_slot_497cec'
                allowed_type='C2SymbolCallback'if typed_symbol else'C2DirectiveCallback'if typed else'C2PendingCallback'
                allowed_stores={name+'.target = '+target+';'for slot,target in expected if slot==name}
                if storage and name=='output_slot_497ce4':
                    allowed_stores.remove(name+'.target = select_output_section;')
                    allowed_stores.add(name+'.target = (void (__cdecl *)()) select_output_section;')
                if typed:
                    allowed_stores.remove(name+'.target = output_callback_41eb89;')
                    allowed_stores.add(name+'.target = (C2OutputDirectiveWriter) output_callback_41eb89;')
                    allowed_stores.add(name+'.target(directive);')
                symbol_use=typed_symbol and path.name=='code-records.c'and line.strip()=='output_slot_497cec.target(pending_debug_symbols->symbol);'
                assert symbol_use or(path.name=='output-callbacks.c'and(line.strip()==allowed_type+' '+name+';'or line.strip()in allowed_stores))or(path.name=='output-callbacks.h'and line.strip()=='extern '+allowed_type+' '+name+';'),'pending callback use/escape requires reviewed actual callable ABI'
            target=[name for name in targets if re.search(r'\b'+name+r'\b',line)]
            if target:
                assert len(target)==1
                name=target[0]
                typed=contract and name==definitions['_output_callback_453e02']['native']
                direct=storage and name==storage['target_native']
                typed_symbol=symbol_writer and name in ('output_callback_41fa04','output_callback_4849fa')
                declaration='void __fastcall '+name+'(C2Symbol *symbol)'if typed_symbol else'void __fastcall '+name+'(C2Section *section)'if direct else'void __fastcall '+name+'(C2OutputDirective *directive)'if typed else'void '+name+'()'
                noop_cast=contract and name=='output_callback_41eb89'and line.strip()=='output_slot_49971c.target = (C2OutputDirectiveWriter) output_callback_41eb89;'
                storage_use=direct and((path.name=='output-callbacks.c'and line.strip()=='output_slot_497ce4.target = (void (__cdecl *)()) select_output_section;')or(path.name=='coff-output.c'and line.strip()=='select_output_section(directive_section);'))
                assert storage_use or noop_cast or(path.name=='output-callbacks.c'and re.fullmatch(r'output_slot_[0-9a-f]+\.target = '+name+r';',line.strip()))or(path.name=='output-callbacks.h'and line.strip()==declaration+';')or(path.name=='stubs.c'and line.strip()==declaration)or(typed and typed_body and path.name=='output-callbacks.c'and line.strip()==declaration)or(direct and direct_body and path.name=='coff-output.c'and line.strip()==declaration),'pending target invocation/escape requires actual ABI review'
    if storage:
        assert original(0x453eb8,5)==b'\xe8'+struct.pack('<i',0x41ec20-0x453ebd)
        assert original(0x453eb0,6)==b'\x8b\x0d'+struct.pack('<I',0x4973fc)
        slot=maps['_output_slot_497ce4'];target=maps[storage['target_symbol']]
        pattern=b'\xc7\x05'+struct.pack('<II',slot,target)
        chunks=graph['owned_code_chunks']['rebuilt']['functions']['_initialize_pass_functions']
        assert sum(rebuilt(start,end-start).count(pattern)for start,end in chunks)==1,'typed target STORAGE must retain exact MAPslot/address without code adapter'
        stub=(BINARY/'src/stubs.c').read_text()
        if not direct_body:assert 'void __fastcall select_output_section(C2Section *section)\n{\n    __asm { int 3 }\n}'in stub
        caller=(BINARY/'src/coff-output.c').read_text()
        assert caller.count('select_output_section(directive_section);')==1
    assembly_certificate=verify_assembly_addresses(config,graph,maps,original,rebuilt)if config.get('assembly_visitor_contract')else None
    symbol_certificate=verify_symbol_writer(symbol_writer,config,graph,maps,original,rebuilt)if symbol_writer else None
    from directive_data import verify as verify_directive_data
    directive_data=verify_directive_data(BINARY,config,graph)
    return {'assembly_addresses':assembly_certificate,'symbol_callback':symbol_certificate,'directive_data':directive_data,'kind':'finite callback identities; only reviewed directive slot callable; its body credit requires separate wholefunction audit'if contract else'stores-only pending function-address identities; zero callback body/ABI credit',
            'directive_contract_sha256':config.get('directive_callback_contract_sha256'),
            'section_storage_contract_sha256':config.get('section_callback_storage_contract_sha256'),
            'boundary_sha256':config['output_callback_boundary_sha256'],
            'original_references':owned['references'],'original_calls':owned['calls'],
            'slots':[{'original':r['original'],'rebuilt':maps['_'+r['native']],'native':r['native'],'initial':0,'alternatives':r['alternatives']}for r in value['slots']],
            'targets':[{'logical':r['logical'],'original':r['original'],'rebuilt':maps[definitions[r['logical']].get('symbol',r['logical'])],'native':definitions[r['logical']]['native'],'signature':(('fastcall C2OutputDirective*ECX RET0; body separately audited'if typed_body else'fastcall C2OutputDirective*ECX RET0; body pending')if contract and r['logical']=='_output_callback_453e02'else'UNRESOLVED; exact noarg RET0 view only at49971c'if contract and r['logical']=='_output_callback_41eb89'else'fastcall C2Section*ECX RET0 direct view; body separately audited; stored slot NOT callable'if storage and r['logical']=='_output_callback_41ec20'else'fastcall C2Symbol*ECX RET0; typed only at497cec; pendingINT3/no bodycredit'if symbol_writer and r['logical']in ('_output_callback_41fa04','_output_callback_4849fa')else'UNRESOLVED; invocation forbidden')}for r in value['targets']]}


def verify_symbol_writer(contract,config,graph,maps,original,rebuilt):
    """Finite declared slot/caller/ABI certificate; common Rust owns all CFG."""
    assert sha((BINARY/'cases/analysis/symbol-callback-disassembly.txt').read_bytes())==contract['disassembly_sha256']
    assert sha((BINARY/contract['stack_witness']['path']).read_bytes())==contract['stack_witness']['sha256']
    assert sha((BINARY/contract['call_inventory']['path']).read_bytes())==contract['call_inventory']['sha256']
    assert original(0x41f53f,9)==b'\x8b\x48\x04\xff\x15'+struct.pack('<I',0x497cec)
    assert original(0x442f2f,2)==b'\x8b\xce'and original(0x442f45,6)==b'\xff\x15'+struct.pack('<I',0x497cec)
    assert original(0x41fada,2)==b'\x8b\xcf'and original(0x41fadf,5)==b'\xe8'+struct.pack('<i',0x41fa04-0x41fae4)
    assert original(0x43414f,2)==b'\x8b\xe9'and original(0x43415d,7)==b'\x8b\xcd\xe8'+struct.pack('<i',0x41fa04-0x434164)
    assert [(r['call'],r['load'],r['stack_arguments'],r['return_cleanup'],r['result'])for r in contract['callers']]==[(0x41f542,0x41f53f,0,0,'ignored'),(0x442f45,0x442f2f,0,0,'ignored')]
    assert contract['recursive_call']['call']==0x41fadf and contract['recursive_call']['target']==0x41fa04 and contract['recursive_call']['stack_arguments']==0 and contract['recursive_call']['result']=='ignored'
    assert contract['additional_direct_call']['call']==0x43415f and contract['additional_direct_call']['target']==0x41fa04 and contract['additional_direct_call']['stack_arguments']==0 and contract['additional_direct_call']['result']=='ignored'
    assert contract['stack_witness']['RET0_helpers']==[0x41fd8e,0x41fd9e]and contract['stack_witness']['RET16_helper']==0x41fcb0 and contract['stack_witness']['RET16_sites']==[0x41fd83,0x469e00,0x469e56]
    for target in contract['targets']:
        expected=([0x41fa04,0x41fc96],[[0x469d0c,0x469d5d]],[0x41faa7,0x41fb50,0x41fc66,0x469d36],[[0x41fca0,0x41fcb0]])if target['entry']==0x41fa04 else([0x4849fa,0x484a0a],[],[0x484a09],[])
        assert (target['primary'],target['code_chunks'],target['normal_ret0_sites'],target['text_data'])==expected,'symbolcallback ABI proof interval/normalreturns differ'
        start,end=target['primary'];assert start==target['entry']
        assert sha(original(start,end-start))==target['body_sha256']
        assert [sha(original(a,z-a))for a,z in target['code_chunks']]==target['extra_sha256s']
        assert all(original(at,1)==b'\xc3'for at in target['normal_ret0_sites'])
        stub='void __fastcall '+target['native']+'(C2Symbol *symbol)\n{\n    __asm { int 3 }\n}'
        assert stub in (BINARY/'src/stubs.c').read_text(),'typedcallback target must remain genuine named INT3'
    assert original(0x4849fa,16)==bytes.fromhex('8a41043c0174043c04750480493101c3')
    assert struct.unpack('<4I',original(0x41fca0,16))==(0x41fbf7,0x41fc7d,0x41fc67,0x41fbed)
    assert original(0x41fd9d,1)==b'\xc3'and all(original(at,3)==b'\xc2\x10\0'for at in (0x41fd83,0x469e00,0x469e56))
    header=(BINARY/'src/output-callbacks.h').read_text()
    assert 'typedef void (__fastcall *C2SymbolWriter)(C2Symbol *symbol);'in header
    assert 'typedef struct C2SymbolCallback { C2SymbolWriter target; } C2SymbolCallback;'in header
    source=(BINARY/'src/code-records.c').read_text()
    exact='void write_pending_debug_symbols(void)\n{\n    while (pending_debug_symbols) {\n        output_slot_497cec.target(pending_debug_symbols->symbol);\n        pending_debug_symbols = pending_debug_symbols->next;\n    }\n}'
    assert source.count(exact)==1 and source.count('output_slot_497cec.target(')==1,'typed497cec consumer must reload actualglobal root after callback'
    owner='_write_pending_debug_symbols';assert owner in graph['functions']
    site=contract['canonical_callsite'];assert site['owner']==owner
    start=maps[owner];slot=maps['_output_slot_497cec']
    root=maps['_pending_debug_symbols']
    assert rebuilt(start+site['root_load_offset'],5)==b'\xa1'+struct.pack('<I',root),'canonical symbolcall must read actual sharedqueue'
    assert rebuilt(start+site['reload_advance_offset'],14)==b'\x8b\x0d'+struct.pack('<I',root)+b'\x8b\x11\x89\x15'+struct.pack('<I',root),'canonical consumer must reload and advance actualglobalroot aftercallback'
    assert rebuilt(start+site['offset'],6)==b'\xff\x15'+struct.pack('<I',slot),'canonical symbolcall must use exact qualified slot'
    assert rebuilt(start+site['load_offset'],len(bytes.fromhex(site['load_bytes'])))==bytes.fromhex(site['load_bytes'])
    assert bytes.fromhex(site['load_bytes']).endswith(b'\x8b\x48\x04'),'actual symbol must come from genuine link+4 intoECX'
    for target in contract['targets']:
        pattern=b'\xc7\x05'+struct.pack('<II',slot,maps[target['symbol']])
        chunks=graph['owned_code_chunks']['rebuilt']['functions']['_initialize_pass_functions']
        assert sum(rebuilt(a,z-a).count(pattern)for a,z in chunks)==1,'typed symboltarget initializer identity differs'
    return {'contract_sha256':config['symbol_callback_contract_sha256'],'slot':0x497cec,'canonical_slot':slot,'canonical_callsite':start+site['offset'],'targets':[{'logical':r['logical'],'original':r['entry'],'canonical':maps[r['symbol']],'pending':True,'body_credit':0}for r in contract['targets']],'scope':'Only497cec C2Symbol*ECX/RET0 callback view, exact two alternatives/fourcallers and typedconsumer. CurrentNULLqueue invokesneither. Other19slotguardspreserved; originaltargethashes areanalysis evidence, not unmask/acceptance.'}


def verify_assembly_addresses(config,graph,maps,original,rebuilt):
    """Six explicit code-address passes only; no callable visitor ABI claim."""
    path=BINARY/config['assembly_visitor_contract'];raw=path.read_bytes()
    assert sha(raw)==config['assembly_visitor_contract_sha256'];value=json.loads(raw)
    assert value['target_sha256']==sha(TARGET.read_bytes())
    assert value['primary']==[0x41f559,0x41f567]and value['code_chunks']==[[0x469c70,0x469cbe]]and value['code_bytes']==92
    assert value['passes']==[[0,0x4894ca],[0,0x489596],[1,0x489596],[0,0x489693],[1,0x489693],[0,0x48a251]]
    assert sha(original(0x41f559,14))==value['primary_sha256']and sha(original(0x469c70,78))==value['cold_sha256']
    assert sha(original(0x42ba01,62))==value['visitor_sha256']and original(0x42ba3e,1)==b'\xc3'
    assert sha((BINARY/'cases/analysis/assembly-visitor-disassembly.txt').read_bytes())==value['disassembly_sha256']
    assert sha((BINARY/'src/assembly-output.c').read_bytes())==value['source_sha256']
    definitions={r['name']:r for r in config['driver']+config['stubs']}
    visitor=definitions['@visit_symbols@8'];assert visitor['native']=='visit_symbols'and visitor['start']==0x42ba01 and visitor['end']==0x42ba3f and visitor['pending_extent']and visitor['extent_known_for_tail_abi']
    assert visitor['requires_resolved_signatures']==['_assembly_callback_4894ca','_assembly_callback_489596','_assembly_callback_489693','_assembly_callback_48a251']
    assert '@visit_symbols@8'not in config['accepted']+config['trial'],'iterator consumer must recover all actual callback signatures before body acceptance'
    for row in value['callbacks']:
        d=definitions[row['name']];assert d['start']==row['entry']and d['end']==row['entry']+1 and d['pending_extent']and d['pending_signature']
        assert row['name']not in config['accepted']+config['trial'],'unknown callback ABI/body remains unaccepted'
    header=(BINARY/'src/output-callbacks.h').read_text();assert 'typedef void (__cdecl *C2AssemblyVisitor)();'in header
    assert 'void __fastcall visit_symbols(int local, C2AssemblyVisitor visitor);'in header
    stub=(BINARY/'src/stubs.c').read_text();assert 'void __fastcall visit_symbols(int local, C2AssemblyVisitor visitor)\n{\n    __asm { int 3 }\n}'in stub
    for row in value['callbacks']:
        native=row['name'][1:];assert 'void '+native+'()\n{\n    __asm { int 3 }\n}'in stub
    exact='void write_assembly_symbols(void)\n{\n    if (assembly_output_active) {\n'+''.join('        visit_symbols(%d, assembly_callback_%x);\n'%(flag,address)for flag,address in value['passes'])+'    }\n}'
    source=(BINARY/'src/assembly-output.c').read_text();assert source.count(exact)==1 and source.count('visit_symbols(')==6,'all six function-address values belong to one guarded assembly producer'
    target_names=[r['name'][1:]for r in value['callbacks']]
    for file in (BINARY/'src').glob('*'):
        if file.suffix not in ('.c','.h'):continue
        for line in file.read_text().splitlines():
            names=[n for n in target_names if re.search(r'\b'+n+r'\b',line)]
            if names:
                assert len(names)==1;name=names[0]
                allowed=(file.name=='output-callbacks.h'and line.strip()=='void '+name+'();')or(file.name=='stubs.c'and line.strip()=='void '+name+'()')or(file.name=='assembly-output.c'and any(line.strip()=='visit_symbols(%d, %s);'%(flag,name)for flag,address in value['passes']if name=='assembly_callback_%x'%address))
                assert allowed,'unresolved assembly callback invoked or escaped beyond approved scalar address pass'
            if re.search(r'\bvisit_symbols\b',line):
                allowed=(file.name=='output-callbacks.h'and line.strip()=='void __fastcall visit_symbols(int local, C2AssemblyVisitor visitor);')or(file.name=='stubs.c'and line.strip()=='void __fastcall visit_symbols(int local, C2AssemblyVisitor visitor)')or(file.name=='assembly-output.c'and any(line.strip()=='visit_symbols(%d, assembly_callback_%x);'%(flag,address)for flag,address in value['passes']))
                assert allowed,'visitor hardstub/knownscalar address-pass ownership changed'
    owner='_write_assembly_symbols';assert owner in graph['functions']
    assert graph['functions'][owner]['edges']==['@visit_symbols@8']
    assert graph['original_accepted_graphs'][owner]['references']==sorted([1]+[r['entry']for r in value['callbacks']]),'all four genuine CODE references must remain separate'
    site=value['canonical_passes'];assert len(site)==6
    base=maps[owner];target=maps['@visit_symbols@8']
    for row,(flag,address)in zip(site,value['passes']):
        native='assembly_callback_%x'%address
        assert row['flag']==flag and row['original_target']==address
        assert rebuilt(base+row['pointer_offset'],5)==b'\xba'+struct.pack('<I',maps['_'+native]),'actual code pointer must be second EDX argument'
        expected=b'\x33\xc9'if flag==0 else b'\xb9\x01\0\0\0'
        assert rebuilt(base+row['flag_offset'],len(expected))==expected,'actual scopeflag must be first ECX argument'
        call=base+row['call_offset'];assert rebuilt(call,5)==b'\xe8'+struct.pack('<i',target-call-5)
    return {'contract_sha256':config['assembly_visitor_contract_sha256'],'passes':value['passes'],'callback_count':4,'callback_ABIs':'UNRESOLVED; addresses only','visitor':'knownECXflag/EDXpointer/RET0 but bodymaskedINT3; accepting actual indirect consumer remains blocked','body_credit':0,'scope':'Six exact scalarcodeaddress expressions, no callback calls or interpretation; original whole92B producer graph/refs/ownership checked by commonRust. Currentflag0 executes none.'}
