#!/usr/bin/env python3
"""Validate fresh raw evidence against an immutable candidate trial."""
import argparse
import json
import os
from pathlib import Path
import re
from state import BINARY, ROOT, load, mask, sha
from capture_original_constants import CRT_SHA


def checked_files(directory, receipt):
    for item in receipt['outputs']:
        path = directory / item['name']
        assert path.is_file() and path.stat().st_size == item['bytes'] and sha(path) == item['sha256'], ('changed raw capture', path)


def validate_pair(config_path, original_dir, native_dir, audit_path=None):
    config_raw = config_path.read_bytes()
    config = json.loads(config_raw)
    from binary import PE
    target = ROOT / config['target']['path']
    assert sha(target) == config['target']['sha256']
    original = json.loads((original_dir / 'receipt.json').read_text())
    native = json.loads((native_dir / 'receipt.json').read_text())
    from caseio import definition,master
    case=definition(original.get('case','help'))
    if 'case_definition_sha256' in original:
        assert original['case_definition_sha256']==sha(original_dir/'case-definition.json')
        assert native['case_definition_sha256']==sha(native_dir/'case-definition.json')
        assert (original_dir/'case-definition.json').read_bytes()==(native_dir/'case-definition.json').read_bytes()
        assert json.loads((original_dir/'case-definition.json').read_text())==case,'changed frozen case definition'
    assert native.get('case','help')==case['name']
    assert native['original_receipt_sha256'] == sha(original_dir/'receipt.json'), 'native inputs do not bind this original trial'
    for directory, record in ((original_dir, original), (native_dir, native)):
        assert (directory / 'configuration.json').read_bytes() == config_raw, 'stale/wrong trial snapshot'
        assert record['configuration_sha256'] == sha(config_path)
        if directory==native_dir or not native.get('reused_original_capture'):
            assert record['native_sha256'] == sha(BINARY / 'build/native/CVPACK'), 'native binary changed'
            assert record['rebuilt_sha256'] == sha(BINARY / 'build/CVPACK.EXE'), 'Win32 binary changed'
        if 'compiled_sources_sha256' in record:
            assert record['compiled_sources_sha256']==sha(directory/'compiled-sources.json')
            from provenance import verify
            verify(directory/'compiled-sources.json')
        checked_files(directory, record)
    assert original['unmasked'] is False, 'unmasked reference is not acceptance evidence'
    assert original['original_sha256'] == sha(target)
    assert original['actual_loaded_crt']['sha256'] == CRT_SHA and original['actual_loaded_crt']['bytes'] == 277776
    provider = Path(original['actual_loaded_crt']['capture_path'])
    assert provider.stat().st_size == 277776 and sha(provider) == CRT_SHA, 'actual loaded-provider bytes differ'
    assert sha(Path(original['actual_target_capture'])) == original['masked_image_sha256'], 'actual child image differs'
    if config.get('observe_widen_factory') and case['name']=='hello-debug-dll':
        actual_provider=original['actual_loaded_mspdb']
        assert actual_provider['sha256']==config['observe_widen_factory']['provider_sha256']
        assert sha(Path(actual_provider['path']))==config['observe_widen_factory']['provider_sha256'],'actual loaded MSPDB provider bytes differ'
    assert original['plan_sha256'] == sha(original_dir / 'runtime-plan')
    historical_audit = json.loads((original_dir/'audit.json').read_text())
    assert original['audit_sha256'] == sha(original_dir/'audit.json')
    assert historical_audit['config_sha256'] == sha(config_path)
    assert historical_audit['graph_config_sha256'] == sha(original_dir/'graph-config.json')
    refreshed_audit=audit_path is not None and audit_path.resolve()!=(original_dir/'audit.json').resolve()
    audit_path=audit_path or original_dir/'audit.json'
    audit = json.loads(audit_path.read_text())
    assert audit['config_sha256'] == sha(config_path)
    assert audit['candidates'] == original['candidates'], 'audit is not for the candidate trial'
    expected_checked = [r['name'] for r in config['functions']
                        if r['status']=='accepted' or r['name'] in original['candidates']]
    assert audit['checked_functions'] == expected_checked
    assert audit['graph']['checked_functions'] == len(expected_checked), 'candidate graph missing'
    import importlib.util
    spec=importlib.util.spec_from_file_location('release_tail',BINARY/'scripts/release-tail.py')
    release=importlib.util.module_from_spec(spec);spec.loader.exec_module(release)
    release.verify_projection(audit,audit_path.parent)
    if audit.get('external_release_tail'):
        assert release.prove(config)==audit['external_release_tail']['proof'], 'forged Release ABI residual'
    for key,path in [('original_sha256',target),('rebuilt_sha256',BINARY/'build/CVPACK.EXE'),
                     ('native_sha256',BINARY/'build/native/CVPACK'),('map_sha256',BINARY/'build/cvpack.map'),
                     ('graph_tool_sha256',Path(os.environ.get('GRAPH_AUDIT_BIN',str(ROOT/'tooling/graph-audit/target/release/graph-audit'))).resolve()),
                     ('runtime_sha256',BINARY/'runtime.json'),('data_sha256',BINARY/'data.json'),
                     ('audit_wrapper_sha256',BINARY/'scripts/audit.py'),
                     ('resource_evidence_sha256',BINARY/'cases/help/resources.json'),
                     ('graph_config_sha256',audit_path.parent/'graph-config.json'),
                     ('graph_runtime_sha256',audit_path.parent/'graph-runtime.json')]:
        assert audit[key] == sha(path), ('stale candidate audit input',key)
    if 'graph_metadata_sha256' in original:
        for key,name in [('graph_metadata_sha256','graph-metadata.json'),('graph_metadata_proof_sha256','graph-metadata-proof.json')]:
            assert original[key]==sha(original_dir/name)==historical_audit[key], 'changed historical metadata'
    if 'graph_metadata_sha256' in audit:
        for key,name in [('graph_metadata_sha256','graph-metadata.json'),('graph_metadata_proof_sha256','graph-metadata-proof.json')]:
            assert audit[key]==sha(audit_path.parent/name), 'stale current metadata'
        assert audit['switch_data_validator_sha256']==sha(BINARY/'scripts/switch-data.py')
    if refreshed_audit:
        expected_binding=dict(original_receipt_sha256=sha(original_dir/'receipt.json'),
            native_receipt_sha256=sha(native_dir/'receipt.json'),
            historical_audit_sha256=sha(original_dir/'audit.json'),
            historical_metadata_sha256=sha(original_dir/'graph-metadata.json') if (original_dir/'graph-metadata.json').exists() else None,
            historical_metadata_proof_sha256=sha(original_dir/'graph-metadata-proof.json') if (original_dir/'graph-metadata-proof.json').exists() else None,
            original_events_sha256=sha(original_dir/'events'),masked_image_sha256=original['masked_image_sha256'])
        assert audit.get('reused_capture_binding')==expected_binding, 'stale/swapped reused-capture sidecar'
        changed_build=(original['native_sha256']!=sha(BINARY/'build/native/CVPACK') or
                       original['rebuilt_sha256']!=sha(BINARY/'build/CVPACK.EXE'))
        assert not changed_build or native.get('reused_original_capture'), 'changed build requires explicitly reused original capture'
        assert {'--check-imports','--resolve-register-imports'}<=set(audit['graph_command']), 'refresh omitted qualified import checks'
        # Rebuild exact current COFF/data/import metadata rather than trusting
        # a sidecar that merely repeats its own changed hashes.
        import importlib.util
        import tempfile
        spec=importlib.util.spec_from_file_location('current_metadata',BINARY/'scripts/switch-data.py')
        current_metadata=importlib.util.module_from_spec(spec);spec.loader.exec_module(current_metadata)
        checked=[r for r in config['functions'] if r['name'] in expected_checked]
        with tempfile.TemporaryDirectory(prefix='cvpack-current-metadata-',dir=Path.home()/'.ai-tmp') as temporary:
            actual,proof=current_metadata.prepare(checked,Path(temporary),config['functions'])
            assert actual.read_bytes()==(audit_path.parent/'graph-metadata.json').read_bytes(), 'forged current metadata'
            assert proof.read_bytes()==(audit_path.parent/'graph-metadata-proof.json').read_bytes(), 'forged current data proof'
        assert current_metadata.check_import_sites(audit['graph'])==audit['cached_import_sites'], 'forged cached callsite report'
        inputs=audit['graph']['register_import_inputs']
        assert inputs['original_sha256']==sha(target) and inputs['rebuilt_sha256']==sha(BINARY/'build/CVPACK.EXE'), 'stale common import report PE inputs'
        import hashlib
        current_schema=json.loads((audit_path.parent/'graph-metadata.json').read_text())
        # Rust reports the compact, sorted serde_json value, not file whitespace.
        assert inputs['metadata_sha256']==hashlib.sha256(json.dumps(current_schema,sort_keys=True,separators=(',',':')).encode()).hexdigest()
        assert inputs['preservation_contracts']==current_schema['register_imports']['preservation_contracts']
        assert {'known_imports','register_imports'}<=set(audit['graph']['checks'])
    else:
        assert not native.get('reused_original_capture'), 'changed native build requires a fresh strict audit sidecar'
    assert original['graph_config_sha256'] == sha(original_dir/'graph-config.json')
    assert original['graph_runtime_sha256'] == sha(original_dir/'graph-runtime.json')
    assert (original_dir/'graph-config.json').read_bytes()==(audit_path.parent/'graph-config.json').read_bytes(), 'audit refresh changed trial projection'
    assert (original_dir/'graph-runtime.json').read_bytes()==(audit_path.parent/'graph-runtime.json').read_bytes(), 'audit refresh changed runtime projection'
    assert original['runtime_sha256']==sha(original_dir/'runtime.json')==sha(BINARY/'runtime.json')
    assert len(set(original['candidates'])) == len(original['candidates'])
    for candidate in original['candidates']:
        assert any(r['name'] == candidate and r['status'] == 'candidate' and r['extent']
                   for r in config['functions']), 'unknown/non-candidate trial function'
    import hashlib
    image = mask(config, PE(target), original['candidates'])
    for line in (original_dir / 'runtime-plan').read_text().splitlines():
        start, _, _ = [int(value, 16) for value in line.split()]
        image[PE(target).offset(start)] = 0xcc
    assert hashlib.sha256(image).hexdigest() == original['masked_image_sha256'], 'wrong original candidate mask'
    events = (original_dir / 'events').read_text()
    frontier = re.findall(r'^frontier ([0-9a-f]{8})$', events, re.M)
    native_entries=(native_dir/'entry-trace').read_text().splitlines()
    original_entries=[int(value,16) for value in re.findall(r'^entry ([0-9a-f]{8})$',events,re.M)]
    if native.get('prepared_widen'):
        dependency=native['prepared_widen']
        assert dependency['provider_sha256']==sha(native_dir/'widen.rwp')
        assert dependency['contract_sha256']==sha(BINARY/'cases/hello-debug-dll/widen-contract.json')
        assert dependency['external_module_sha256']=='cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33'
        assert dependency['input_dll_sha256']==case['inputs'][0]['sha256'] and dependency['instances']==21
        traversed=any(0x410290<=address<0x41031e for address in original_entries)
        provider=original.get('actual_loaded_mspdb')
        assert provider or not traversed,'dependency traversed without actual original module-file proof'
        if provider:
            assert provider['sha256']==dependency['external_module_sha256']==sha(Path(provider['path'])),'actual loaded MSPDB bytes differ'
    expected_entries=[r['native_symbol'] for address in original_entries for r in config['functions'] if r['original']==address]
    native_status_text=(native_dir/'native-status').read_text()
    assert re.fullmatch(r'-?\d+\n',native_status_text), 'missing actual native status'
    native_status=int(native_status_text)
    for candidate in original['candidates']:
        record=next(r for r in config['functions'] if r['name']==candidate)
        assert record['original'] in original_entries, 'candidate never entered original'
        assert record['native_symbol'] in native_entries, 'candidate never entered native'
    for name in case['artifacts']:
        assert (original_dir/('artifact-'+name)).read_bytes()==(native_dir/('artifact-'+name)).read_bytes(),'different partial artifact'
    if not frontier:
        exits=re.findall(r'^exit (\d+)$',events,re.M)
        assert len(exits)==1 and not re.search(r'^exception |^debug-timeout',events,re.M), 'missing real original completion'
        status=int(exits[0])
        expected=master(case)
        assert status==native_status==expected['status'], ('actual completion statuses differ',exits,native_status)
        assert json.loads((native_dir/'state.json').read_text())==dict(terminal='exit',exit_status=status)
        assert native_entries==expected_entries, 'completion application trace differs'
        if case['name']=='help':assert sha(BINARY/expected['stdout'])=='f0188b3a0350b59bac8eea2dff386dd4897b1518426e167109712fcbc5f52b1c'
        for stream in ('stdout','stderr'):
            assert (original_dir/stream).read_bytes()==(native_dir/stream).read_bytes()==(BINARY/expected[stream]).read_bytes()
        for item in expected.get('artifacts',[]):
            assert sha(BINARY/item['source'])==item['sha256']
            assert sha(native_dir/('artifact-'+item['name']))==item['sha256'],'completed artifact differs from master'
        return dict(terminal='exit',exit_status=status,application_trace=expected_entries,
                    stdout_bytes=(native_dir/'stdout').stat().st_size,stderr_bytes=(native_dir/'stderr').stat().st_size)
    native_raw = (native_dir / 'gdb-stdout').read_text()
    assert len(frontier) == 1, 'need one real original frontier'
    assert 'Program received signal SIGTRAP' in native_raw, 'missing raw native trap'
    frames = re.findall(r'^0x[0-9a-f]+ in ([A-Za-z_][A-Za-z_0-9]*) \(', native_raw, re.M)
    assert len(frames) == 1 and frames[0], 'missing raw native frontier'
    state = json.loads((native_dir / 'state.json').read_text())
    assert state['function'] == frames[0], 'native state does not match raw GDB frame'
    address = int(frontier[0], 16)
    assert native_status==-5, 'missing actual native trap status'
    if address == config['target']['entry']:
        assert frames[0] == 'main' and config.get('native_entry_guard', True), 'wrong initial CRT guard'
        logical = 'runtime:mainCRTStartup'
    else:
        matches = [r for r in config['functions'] if r['original'] == address and r['native_symbol'] == frames[0]]
        assert len(matches) == 1, ('different logical frontier', frontier, frames)
        logical = matches[0]['name']
        if frames[0] not in expected_entries:expected_entries.append(frames[0])
        assert native_entries == expected_entries, ('application first-entry traces differ',expected_entries,native_entries)
        assert (original_dir/'data.json').read_bytes() == (native_dir/'data.json').read_bytes() == (BINARY/'data.json').read_bytes(), 'stale data schema'
        assert original['data_sha256'] == native['data_sha256'] == sha(BINARY/'data.json')
        import importlib.util
        spec = importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
        decoder = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(decoder)
        typed = decoder.compare(config,json.loads((original_dir/'data.json').read_text()),events,state,
            {name:(original_dir/('artifact-'+name)).read_bytes() for name in case['artifacts']},
            dict(original={p.name:p.read_bytes() for p in original_dir.glob('events-view-*.bin')},
                 native={p.name:p.read_bytes() for p in native_dir.glob('native-view-*.bin')}))
    for name in ('stdout', 'stderr'):
        assert (original_dir / name).read_bytes() == (native_dir / name).read_bytes(), ('different partial output', name)
    return dict(terminal='trap', frontier=logical, original_address=address, native_function=frames[0],
                typed_state=typed if address != config['target']['entry'] else 'initial CRT entry only')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--config', type=Path, default=BINARY / 'functions.json')
    parser.add_argument('--candidate')
    parser.add_argument('--audit',type=Path)
    parser.add_argument('--check-only', action='store_true')
    args = parser.parse_args()
    if args.candidate:
        config=json.loads(args.config.read_text())
        receipt=json.loads((args.original/'receipt.json').read_text())
        assert receipt['candidates']==[args.candidate], 'requested candidate differs from original trial'
        assert any(r['name']==args.candidate and r['status']=='candidate' for r in config['functions'])
    result = validate_pair(args.config, args.original, args.native,args.audit)
    if not args.check_only:
        assert args.candidate, 'promotion requires an explicit single candidate'
        assert args.config.resolve()==(BINARY/'functions.json').resolve(), 'only promote the current primary trial'
        review=json.loads((BINARY/'cases/help/review.json').read_text())
        assert review['first_main_promotion']=='approved'
        if not config['accepted']:assert args.candidate==review['first_function']
        else:assert review['autonomous_help_protocol']=='approved after first main promotion'
        if receipt.get('case','help')!='help':
            plan=json.loads((BINARY/'cases/plan.json').read_text())
            assert receipt['case'] in plan['autonomous_cases'],'case not released'
        record=next(r for r in config['functions'] if r['name']==args.candidate)
        proof=(args.original.parent/'promotion.json').resolve()
        promoted=dict(schema=1,candidate=args.candidate,extent=record['extent'],
            trial_config_sha256=sha(args.config),original_receipt_sha256=sha(args.original/'receipt.json'),
            native_receipt_sha256=sha(args.native/'receipt.json'),
            audit_sha256=sha(args.audit or args.original/'audit.json'),result=result)
        if proof.exists():
            assert json.loads(proof.read_text())==promoted, 'different promotion already recorded'
        else:
            proof.write_text(json.dumps(promoted,indent=2)+'\n')
        record['status']='accepted'
        record['acceptance_proof']=str(proof.relative_to(BINARY))
        assert args.candidate not in config['accepted']
        config['accepted'].append(args.candidate)
        args.config.write_text(json.dumps(config,indent=2)+'\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
