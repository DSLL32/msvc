#!/usr/bin/env python3
"""Target/source/data ownership checks plus the shared Rust graph engine."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import time
from state import BINARY, ROOT, load, mask, sha
from binary import PE


def ownership(config, pe):
    intervals = []
    for record in config['functions']:
        assert record['status'] in ('stub', 'candidate', 'accepted')
        if record['status'] == 'stub':
            assert record['extent'] is None and not record.get('text_data')
            continue
        assert record['extent'][0] == record['original']
        for start, end in [record['extent']] + record.get('text_data', []):
            assert start < end
            assert any(s['flags'] & 0x20 and pe.base+s['rva'] <= start < end <=
                       pe.base+s['rva']+min(s['size'],s['virtual_size']) for s in pe.sections)
            intervals.append((start, end, record['name']))
    for record in config['support_unmasked'] + config['loader_data']:
        assert record.get('evidence'), 'restored support/data requires evidence'
        assert (BINARY / record['evidence']).is_file()
        start, end = record['start'], record['end']
        assert start < end and pe.offset(end-1)-pe.offset(start) == end-start-1
        intervals.append((start, end, record['name']))
    for i, left in enumerate(intervals):
        for right in intervals[i+1:]:
            assert left[1] <= right[0] or right[1] <= left[0], ('overlapping ownership', left, right)
    for start, end, owner in intervals:
        for record in config['functions']:
            assert record['name'] == owner or not start <= record['original'] < end, ('another entry owned', owner, record['name'])
    return intervals


def graph_driver(functions, config, pe):
    """Transport proven tail RET extents without accepting or unmasking stubs."""
    import hashlib
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_JUMP
    from capstone.x86 import X86_OP_IMM
    driver=[]
    for record in functions:
        row=dict(name=record['symbol'],start=record['original'],
                 end=record['extent'][1] if record['extent'] else record['original']+1)
        known=record.get('abi_tail_extent')
        if known:
            assert record['status']=='stub' and record['extent'] is None and not record.get('acceptance_proof')
            assert record['name'] not in config['accepted']
            path=BINARY/known['evidence']
            assert sha(path)==known['sha256'], 'changed unaccepted tail extent proof'
            proof=json.loads(path.read_text())
            assert proof['target_sha256']==config['target']['sha256'] and proof['name']==record['name']
            start,end=proof['primary']
            assert start==record['original'] and start<end
            assert proof['code_chunks']==proof['text_data']==[], 'tail proof requires all cold chunks/owned data'
            raw=pe.read(start,end-start)
            assert hashlib.sha256(raw).hexdigest()==proof['code_sha256']
            decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
            instructions=list(decoder.disasm(raw,start))
            assert sum(i.size for i in instructions)==end-start, 'incomplete tail extent instructions'
            returns=[]
            for instruction in instructions:
                if instruction.mnemonic.startswith('ret'):
                    returns.append(dict(address=instruction.address,cleanup=int(instruction.op_str,0) if instruction.op_str else 0,bytes=instruction.bytes.hex()))
                if instruction.group(CS_GRP_JUMP):
                    assert instruction.operands[0].type==X86_OP_IMM and start<=instruction.operands[0].imm<end, 'unproved external tail/cold chunk'
            assert returns and returns==proof['returns'], 'tail RET inventory differs'
            assert not any(start<other['original']<end for other in functions), 'tail extent owns another function entry'
            image=mask(config,pe,())
            assert image[pe.offset(start):pe.offset(end)]==b'\xcc'*(end-start), 'unaccepted tail extent was unmasked'
            row.update(end=end,pending_extent=False,accepted=False,evidence=None,
                       kind='unaccepted-application-tail-target',extent_evidence=known,
                       code_chunks=[],shared=[])
        driver.append(row)
    return driver


def run(config_path, candidates=(), output=None, reuse_original=None, reuse_native=None):
    begin = time.perf_counter()
    config = json.loads(config_path.read_text())
    assert sha(ROOT / config['target']['path']) == config['target']['sha256']
    pe = PE(ROOT / config['target']['path'])
    intervals = ownership(config, pe)
    from runtime import identify
    exact_runtime = identify(ROOT/config['target']['path'], {'_main':config['target']['application_main']})
    assert exact_runtime == json.loads((BINARY/'runtime.json').read_text()), 'runtime identity index is stale'
    exact_ranges = {(r['name'],r['start'],r['end']) for r in exact_runtime['functions']+exact_runtime['imports']}
    for record in config['support_unmasked']:
        assert (record['name'],record['start'],record['end']) in exact_ranges, 'unproved runtime range'
        proof = json.loads((BINARY/record['evidence']).read_text())
        assert proof['library_sha256'] == exact_runtime['library_sha256']
        assert proof['original_sha256'] == exact_runtime['image_sha256']
        assert record in proof['selected'], 'range absent from reviewed runtime boundary'
    functions = config['functions']
    assert len({r['name'] for r in functions}) == len(functions)
    assert len({r['symbol'] for r in functions}) == len(functions)
    source = '\n'.join(path.read_text() for pattern in ('*.c','*.cpp') for path in (BINARY / 'src').glob(pattern))
    live={row['name']:row for row in json.loads((BINARY/'functions.json').read_text())['functions']}
    historical_bodies=[]
    for record in functions:
        native_name = record['native_symbol']
        declaration = record.get('source_name','main' if record['symbol'] == '_main' else record['symbol'].lstrip('_').split('@')[0])
        assert re.search(r'\b' + re.escape(declaration) + r'\s*\([^;]+?\)\s*\{', source), ('missing source body', record['name'])
        assert native_name
        if record['status']=='stub':
            stub = re.search(r'\b'+re.escape(declaration)+r'\s*\([^;{}]*\)\s*\{\s*__asm\s*\{\s*int\s+3\s*\}\s*(?:return\s+(?:0|NULL);\s*)?\}',source)
            if not stub:
                current=live.get(record['name'],{})
                assert config_path.resolve()!=(BINARY/'functions.json').resolve() and current.get('status')=='accepted' and current.get('original')==record['original'], ('stub is not a named INT3 boundary',record['name'])
                proof=BINARY/current['acceptance_proof']
                assert proof.is_file()
                historical_bodies.append(dict(name=record['name'],historical_status='stub',current_status='accepted',
                    proof=current['acceptance_proof'],proof_sha256=sha(proof),
                    scope='Current body exists; excluded from this historical mask and checked-function set.'))
    checked = [r for r in functions if r['status'] == 'accepted' or r['name'] in candidates]
    assert all(r['status'] != 'stub' and r['extent'] for r in checked), 'candidate not reconstructed'
    graph_config = {
        'target': config['target'], 'accepted': [r['symbol'] for r in checked],
        'driver': graph_driver(functions,config,pe),
    }
    destination = output.parent if output else BINARY / 'build'
    destination.mkdir(parents=True,exist_ok=True)
    generated = destination / 'graph-config.json'
    generated.write_text(json.dumps(graph_config, indent=2) + '\n')
    empty = destination / 'graph-runtime.json'
    empty.write_text('[]\n')
    import importlib.util
    spec=importlib.util.spec_from_file_location('switch_data',BINARY/'scripts/switch-data.py')
    switches=importlib.util.module_from_spec(spec);spec.loader.exec_module(switches)
    metadata,metadata_proof=switches.prepare(checked,destination,functions)
    tool = Path(os.environ.get('GRAPH_AUDIT_BIN',str(ROOT/'tooling/graph-audit/target/release/graph-audit'))).resolve()
    command = [str(tool), '--profile', 'cl', '--project', str(BINARY), '--repo', str(ROOT),
               '--original', str(ROOT / config['target']['path']), '--rebuilt', str(BINARY / 'build/CVPACK.EXE'),
               '--map', str(BINARY / 'build/cvpack.map'), '--config', str(generated),
               '--runtime', str(empty), '--rebuilt-runtime', str(empty), '--metadata',str(metadata),'--discovery', 'cfg',
               '--check-ret', '--check-imports', '--resolve-register-imports', '--check-references', '--check-ownership', '--check-extents']
    release_tail=None
    if any(row['name']=='release_widen' for row in checked):
        import importlib.util
        spec=importlib.util.spec_from_file_location('release_tail',BINARY/'scripts/release-tail.py')
        release=importlib.util.module_from_spec(spec);spec.loader.exec_module(release)
        proof=release.prove(config)
        command.remove('--check-ret')
        projection=destination/'ret-projection-config.json'
        projection.write_text(json.dumps(dict(graph_config,accepted=[name for name in graph_config['accepted'] if name!='_release_widen']),indent=2)+'\n')
        ret_command=command.copy();ret_command[ret_command.index('--config')+1]=str(projection);ret_command.append('--check-ret')
        ret_result=subprocess.run(ret_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
        assert ret_result.returncode==0,ret_result.stderr.decode()
        ret_report=destination/'ret-projection-report.json';ret_report.write_bytes(ret_result.stdout)
        release_tail=dict(proof=proof,command=ret_command,config_sha256=sha(projection),report_sha256=sha(ret_report),report=json.loads(ret_result.stdout))
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=15)
    assert result.returncode == 0, result.stderr.decode()
    graph = json.loads(result.stdout)
    import_sites=switches.check_import_sites(graph)
    factory_import=None
    if config.get('observe_widen_factory'):
        from binary import u16,u32,COFF
        policy=config['observe_widen_factory'];provider=ROOT/'toolchains/5.10/bin/MSPDB50.DLL'
        module=PE(provider)
        assert sha(provider)==policy['provider_sha256']=='cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33'
        import struct
        assert [value-module.base for value in struct.unpack('<10I',module.read(module.base+policy['table_rva'],40))]==policy['slots'],'factory canonical provider vtable differs'
        name='?fCreate@WidenTi@@SAHAAPAU1@IH@Z'
        directory=module.read(module.base+u32(module.data,module.optional+96),40)
        ordinal_base,count,functions_table,names_table,ordinals_table=(u32(directory,16),u32(directory,24),u32(directory,28),u32(directory,32),u32(directory,36))
        exports=[]
        for index in range(count):
            if module.string(module.base+u32(module.read(module.base+names_table+index*4,4),0))!=name:continue
            slot=u16(module.read(module.base+ordinals_table+index*2,2),0)
            exports.append(dict(name=name,ordinal=ordinal_base+slot,entry=module.base+u32(module.read(module.base+functions_table+slot*4,4),0)))
        assert len(exports)==1 and exports[0]['ordinal']==13,'factory actual export differs'
        definition=BINARY/'src/mspdb50.def'
        base_definition='LIBRARY MSPDB50.DLL\nEXPORTS\n    '+name+'\n'
        assert definition.read_text() in (base_definition,base_definition+'    TypesClose\n    PDBClose\n'),'factory import definition differs'
        close_imports=[]
        if definition.read_text()!=base_definition:
            close_path=BINARY/'cases/hello-debug-dll/pdb-close-imports.json'
            close=json.loads(close_path.read_text())
            assert close['target_sha256']==sha(ROOT/config['target']['path']) and close['provider_sha256']==sha(provider)
            target_pe=PE(ROOT/config['target']['path'])
            for row in close['functions']:
                matches=[]
                for index in range(count):
                    if module.string(module.base+u32(module.read(module.base+names_table+index*4,4),0))!=row['name']:continue
                    slot=u16(module.read(module.base+ordinals_table+index*2,2),0)
                    matches.append((ordinal_base+slot,u32(module.read(module.base+functions_table+slot*4,4),0)))
                assert matches==[(row['ordinal'],row['rva'])],'PDB actual named export differs'
                assert module.read(module.base+row['rva'],9)==bytes.fromhex(row['wrapper_bytes'])==bytes.fromhex('8b4c24048b01ff60')+bytes([row['vtable_slot']]),'PDB cdecl pointer wrapper differs'
                first,last=row['target_interval']
                assert target_pe.read(first,last-first)==bytes.fromhex(row['target_bytes']),'PDB caller ABI/result witness differs'
                close_imports.append(dict(name=row['name'],ordinal=row['ordinal'],rva=row['rva'],evidence_sha256=sha(close_path),runtime_reached=False))
            assert [row['name'] for row in close_imports]==['TypesClose','PDBClose']

        obj=COFF((BINARY/'build/widen.obj').read_bytes())
        bodies=[s['name'] for s in obj.symbols.values() if s.get('type')==0x20 and s['section']>0]
        assert bodies==['_create_widen','_query_widen','_emit_widen','_release_widen'],'extra target C++ function vertices'
        imports=[s['name'] for s in obj.symbols.values() if s['name'].startswith('__imp_')]
        assert imports==['__imp_'+name],'factory target import symbol differs'
        factory_import=dict(unreached_close_imports=close_imports,provider_sha256=sha(provider),export=exports[0],definition_sha256=sha(definition),object_sha256=sha(BINARY/'build/widen.obj'),target_functions=bodies)
    free_lifetimes=None
    if config.get('observe_main_frees'):
        import importlib.util
        spec=importlib.util.spec_from_file_location('main_free_evidence',BINARY/'scripts/main-free-evidence.py')
        free_evidence=importlib.util.module_from_spec(spec);spec.loader.exec_module(free_evidence)
        free_lifetimes=free_evidence.prove(config)
    rebuilt=PE(BINARY/'build/CVPACK.EXE')
    publics={}
    for line in (BINARY/'build/cvpack.map').read_text().splitlines():
        found=re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',line)
        if found:publics[found[1]]=int(found[2],16)
    symbols=subprocess.check_output(['nm','-S','--defined-only',str(BINARY/'build/native/CVPACK')],text=True)
    sizes={line.split()[-1]:int(line.split()[1],16) for line in symbols.splitlines() if len(line.split())==4}
    data=json.loads((BINARY/'data.json').read_text())
    # Historical /help indices predate these callbacks. Initial data still uses
    # the current named source index, without counting those bodies as checked.
    function_tables=switches.function_tables(list(live.values()))
    data_aliases=[]
    for item in data:
        storage=item.get('storage')
        if storage:
            owner=next(row for row in data if row['name']==storage['name'])
            offset=storage['offset'];expression=item['native_expression']
            assert 0<=offset and offset+item['size']<=sizes[owner['name']]==owner['size'],'aliased global exceeds actual storage'
            assert item['original']==owner['original']+offset,'original data alias offset differs'
            # Query the actual ELF DWARF field, not just a second copy of metadata.
            script='python import gdb,json; v=gdb.parse_and_eval('+repr(expression)+'); b=gdb.parse_and_eval('+repr(owner['name'])+'); print("DATA-ALIAS "+json.dumps([int(v.address)-int(b.address),int(v.type.sizeof)]))'
            alias_output=subprocess.check_output(['gdb','-q','-nx','-batch',str(BINARY/'build/native/CVPACK'),'-ex',script],text=True,stderr=subprocess.STDOUT)
            actual=json.loads(next(line[11:] for line in alias_output.splitlines() if line.startswith('DATA-ALIAS ')))
            assert actual==[offset,item['size']],'actual DWARF data alias offset/extent differs'
            rebuilt_address=publics['_'+owner['name']]+offset
            data_aliases.append(dict(name=item['name'],storage=storage,native_expression=expression,actual_offset=actual[0],actual_size=actual[1],rebuilt=rebuilt_address))
        else:
            assert sizes[item['name']]==item['size'], ('native global type/extent differs',item['name'])
            rebuilt_address=publics['_'+item['name']]
        if not item.get('function_table'):
            assert pe.read(item['original'],item['size'])==rebuilt.read(rebuilt_address,item['size']), ('initial Win32 data differs',item['name'])
    # The allocator exhaustion branch and both compactors share one original
    # flag. Keep that actual storage identity instead of a duplicate zero flag.
    flag=[row for row in data if row['original']==0x412d00]
    assert len(flag)==1 and flag[0]['name']=='discard_types' and flag[0]['size']==4,'discard flag data identity differs'
    assert 'global_types_exhausted' not in sizes and '_global_types_exhausted' not in publics,'duplicate discard flag storage'
    from binary import COFF
    discard_flag_references=[]
    for name,count in [('global-types',2),('modern-types',2),('legacy-types',1)]:
        path=BINARY/'build'/(name+'.obj');obj=COFF(path.read_bytes())
        references=[offset for section in obj.sections if section['name']=='.text' for offset,symbol,kind in section['relocs'] if symbol['name']=='_discard_types']
        assert len(references)==count,('consumer no longer uses the shared discard flag',name,references)
        discard_flag_references.append(dict(object=name,sha256=sha(path),symbol='_discard_types',references=references))
    assert pe.read(0x40b493,5)==bytes.fromhex('a1002d4100')
    assert pe.read(0x40b866,10)==bytes.fromhex('c705002d410001000000')
    assert pe.read(0x4027fa,6)==bytes.fromhex('392d002d4100')
    assert pe.read(0x4031d0,6)==pe.read(0x403268,6)==bytes.fromhex('391d002d4100')
    type_flag_references=[]
    alias=next(row for row in data if row['name']=='statistics_enabled')
    assert alias['original']==0x4154c2 and alias['storage']==dict(name='current_module_has_types',offset=0),'module type flag storage identity differs'
    assert 'statistics_enabled' not in sizes and '_statistics_enabled' not in publics,'duplicate module type flag storage'
    for name in ('main','pack'):
        path=BINARY/'build'/(name+'.obj');obj=COFF(path.read_bytes())
        references=[offset for section in obj.sections if section['name']=='.text' for offset,symbol,kind in section['relocs'] if symbol['name']=='_current_module_has_types']
        assert len(references)==1,('module type flag consumer reference differs',name,references)
        type_flag_references.append(dict(object=name,sha256=sha(path),symbol='_current_module_has_types',references=references))
    assert pe.read(0x404763,7)==bytes.fromhex('66391dc2544100'),'original module type flag reader differs'
    assert pe.read(0x404e88,9)==bytes.fromhex('66c705c25441000100'),'original module type flag writer differs'
    import importlib.util
    spec=importlib.util.spec_from_file_location('resource_evidence',BINARY/'scripts/resource-evidence.py')
    resources=importlib.util.module_from_spec(spec);spec.loader.exec_module(resources)
    expected=json.loads((BINARY/'cases/help/resources.json').read_text())['resources']
    actual=resources.resources_from(rebuilt)
    assert [(r['id'],r['language'],r['utf16le']) for r in actual]==[(r['id'],r['language'],r['utf16le']) for r in expected], 'encountered Win32 resources differ'
    receipt = dict(schema=1, config_sha256=sha(config_path), graph_config_sha256=sha(generated),
        original_sha256=sha(ROOT / config['target']['path']), rebuilt_sha256=sha(BINARY / 'build/CVPACK.EXE'),
        native_sha256=sha(BINARY / 'build/native/CVPACK'), map_sha256=sha(BINARY / 'build/cvpack.map'),
        graph_tool_sha256=sha(tool),graph_metadata_sha256=sha(metadata),graph_metadata_proof_sha256=sha(metadata_proof),
        switch_data_validator_sha256=sha(BINARY/'scripts/switch-data.py'), data_sha256=sha(BINARY/'data.json'),
        runtime_sha256=sha(BINARY/'runtime.json'), graph_runtime_sha256=sha(empty),
        audit_wrapper_sha256=sha(BINARY/'scripts/audit.py'),
        main_free_lifetimes=free_lifetimes, cached_import_sites=import_sites, factory_import=factory_import, function_tables=function_tables, data_aliases=data_aliases, discard_flag_references=discard_flag_references,type_flag_references=type_flag_references, data_objects_checked=len(data), resource_strings_checked=len(actual), historical_reconstructed_bodies=historical_bodies,
        resource_evidence_sha256=sha(BINARY/'cases/help/resources.json'),
        candidates=list(candidates), checked_functions=[r['name'] for r in checked],
        owned_ranges=len(intervals), graph_command=command, graph=graph, external_release_tail=release_tail,
        seconds=time.perf_counter()-begin)
    assert bool(reuse_original)==bool(reuse_native), 'reuse requires both raw captures'
    if reuse_original:
        original=json.loads((reuse_original/'receipt.json').read_text())
        native=json.loads((reuse_native/'receipt.json').read_text())
        assert native['original_receipt_sha256']==sha(reuse_original/'receipt.json')
        assert original['candidates']==list(candidates)
        assert original['configuration_sha256']==native['configuration_sha256']==sha(config_path)
        receipt['reused_capture_binding']=dict(
            original_receipt_sha256=sha(reuse_original/'receipt.json'),
            native_receipt_sha256=sha(reuse_native/'receipt.json'),
            historical_audit_sha256=sha(reuse_original/'audit.json'),
            historical_metadata_sha256=sha(reuse_original/'graph-metadata.json') if (reuse_original/'graph-metadata.json').exists() else None,
            historical_metadata_proof_sha256=sha(reuse_original/'graph-metadata-proof.json') if (reuse_original/'graph-metadata-proof.json').exists() else None,
            original_events_sha256=sha(reuse_original/'events'),
            masked_image_sha256=original['masked_image_sha256'])
    if reuse_native is not None and config.get('observe_next_hash_output_block'):
        spec=importlib.util.spec_from_file_location('native_observer_files',BINARY/'scripts/native-observer-files.py')
        observers=importlib.util.module_from_spec(spec);spec.loader.exec_module(observers)
        receipt['native_observer_files']=observers.bind(reuse_native,config)
    if output:
        output.write_text(json.dumps(receipt, indent=2) + '\n')
    return receipt


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', type=Path, default=BINARY / 'functions.json')
    parser.add_argument('--candidate', action='append', default=[])
    parser.add_argument('--output', type=Path, default=BINARY / 'build/audit.json')
    parser.add_argument('--reuse-original',type=Path)
    parser.add_argument('--reuse-native',type=Path)
    args = parser.parse_args()
    print(json.dumps(run(args.config, args.candidate, args.output,args.reuse_original,args.reuse_native), indent=2))
