#!/usr/bin/env python3
"""Explicit fresh original/native CRT trial; prepare evidence, never accept it."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
from accept import identity,trial_config,reconstruction_capture
from audit import run as audit
from state import project
from build_inputs import verify_native_build
import original

PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
SCRATCH=Path('/root/.ai-tmp/msvc2/vc5.10/cl')
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()


def binding(path):return {'path':str(path),'sha256':sha(path)}


def run(name,label,reuse_original=None,reverify_accepted=False,case_name='help',dependency_response=False,link_response=False,clock_response=False):
    assert sum(bool(v)for v in(dependency_response,link_response,clock_response))<=1,'ambiguous child response mode'
    assert name.startswith('_'),'explicit indexed candidate required'
    index=json.loads((PROJECT/'functions.json').read_text());base=copy.deepcopy(index)
    if reverify_accepted:
        assert name in base['accepted'],'cannot reverify an unaccepted candidate'
        base['accepted'].remove(name)
    trial=trial_config(base,name)
    work=SCRATCH/label
    assert not work.exists(),'fresh trial label required'
    work.mkdir(parents=True);config=work/'trial.json';config.write_text(json.dumps(trial,indent=2)+'\n')
    report=work/'audit.json';assert audit(config,report)==0
    graph=json.loads(report.with_suffix('.receipt.json').read_text())
    if reuse_original is None:
        observed,receipt=original.capture(label+'-original',config,case_name=case_name,dependency_response=dependency_response,link_response=link_response,clock_response=clock_response)
    else:
        observed=Path(reuse_original).resolve()
        receipt=json.loads((observed/'capture.json').read_text())
        assert json.loads((observed/'config.json').read_text())==trial,'reused original has different trial'
        assert bool(receipt.get('dependency_response'))==dependency_response,'reused original response mode differs'
        assert bool(receipt.get('link_response'))==link_response,'reused original LINK mode differs'
        assert bool(receipt.get('clock_response'))==clock_response,'reused original clock mode differs'
    reconstruction_capture(receipt)
    assert receipt['case']==case_name,'reused original belongs to a different case'
    projection=project('original',observed)
    original_state=observed/'state.json'
    if reuse_original is None:
        original_state.write_text(json.dumps(projection,indent=2)+'\n')
    elif not original_state.exists() or json.loads(original_state.read_text())!=projection:
        # Additional typed fields may be decoded from already captured raw
        # bytes. Preserve every old projection and create a versioned product.
        payload=json.dumps(projection,indent=2)+'\n'
        original_state=observed/('state-derived-'+hashlib.sha256(payload.encode()).hexdigest()[:16]+'.json')
        if original_state.exists():assert original_state.read_text()==payload
        else:original_state.write_text(payload)
    spec=importlib.util.spec_from_file_location('native_main_capture',PROJECT/'scripts/native-main.py')
    native=importlib.util.module_from_spec(spec);spec.loader.exec_module(native)
    native_dir=SCRATCH/(label+'-native');native.capture(observed,native_dir)
    native_state=project('native',native_dir)
    assert projection['fields']==native_state['fields'],'original/native state differs; both raw captures retained'
    assert index==json.loads((PROJECT/'functions.json').read_text()),'accepted index changed during trial'
    expected={'candidate':name,'target_sha256':receipt['target_sha256'],
        'native_sha256':sha(PROJECT/'build/native-trace/CL.EXE'),'rebuilt_sha256':sha(PROJECT/'build/cl.exe'),
        'trial_mask_sha256':receipt['original_mask_sha256']}
    result={**expected,'case':case_name,'arguments':receipt['arguments'],'base_identity':identity(base),'trial_identity':identity(trial),'graph_audit':graph,
        'sources':{str(p.relative_to(PROJECT)):sha(p)for p in sorted((PROJECT/'src').glob('*'))if p.is_file()},
        'native_build':binding(native_dir/'native-build.json')}
    verify_native_build(json.loads((native_dir/'native-build.json').read_text()),expected['native_sha256'],PROJECT)
    if reuse_original is not None:
        result['original_reuse']={'capture':binding(observed/'capture.json'),
            'reason':'Explicit native-only retry or decoder refresh; original execution inputs, target, candidate and mask unchanged; historical raw records preserved.'}
    for platform,directory in (('original',observed),('native',native_dir)):
        if platform=='original' and reuse_original is not None and original_state.name=='state.json' and (directory/'observation.json').exists():
            result[platform]=binding(directory/'observation.json')
            continue
        files={role:binding(directory/name)for role,name in [('stdout','stdout.bin'),('stderr','stderr.bin')]}
        files['state']=binding(original_state if platform=='original' else directory/'state.json')
        files['trace']=binding(directory/'trace.txt')
        if platform=='original':
            files['entry_guards']=binding(directory/'entries.txt')
            files.update(events=binding(directory/'events.txt'),loaded_msvcrt=binding(directory/'loaded-msvcrt.bin'))
            if (directory/'loaded-target.bin').exists():files['loaded_target']=binding(directory/'loaded-target.bin')
            if (directory/'resource-events.txt').exists():
                files['resource_events']=binding(directory/'resource-events.txt')
                for file in directory.glob('resource-*-*.bin'):files['resource:'+file.name]=binding(file)
        else:
            files['debugger']=binding(directory/'debugger.txt')
            for raw in ('trace-filter.txt','trace-seen.txt','exec-wrapper.py'):
                files['raw:'+raw]=binding(directory/raw)
            if (directory/'resources.bin').exists():files['resource_provider']=binding(directory/'resources.bin')
        state=project(platform,directory)
        for raw_name in state['raw_inputs']:files['raw:'+raw_name]=binding(directory/raw_name)
        record={**expected,'platform':platform,'trial_identity':identity(trial),'candidate':name,
            'config_snapshot':binding(directory/'config.json'),'case':case_name,'arguments':receipt['arguments'],'files':files}
        if platform=='native':record['native_build']=binding(directory/'native-build.json')
        else:
            record['schema']=2
            record['capture_receipt']=binding(directory/'capture.json')
        if platform=='original' and reuse_original is not None:
            # These historic pairing fields did not participate in original
            # execution. Keep their original values in the new envelope.
            record['native_sha256']=receipt['native_sha256']
            record['rebuilt_sha256']=receipt['rebuilt_sha256']
        path=(work/'original-observation.json') if platform=='original' and reuse_original is not None else directory/'observation.json';path.write_text(json.dumps(record,indent=2)+'\n')
        result[platform]=binding(path)
    destination=work/'pair.json';destination.write_text(json.dumps(result,indent=2)+'\n')
    print('Fresh trial evidence prepared:',destination,'; accepted index unchanged')
    return destination


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate',required=True);parser.add_argument('--label',required=True)
    parser.add_argument('--reuse-original',type=Path,help='Explicit native-only retry, preserving the original raw capture and observation')
    parser.add_argument('--reverify-accepted',action='store_true',help='Prepare a review-only proof of the current accepted trial; never modify its index')
    parser.add_argument('--case',default='help')
    parser.add_argument('--dependency-response',action='store_true',help='Capture the actual C1 response during this fresh masked candidate trial')
    parser.add_argument('--link-response',action='store_true',help='Observe the actual LINK-only child in the masked candidate trial')
    parser.add_argument('--clock-response',action='store_true',help='Bind this timing candidate to its actual clock and LINK response')
    args=parser.parse_args();run(args.candidate,args.label,args.reuse_original,args.reverify_accepted,args.case,args.dependency_response,args.link_response,args.clock_response)
