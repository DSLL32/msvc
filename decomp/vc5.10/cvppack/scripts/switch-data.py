"""Verify encountered compiler switch data from COFF relocations and linked bytes."""
import json
import re
import struct
from state import BINARY, ROOT, sha
from binary import COFF, PE, u32


def function_tables(available, original=None, rebuilt=None):
    """Check the encountered typed callback data using real linked symbols."""
    original=original or PE(ROOT/'toolchains/5.10/bin/CVPACK.EXE')
    rebuilt=rebuilt or PE(BINARY/'build/CVPACK.EXE')
    publics={}
    for line in (BINARY/'build/cvpack.map').read_text().splitlines():
        match=re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',line)
        if match:publics[match[1]]=int(match[2],16)
    proof=[]
    for item in json.loads((BINARY/'data.json').read_text()):
        if not item.get('function_table'):continue
        spec=item['function_table'] if isinstance(item['function_table'],dict) else {}
        stride=spec.get('stride',8);pointer=spec.get('pointer_offset',4)
        assert stride>=4 and 0<=pointer<=stride-4 and item['size']%stride==0
        before=original.read(item['original'],item['size'])
        after=rebuilt.read(publics['_'+item['name']],item['size']);rows=[]
        for offset in range(0,item['size'],stride):
            a=before[offset:offset+stride];b=after[offset:offset+stride]
            assert a[:pointer]==b[:pointer] and a[pointer+4:]==b[pointer+4:],'symbol rewrite table kind/reserved differs'
            if stride==8 and pointer==4:
                kind,reserved=struct.unpack_from('<HH',a);assert reserved==0,'symbol rewrite table kind/reserved differs'
            else:kind=None
            address=u32(a,pointer);other=u32(b,pointer)
            if address:
                function=next(row for row in available if row['original']==address)
                assert other==publics[function['symbol']],'symbol rewrite table callback identity differs'
                name=function['name']
            else:
                assert other==0,'symbol rewrite table NULL callback differs'
                name=None
            rows.append(dict(kind=kind,function=name))
        proof.append(dict(name=item['name'],original=item['original'],rebuilt=publics['_'+item['name']],size=item['size'],stride=stride,pointer_offset=pointer,rows=rows))
    return proof


def prepare(checked,destination,available=None):
    metadata=dict(schema=1,functions={});proof=dict(schema=1,tables=[])
    evidence=BINARY/'cases/hello-debug-dll/import-preservation.json'
    preservation=json.loads(evidence.read_text())
    provider=ROOT/preservation['provider']['path'];module=PE(provider)
    start,end=preservation['provider']['function']
    import hashlib
    assert sha(provider)==preservation['provider']['sha256']=='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
    assert hashlib.sha256(module.read(start,end-start)).hexdigest()==preservation['provider']['function_sha256']
    assert module.read(start,2)==bytes.fromhex('5156')
    assert all(module.read(address,3)==bytes.fromhex('5e59c3') for address in (0x780013d2,0x780013ef))
    assert sha(ROOT/preservation['target']['path'])==preservation['target']['sha256']
    metadata['register_imports']={}
    metadata['register_imports']['preservation_contracts']=[{'target':{'import':preservation['qualified_import']},'preserves':['esi'],
        'evidence':dict(path=str(evidence.relative_to(BINARY)),sha256=sha(evidence))}]
    proof['register_preservation_evidence_sha256']=sha(evidence)
    if any(row['name']=='reset_type_name_cache' for row in checked):
        extended=BINARY/'cases/hello-debug-dll/type-name-cache-import.json'
        witness=json.loads(extended.read_text())
        assert witness['base_evidence_sha256']==sha(evidence)
        assert witness['target_sha256']==sha(ROOT/preservation['target']['path'])
        assert witness['provider']['sha256']==sha(provider)
        assert witness['provider']['function_sha256']==preservation['provider']['function_sha256']
        assert sha(ROOT/witness['sdk']['path'])==witness['sdk']['sha256']
        metadata['register_imports']['preservation_contracts'][0].update(preserves=['esi','ebp'],
            evidence=dict(path=str(extended.relative_to(BINARY)),sha256=sha(extended)))
        metadata['functions']['_reset_type_name_cache']=dict(register_imports=dict(require_all_register_calls_resolved=True))
        proof['extended_free_preservation_evidence_sha256']=sha(extended)

    additional=BINARY/'cases/hello-debug-dll/cached-import-contracts.json'
    contracts=json.loads(additional.read_text())
    target=PE(ROOT/preservation['target']['path'])
    assert contracts['target_sha256']==sha(ROOT/preservation['target']['path'])
    assert contracts['provider_sha256']==sha(provider)
    available_names={row['symbol'] for row in available} if available is not None else None
    for row in contracts['contracts']:
        # Old immutable help trials have a smaller named driver index. A claim
        # for an absent, unreachable helper cannot be resolved by that index.
        if available_names is not None and row['target'].get('function') and row['target']['function'] not in available_names:continue
        if 'sdk_header' in row:
            assert sha(ROOT/row['sdk_header'])==row['sdk_header_sha256'], 'changed ABI declaration'
        owner=module if row['target'].get('import',{}).get('dll')=='msvcrt.dll' else target
        if row.get('provider_sha256'):
            assert row['provider_sha256']==sha(provider)
        for witness in row['static_witnesses']:
            assert hashlib.sha256(owner.read(witness['start'],witness['end']-witness['start'])).hexdigest()==witness['sha256'], 'changed preservation body'
        if 'executed_witness' in row:
            observed=row['executed_witness'];events=BINARY/observed['events']
            assert sha(events)==observed['events_sha256']
            assert sha(events.parent/'receipt.json')==observed['receipt_sha256']
            assert observed['edi_before']==observed['edi_after']
        metadata['register_imports']['preservation_contracts'].append(dict(target=row['target'],preserves=row['preserves'],
            evidence=dict(path=str(additional.relative_to(BINARY)),sha256=sha(additional))))
    proof['additional_register_preservation_evidence_sha256']=sha(additional)
    for name in ('_parse_options','_print_banner','_fatal_error','_read_image_debug','_create_mapping'):
        if any(row['symbol']==name for row in checked):
            metadata['functions'][name]=dict(register_imports=dict(require_all_register_calls_resolved=True))
    if any(row['name']=='intern_type' for row in checked):
        metadata['functions']['@intern_type@4']=dict(register_imports=dict(require_all_register_calls_resolved=True))
    if any(row['name']=='add_type_reference' for row in checked):
        path=BINARY/'cases/hello-debug-dll/type-reference-import.json'
        witness=json.loads(path.read_text())
        assert witness['target_sha256']==sha(ROOT/preservation['target']['path'])
        assert witness['provider_sha256']==sha(provider)
        assert witness['existing_free_witness_sha256']==sha(BINARY/'cases/hello-debug-dll/type-name-cache-import.json')
        assert witness['existing_callee_witness_sha256']==sha(additional)
        assert witness['register']=='ebx'
        for name,owner in [('free',module),('allocate_cleared',target)]:
            row=witness[name]
            assert hashlib.sha256(owner.read(row['start'],row['end']-row['start'])).hexdigest()==row['sha256']
        assert target.read(witness['target']['load'],6)==bytes.fromhex('8b1dac614100')
        assert all(target.read(site,2)==bytes.fromhex('ffd3') for site in witness['target']['sites'])
        claims=metadata['register_imports']['preservation_contracts']
        for row in claims:
            if row['target'] in ({'import':witness['free']['qualified_import']},{'function':'_allocate_cleared'}):
                row['preserves'].append('ebx')
                row['evidence']=dict(path=str(path.relative_to(BINARY)),sha256=sha(path))
        proof['type_reference_import_witness_sha256']=sha(path)
        metadata['functions']['@add_type_reference@8']=dict(register_imports=dict(require_all_register_calls_resolved=True))
    if any(row['symbol']=='_free_widen_buffers' for row in checked):
        metadata['functions']['_free_widen_buffers']=dict(register_imports=dict(require_all_register_calls_resolved=True))
    if any(row['name']=='free_type_tables' for row in checked):
        metadata['functions']['_free_type_tables']=dict(register_imports=dict(require_all_register_calls_resolved=True))
    metadata['functions']['_main']=dict(register_imports=dict(require_all_register_calls_resolved=False))
    if any(row['name']=='pack_module' for row in checked):
        path=BINARY/'build/pack.obj';obj=COFF(path.read_bytes())
        symbols={row['name']:row for row in obj.symbols.values()}
        function=symbols['_pack_module'];following=symbols['_initialize_single_module']
        assert function['section']==following['section'] and function['type']==following['type']==0x20
        section=obj.sections[function['section']-1];code=section['data']
        maps=[];jumps=[]
        for offset,target,kind in section['relocs']:
            if not function['value']<=offset<following['value'] or kind!=6 or target['section']!=function['section']:continue
            if code[offset-2:offset]==b'\x8a\x90':maps.append((offset,target))
            if code[offset-3:offset]==b'\xff\x24\x95':jumps.append((offset,target))
        assert len(maps)==len(jumps)==1,'switch operand COFF proof changed'
        map_operand,classes=maps[0];jump_operand,table=jumps[0]
        assert u32(code,map_operand)==u32(code,jump_operand)==0
        start=table['value'];middle=classes['value'];end=middle+15
        assert start+6*4==middle and end==following['value'] and code[start-1]==0xc3,'switch object boundaries changed'
        rows=[row for row in section['relocs'] if start<=row[0]<middle]
        assert [row[0] for row in rows]==list(range(start,middle,4))
        publics={}
        for line in (BINARY/'build/cvpack.map').read_text().splitlines():
            match=re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',line)
            if match:publics[match[1]]=int(match[2],16)
        base=publics['_pack_module']-function['value']
        pe=PE(BINARY/'build/CVPACK.EXE');original=PE(ROOT/'toolchains/5.10/bin/CVPACK.EXE')
        assert publics['_initialize_single_module']==base+end
        remap=code[middle:end]
        assert len(remap)==15 and all(index<6 for index in remap)
        assert remap==original.read(0x404ec8,15)==pe.read(base+middle,15),'class map contents changed'
        targets=[]
        for offset,target,kind in rows:
            assert kind==6 and target['section']==function['section'] and function['value']<=target['value']<start
            address=base+target['value']+u32(code,offset)
            assert pe.read(base+offset,4)==struct.pack('<I',address),'linked switch target/COFF relocation differs'
            targets.append(address)
        assert pe.read(base+map_operand,4)==struct.pack('<I',base+middle)
        assert pe.read(base+jump_operand,4)==struct.pack('<I',base+start)
        metadata['functions']['_pack_module']=dict(rebuilt=dict(text_data=[[base+start,base+end]],switches={str(base+jump_operand-3):targets}))
        frozen=destination/'pack-switch.obj';frozen.write_bytes(path.read_bytes())
        proof['tables'].append(dict(owner='_pack_module',object_sha256=sha(frozen),object='pack-switch.obj',
            rebuilt_sha256=sha(BINARY/'build/CVPACK.EXE'),map_sha256=sha(BINARY/'build/cvpack.map'),
            coff_function=function,coff_next_function=following,coff_pointer_table=table,coff_class_map=classes,
            pointer_relocations=[dict(offset=offset,symbol=target,kind=kind) for offset,target,kind in rows],
            rebuilt_data=[base+start,base+end],rebuilt_targets=targets,class_map=remap.hex(),
            original_class_map=[0x404ec8,0x404ed7]))
    if any(row['name']=='rewrite_module_symbols' for row in checked):
        assert available is not None
        proof['function_tables']=function_tables(available)
    (destination/'graph-metadata.json').write_text(json.dumps(metadata,indent=2)+'\n')
    (destination/'graph-metadata-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    return destination/'graph-metadata.json',destination/'graph-metadata-proof.json'


def check_import_sites(graph):
    """Check report callsites where aggregate import sets cannot show omissions."""
    evidence=BINARY/'cases/hello-debug-dll/import-preservation.json'
    witness=json.loads(evidence.read_text())
    report=graph['functions']['_main']['register_imports']
    identity=witness['qualified_import']
    original=report['original'];rebuilt=report['rebuilt']
    expected={str(site):identity for site in witness['target']['sites']}
    assert original['resolved_register_calls']==expected, 'main cached free site inventory differs'
    assert original['unresolved_register_calls']==[witness['dynamic_callback']['site']], 'main unknown register call is not the genuine checksum callback'
    assert original['other_unresolved_calls']==[], 'unexpected original opaque call'
    assert rebuilt['resolved_register_calls']=={} and rebuilt['unresolved_register_calls']==[], 'unexpected rebuilt register call'
    calls={site:name for site,name in rebuilt['direct_import_calls'].items() if name==identity}
    assert len(calls)==5, 'rebuilt five free calls missing'
    # Original CL /Od emits the single genuine typed checksum call through its
    # main-frame local at EBP-32. This target-specific instruction witness avoids
    # treating any arbitrary unknown memory call as an approved callback.
    publics={}
    for line in (BINARY/'build/cvpack.map').read_text().splitlines():
        match=re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',line)
        if match:publics[match[1]]=int(match[2],16)
    callback=publics['_main']+0x227
    assert rebuilt['other_unresolved_calls']==[callback], 'unexpected rebuilt opaque callback'
    assert PE(BINARY/'build/CVPACK.EXE').read(callback,3)==bytes.fromhex('ff55e0'), 'rebuilt checksum callback instruction changed'
    reference_sites=None
    if '@add_type_reference@8' in graph['functions']:
        path=BINARY/'cases/hello-debug-dll/type-reference-import.json'
        typed=json.loads(path.read_text());report=graph['functions']['@add_type_reference@8']['register_imports']
        expected={str(site):identity for site in typed['target']['sites']}
        assert report['original']['resolved_register_calls']==expected,'type reference cached free inventory differs'
        assert report['original']['unresolved_register_calls']==report['original']['other_unresolved_calls']==[],'type reference has unknown original calls'
        assert report['rebuilt']['resolved_register_calls']=={} and report['rebuilt']['unresolved_register_calls']==report['rebuilt']['other_unresolved_calls']==[],'type reference has unknown rebuilt calls'
        actual={site:name for site,name in report['rebuilt']['direct_import_calls'].items() if name==identity}
        assert len(actual)==2,'type reference rebuilt two free calls missing'
        reference_sites=dict(evidence_sha256=sha(path),original_free_sites=typed['target']['sites'],rebuilt_free_sites=sorted(map(int,actual)))
    scratch_sites=None
    if '_free_widen_buffers' in graph['functions']:
        scratch=graph['functions']['_free_widen_buffers']['register_imports']
        assert scratch['original']['resolved_register_calls']=={str(site):identity for site in (0x40523d,0x405249)},'scratch cached free site inventory differs'
        assert scratch['original']['unresolved_register_calls']==scratch['original']['other_unresolved_calls']==[],'unknown scratch original call'
        assert scratch['rebuilt']['resolved_register_calls']=={} and scratch['rebuilt']['unresolved_register_calls']==scratch['rebuilt']['other_unresolved_calls']==[],'unknown scratch rebuilt call'
        imported={site:name for site,name in scratch['rebuilt']['direct_import_calls'].items() if name==identity}
        assert len(imported)==2,'scratch rebuilt two free calls missing'
        scratch_sites=dict(original=[0x40523d,0x405249],rebuilt=sorted(map(int,imported)))
    result=dict(scratch_cleanup_free_sites=scratch_sites,evidence_sha256=sha(evidence),original_free_sites=witness['target']['sites'],
        original_checksum_callback=witness['dynamic_callback']['site'],rebuilt_free_sites=sorted(map(int,calls)),
        rebuilt_checksum_callback=callback,
        scope='Per-site report checks; no secondary CFG or import inference.')
    if reference_sites is not None:result['type_reference_sites']=reference_sites
    return result
