#!/usr/bin/env python3
"""Explicitly accept one candidate using immutable trial and raw paired evidence."""
import argparse
import json
from pathlib import Path
from mask import BINARY,load,sha
from evidence import bind,compare_frontier,original_raw,compare_candidate_entry,compare_message_observations
from audit import check_source,projection,audit_hashes,audit_command,import_residual,strict_import_command,strict_import_functions
from inputs import fingerprints

def verify(config,function,original,native,audit,frozen=False):
    candidates=[f['name'] for f in config['functions'] if f['status']=='candidate']
    assert candidates==[function],'one explicit current candidate is required'
    check_source(config)
    old,new=bind(config,original,native,frozen=frozen)
    assert old['candidates']==[function] and not old['initial'],'wrong trial capture'
    candidate=next(f for f in config['functions'] if f['name']==function)
    assert candidate['start'] in original_raw(original)['entries'],'candidate was not entered in original raw trace'
    assert candidate['native'] in (native/'trace.txt').read_text().splitlines(),'candidate was not entered in native raw trace'
    entry=compare_candidate_entry(config,function,original,native)
    result=compare_frontier(config,original,native)
    result['candidate_entry']=entry
    if function=='load_message':result['message_observations']=compare_message_observations(config,original,native)
    bind_audit(config,new,audit)
    return result

def bind_audit(config,old,audit):
    audit=audit.resolve()
    report=json.loads((audit/'receipt.json').read_text())
    expected_cfg,expected_metadata=projection(config)
    assert report['execution_inputs_before']==report['execution_inputs_after']==audit_hashes(config,audit),'audit tool/PE/index/metadata/trial input changed'
    assert report['command']==audit_command(config,audit),'audit invocation differs'
    strict_command=strict_import_command(config,audit)
    assert report.get('strict_import_command')==strict_command,'strict import invocation differs'
    if strict_command:
        cfg,_=projection(config);cfg['accepted']=strict_import_functions(config)
        assert json.loads((audit/'strict-import-config.json').read_text())==cfg
        strict=json.loads((audit/'strict-import.stdout').read_text());assert strict==report['strict_import_report']
        assert strict['checked_functions']==len(cfg['accepted']) and set(strict['functions'])==set(cfg['accepted'])
        assert 'known_imports' in strict['checks']
    residual=import_residual(config,audit if (audit/'rebuilt.exe').exists() else None)
    if residual is not None:assert json.loads((audit/'import-residual.json').read_text())==residual,'bounded import residual differs'
    assert json.loads((audit/'trial.json').read_text())==config,'audit frozen trial differs'
    assert json.loads((audit/'metadata.json').read_text())==expected_metadata,'audit metadata projection differs'
    assert json.loads((audit/'runtime.json').read_text())==[],'unexpected runtime exclusion index'
    # Validator/shared worktree edits do not rewrite immutable build inputs.
    from evidence import verify_build_snapshot
    verify_build_snapshot(audit/'native-build.json',report['native_sha256'],report['native_build_snapshot_sha256'])
    verify_build_snapshot(audit/'original-build.json',report['rebuilt_sha256'],report['original_build_snapshot_sha256'])
    assert report['trial_sha256']==sha(json.dumps(config,sort_keys=True).encode()),'audit is not current trial'
    assert report['native_sha256']==old['native_sha256']
    assert report['rebuilt_sha256']==old['rebuilt_sha256']
    assert report['target_sha256']==old['original_sha256']
    assert report['map_sha256']==old['map_sha256']
    graph=json.loads((audit/'graph.stdout').read_text())
    assert report['report']==graph,'graph report/raw result differs'
    accepted=[f['symbol'] for f in config['functions'] if f['status'] in ('accepted','candidate')]
    assert graph['checked_functions']==len(accepted)
    assert set(graph['functions'])==set(accepted),'candidate graph not audited'
    cfg=json.loads((audit/'graph-config.json').read_text())
    assert cfg==expected_cfg and cfg['accepted']==accepted,'audit exact trial graph projection differs'
    assert report['graph_config_sha256']==sha((audit/'graph-config.json').read_bytes())
    assert report['metadata_sha256']==sha((audit/'metadata.json').read_bytes())


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--function',required=True);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--audit',type=Path,required=True);p.add_argument('--proof',type=Path,required=True);a=p.parse_args()
    a.proof=a.proof.resolve()
    config,_=load();result=verify(config,a.function,a.original.resolve(),a.native.resolve(),a.audit.resolve())
    proof=dict(schema=1,function=a.function,comparison=result,original=str(a.original.resolve()),native=str(a.native.resolve()),audit=str(a.audit.resolve()))
    a.proof.parent.mkdir(parents=True,exist_ok=True);a.proof.write_text(json.dumps(proof,indent=2)+'\n')
    f=next(f for f in config['functions'] if f['name']==a.function);f['status']='accepted';f['evidence']=str(a.proof.relative_to(BINARY));config['accepted'].append(f['symbol'])
    (BINARY/'functions.json').write_text(json.dumps(config,indent=2)+'\n')
    print(json.dumps(proof,indent=2))
