#!/usr/bin/env python3
"""Accept only a newly executed/audited trial, never an old accepted-only mask."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
from mask import construct
from audit import input_paths,FLAGS,pass_configurations,import_leaves
from state import project as decode_state
from import_residuals import verify as verify_import_residuals
from build_inputs import verify_native_build

PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
CRT_SHA='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'


def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def identity(config):
    return hashlib.sha256(json.dumps(config,sort_keys=True,separators=(',',':')).encode()).hexdigest()


def trial_config(base,name):
    records={r['name']:r for r in base['driver']+base.get('runtime',[])}
    assert name in records and name not in base['accepted'],'unknown or already accepted candidate'
    assert records[name].get('evidence'),'candidate has no implementation or library identity evidence'
    trial=copy.deepcopy(base);trial['accepted'].append(name)
    runtime={r['name']:r for r in base.get('runtime',[])}
    for support in records[name].get('import_support',[]):
        assert support in runtime and support.startswith('import:'),'nonimport support promotion requested'
        proof=runtime[support]['evidence']
        assert proof['kind']=='exact FF25 IAT import thunk' and proof.get('qualified_identity') and proof.get('abi'),'unproved import support identity or ABI'
        if support not in trial['accepted']:trial['accepted'].append(support)
    return trial


def checked_file(entry):
    path=Path(entry['path'])
    assert path.is_file() and digest(path)==entry['sha256'],'raw capture file changed: '+str(path)
    return path


def reconstruction_capture(capture):
    assert capture.get('baseline') is False and not capture.get('dependency_analysis') and not capture.get('clock_analysis'), 'analysis/reference baseline cannot be used as fresh reconstruction acceptance'
    assert capture['controller_error'] is None and capture['helper_status'] in (0,capture.get('expected_status',0)&255), 'original capture did not succeed'
    if capture.get('link_response'):
        assert capture['case']=='link-object' and not capture.get('dependency_analysis') and not capture.get('dependency_complete') and not capture.get('dependency_response'), 'ambiguous link response mode'
        marker=b'CL_LINK_RESPONSE_V1\n'
        assert capture['files']['dependency-observer.txt']=={'sha256':hashlib.sha256(marker).hexdigest(),'bytes':len(marker)}, 'missing or different link response request'
        assert capture['command'][-2]=='1', 'link response original was not a masked trial'
        # Full raw child/state decoding is mandatory in observation()->state.project;
        # this guard establishes only the unambiguous fresh masked mode.
    if capture.get('dependency_response'):
        assert capture['case']=='minimal-c' and not capture.get('dependency_complete'), 'ambiguous dependency response mode'
        marker=b'CL_DEPENDENCY_RESPONSE_V1\n'
        assert capture['files']['dependency-observer.txt']=={'sha256':hashlib.sha256(marker).hexdigest(),'bytes':len(marker)}, 'missing or different actual response request'
        assert capture['command'][-2]=='1', 'response original was not launched as a masked trial'


def observation(entry,trial,expected):
    path=checked_file(entry)
    raw=json.loads(path.read_text())
    config_path=checked_file(raw['config_snapshot'])
    assert json.loads(config_path.read_text())==trial,'mutually stale or different trial snapshot'
    assert raw['trial_identity']==identity(trial),'wrong trial identity'
    assert raw['candidate']==expected['candidate'],'wrong observed candidate'
    # The original execution never consumed the paired Linux ELF. Its historic
    # native hash remains intact when an explicitly recorded native-only retry
    # reuses this original observation.
    keys=('target_sha256','trial_mask_sha256')
    if raw['platform']=='native':keys+=('native_sha256','rebuilt_sha256')
    for key in keys:
        assert raw[key]==expected[key],'capture binary snapshot differs: '+key
    from case_inputs import load
    case=load(raw['case'])
    assert raw['arguments']==case['arguments'],'unapproved case arguments'
    if 'case' in expected:assert raw['case']==expected['case'] and raw['arguments']==expected['arguments'],'pair case differs'
    files={role:checked_file(value) for role,value in raw['files'].items()}
    assert {'stdout','stderr','state'}<=files.keys(),'incomplete raw capture'
    if raw['platform']=='original':
        capture_entry=raw.get('capture_receipt',raw['files'].get('raw:capture.json'))
        assert raw.get('schema',1)<2 or capture_entry, 'original execution receipt is missing'
        if capture_entry:
            capture=json.loads(checked_file(capture_entry).read_text())
            reconstruction_capture(capture)
            if capture['case']=='link-object' and '_write_link_message'in trial['accepted']:
                assert capture.get('link_response')is True,'LINK child candidate requires its complete actual response'
            assert capture['target_sha256']==expected['target_sha256'] and capture['original_mask_sha256']==expected['trial_mask_sha256'], 'original execution receipt image differs'
            assert capture['config_sha256']==digest(config_path), 'original execution receipt trial differs'
            assert capture['case']==raw['case'] and capture['arguments']==raw['arguments'], 'original execution receipt case differs'
            assert capture.get('expected_status',0)==case.get('expected_status',0),'original case status contract differs'
        assert {'events','loaded_msvcrt'}<=files.keys()
        assert digest(files['loaded_msvcrt'])==CRT_SHA,'noncanonical actually loaded original CRT'
        if 'loaded_target' in files:assert digest(files['loaded_target'])==expected['trial_mask_sha256'],'actual loaded image differs from trial mask'
        if 'resource_events' in files:
            from resources import verify_api_capture
            verify_api_capture(files['resource_events'].parent)
        text=files['events'].read_text()
        traps=re.findall(r'^trap .*address=([0-9a-f]{8}) ',text,re.M)
        exits=re.findall(r'^exit .*status=(\d+)$',text,re.M)
        if traps:
            assert len(traps)==1,'ambiguous original frontier'
            if capture_entry:assert capture['helper_status']==0,'trap controller did not succeed'
            address=int(traps[0],16)
            matches=[r['name'] for r in trial['driver']+trial.get('runtime',[]) if r['start']==address]
            assert len(matches)==1,'unknown or ambiguous original logical frontier'
            outcome={'kind':'trap','logical_frontier':matches[0]}
        else:
            assert exits==[str(case.get('expected_status',0))],'original exit differs from declared case status'
            if capture_entry:assert capture['helper_status']==(case.get('expected_status',0)&255),'helper status differs from normal target exit'
            outcome={'kind':'completed','status':case.get('expected_status',0)}
    else:
        assert raw['platform']=='native' and 'debugger' in files
        if 'resource_provider' in files:
            from resources import provider_bytes
            assert files['resource_provider'].read_bytes()==provider_bytes(),'native resource input differs from encountered Win32 source/evidence'
        text=files['debugger'].read_text()
        symbols=re.findall(r'^NATIVE_FRONTIER_SYMBOL=(\S+)$',text,re.M)
        if symbols:
            assert len(symbols)==1 and 'SIGTRAP' in text,'native did not trap once'
            matches=[r['name'] for r in trial['driver']+trial.get('runtime',[])
                     if r.get('native_symbol',r['name'].lstrip('_'))==symbols[0]]
            assert len(matches)==1,'unknown or ambiguous native logical frontier'
            outcome={'kind':'trap','logical_frontier':matches[0]}
        else:
            assert 'NATIVE_COMPLETION_STATUS='+str(case.get('expected_status',0))+'\n'in text,'native exit differs from declared case status'
            outcome={'kind':'completed','status':case.get('expected_status',0)}
    assert 'trace' in files,'missing first-entry trace'
    trace=files['trace'].read_text().splitlines()
    allowed={r['native_symbol']:r for r in trial['driver'] if r['name'] in trial['accepted']}
    assert len(trace)==len(set(trace)) and set(trace)<=set(allowed),'unfiltered or duplicate entry trace'
    candidate=[r for r in trial['driver']if r['name']==expected['candidate']]
    if candidate:assert candidate[0]['native_symbol'] in trace,'candidate absent from actual entry trace'
    if raw['platform']=='original':
        from binary import PE
        pe=PE(ROOT/trial['target']['path'])
        expected_guards=''.join('%x %d %s\n'%(r['start']-pe.base,pe.read(r['start'],1)[0],r['native_symbol'])
            for r in trial['driver']if r['name'] in trial['accepted'])
        assert files['entry_guards'].read_text()==expected_guards,'original trace guard set differs from trial'
        entries=re.findall(r'^entry event=\d+ address=[0-9a-f]{8} name=(\S+)$',text,re.M)
        assert entries==trace,'original trace differs from raw breakpoint events'
    else:
        assert files['raw:trace-filter.txt'].read_text()==''.join(r['native_symbol']+'\n'for r in trial['driver']if r['name'] in trial['accepted']),'native trace filter differs from trial'
    state=json.loads(files['state'].read_text())
    assert state==decode_state(raw['platform'],files['state'].parent),'typed state is not decoded from current raw CTX/globals/stack/GDB inputs'
    assert state.get('projection') and isinstance(state.get('fields'),dict),'missing explicit state projection'
    assert state['fields'],'empty state comparison is not acceptance evidence'
    if case['inputs']:
        assert state['fields']['case_inputs']==case,'observation label does not match actual case inputs'
    return outcome,{'stdout':digest(files['stdout']),'stderr':digest(files['stderr'])},{'projection':state['projection'],'fields':state['fields'],'trace':trace}


def validate(base,receipt,name,project=PROJECT,root=ROOT,review_only=False):
    assert review_only or base['acceptance_reviewed'],'orchestrator scaffold review is required'
    trial=trial_config(base,name)
    assert receipt['base_identity']==identity(base),'stale base config'
    assert receipt['trial_identity']==identity(trial),'trial omits or changes candidate'
    expected={'candidate':name,'target_sha256':digest(root/base['target']['path']),
        'native_sha256':digest(project/'build/native-trace/CL.EXE'),
        'rebuilt_sha256':digest(project/'build/cl.exe'),
        'trial_mask_sha256':hashlib.sha256(construct(trial)).hexdigest()}
    if 'case' in receipt:expected.update(case=receipt['case'],arguments=receipt['arguments'])
    assert receipt['candidate']==name,'receipt candidate differs'
    for key,value in expected.items():assert receipt[key]==value,'wrong current binary or trial mask: '+key
    sources={str(p.relative_to(project)):digest(p) for p in sorted((project/'src').glob('*')) if p.is_file()}
    assert receipt['sources']==sources,'source changed since trial'
    native_build_path=checked_file(receipt['native_build'])
    verify_native_build(json.loads(native_build_path.read_text()),expected['native_sha256'],project)
    audit=receipt['graph_audit']
    audit_config=checked_file(audit['config_snapshot'])
    assert json.loads(audit_config.read_text())==trial,'audit config omits candidate or is stale'
    assert audit['trial_identity']==identity(trial),'wrong audit trial identity'
    assert audit['status']==0,'graph audit failed'
    assert audit['flags']==FLAGS,'audit omitted required graph checks'
    expected_inputs=input_paths(project,root,audit_config)
    assert set(audit['inputs'])==set(expected_inputs),'incomplete audited-input binding'
    for key,path in expected_inputs.items():
        item=audit['inputs'][key]
        assert Path(item['path']).resolve()==path.resolve(),'wrong audited input path: '+key
        assert checked_file(item)==Path(item['path']),'audited input changed: '+key
    assert audit['inputs']['rebuilt']['sha256']==expected['rebuilt_sha256'],'audit checked an older rebuilt executable'
    if audit.get('passes'):
        configurations=pass_configurations(trial)
        assert set(audit['passes'])==set(configurations),'missing strict/complete import audit pass'
        for label,(value,flags) in configurations.items():
            run=audit['passes'][label]
            path=checked_file(run['config_snapshot'])
            assert json.loads(path.read_text())==value,'audit projection changed the required subset'
            assert run['status']==0 and run['flags']==flags,'audit pass failed or omitted checks'
            paths=input_paths(project,root,path)
            assert set(run['inputs'])==set(paths),'audit projection inputs incomplete'
            for role,file in paths.items():
                assert checked_file(run['inputs'][role]).resolve()==file.resolve(),'stale audit projection input'
            decoded=json.loads(checked_file(run['report']).read_text())
            assert set(decoded['functions'])==set(value['accepted']),'audit projection function set differs'
            if label=='qualified_imports' and '_execute_pass' in value['accepted']:
                from gpa_preservation import verify
                verify(decoded,project,root)
        if len(configurations)>1:
            assert audit['strict_excluded_imports']==sorted(import_leaves(trial)),'strict RET waiver exceeds verified import leaves'
            checked_file(audit['cached_import_residuals'])
            verify_import_residuals(trial,project,root)
    else:
        assert not import_leaves(trial),'import support requires complete/strict audit projections'
    if '_find_rule' in trial['accepted']:
        from rules import verify
        assert audit['grammar']==verify(project,root),'grammar audit receipt differs from compiled data'
    if '_classify_input' in trial['accepted']:
        from input_kinds import verify
        assert audit['input_kinds']==verify(project,root),'input-kind receipt differs from compiled data'
    if '_prepare_jobs' in trial['accepted']:
        from mre_imports import verify
        assert audit['mre_imports']==verify(project,root),'external MSPDB import proof differs'
    report=json.loads(checked_file(audit['report']).read_text())
    assert report['discovery']=='cfg' and name in report['functions'],'candidate was not audited'
    # A native-only retry is valid only against this exact recorded original
    # process, mask, config and raw file set; never rewrite its old metadata.
    if receipt.get('original_reuse'):
        reuse=receipt['original_reuse']
        original_capture_path=checked_file(reuse['capture'])
        original_capture=json.loads(original_capture_path.read_text())
        assert original_capture['target_sha256']==expected['target_sha256']
        assert original_capture['original_mask_sha256']==expected['trial_mask_sha256']
        assert original_capture['config_sha256']==digest(checked_file(json.loads(checked_file(receipt['original']).read_text())['config_snapshot']))
        for filename,item in original_capture['files'].items():
            assert digest(original_capture_path.parent/filename)==item['sha256'],'reused original raw file changed'
    original=observation(receipt['original'],trial,expected)
    native_raw=json.loads(checked_file(receipt['native']).read_text())
    assert checked_file(native_raw['native_build'])==native_build_path,'native capture/build provenance differs'
    native=observation(receipt['native'],trial,expected)
    assert original==native,'frontier, streams or typed state differs'
    outcome=original[0]
    assert outcome['kind']=='completed' or outcome['logical_frontier']!=name,'candidate never executed beyond its entry trap'
    if outcome['kind']=='completed':
        reference=json.loads((project/'cases'/expected.get('case','help')/'reference/manifest.json').read_text())
        assert reference.get('status',0)==outcome['status'],'reference target status differs'
        assert original[1]==reference['streams'],'completed streams differ from canonical original'
    return trial


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('name');parser.add_argument('receipt',type=Path)
    parser.add_argument('--review-only',action='store_true',help='Validate held trial evidence without persisting acceptance')
    args=parser.parse_args()
    config=json.loads((PROJECT/'functions.json').read_text())
    receipt=json.loads(args.receipt.read_text())
    trial=validate(config,receipt,args.name,review_only=args.review_only)
    if args.review_only:
        print('Held trial evidence passes; accepted index unchanged')
    else:
        assert config['acceptance_reviewed'],'cannot persist acceptance before review'
        (PROJECT/'functions.json').write_text(json.dumps(trial,indent=2)+'\n')
        journal_path=PROJECT/'cases'/receipt.get('case','help')/'acceptance-order.json'
        journal=json.loads(journal_path.read_text()) if journal_path.exists() else {'schema':1,'case':receipt['case'],'entries':[]}
        record=next(r for r in trial['driver']+trial['runtime']if r['name']==args.name)
        journal['entries'].append({'order':len(journal['entries'])+1,'candidate':args.name,
            'class':'application' if record in trial['driver'] else 'CRT support',
            'extent_bytes':record['end']-record['start'],
            'receipt':{'path':str(args.receipt.resolve()),'sha256':digest(args.receipt)}})
        journal_path.write_text(json.dumps(journal,indent=2)+'\n')
