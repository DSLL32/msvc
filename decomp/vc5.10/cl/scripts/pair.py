#!/usr/bin/env python3
"""Explicit fresh original/native CRT trial; prepare evidence, never accept it."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
from accept import identity,trial_config
from audit import run as audit
from state import project
from build_inputs import verify_native_build
import original

PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
SCRATCH=Path('/root/.ai-tmp/msvc2/vc5.10/cl')
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()


def binding(path):return {'path':str(path),'sha256':sha(path)}


def run(name,label,reuse_original=None):
    assert name.startswith('_'),'explicit indexed candidate required'
    base=json.loads((PROJECT/'functions.json').read_text());trial=trial_config(base,name)
    work=SCRATCH/label
    assert not work.exists(),'fresh trial label required'
    work.mkdir(parents=True);config=work/'trial.json';config.write_text(json.dumps(trial,indent=2)+'\n')
    report=work/'audit.json';assert audit(config,report)==0
    graph=json.loads(report.with_suffix('.receipt.json').read_text())
    if reuse_original is None:
        observed,receipt=original.capture(label+'-original',config)
    else:
        observed=Path(reuse_original).resolve()
        receipt=json.loads((observed/'capture.json').read_text())
        assert json.loads((observed/'config.json').read_text())==trial,'reused original has different trial'
    projection=project('original',observed)
    if reuse_original is None:
        (observed/'state.json').write_text(json.dumps(projection,indent=2)+'\n')
    else:assert json.loads((observed/'state.json').read_text())==projection
    spec=importlib.util.spec_from_file_location('native_main_capture',PROJECT/'scripts/native-main.py')
    native=importlib.util.module_from_spec(spec);spec.loader.exec_module(native)
    native_dir=SCRATCH/(label+'-native');native.capture(observed,native_dir)
    native_state=project('native',native_dir)
    assert projection['fields']==native_state['fields'],'original/native state differs; both raw captures retained'
    assert base==json.loads((PROJECT/'functions.json').read_text()),'accepted index changed during trial'
    expected={'candidate':name,'target_sha256':receipt['target_sha256'],
        'native_sha256':sha(PROJECT/'build/native-trace/CL.EXE'),'rebuilt_sha256':sha(PROJECT/'build/cl.exe'),
        'trial_mask_sha256':receipt['original_mask_sha256']}
    result={**expected,'base_identity':identity(base),'trial_identity':identity(trial),'graph_audit':graph,
        'sources':{str(p.relative_to(PROJECT)):sha(p)for p in sorted((PROJECT/'src').glob('*'))if p.is_file()},
        'native_build':binding(native_dir/'native-build.json')}
    verify_native_build(json.loads((native_dir/'native-build.json').read_text()),expected['native_sha256'],PROJECT)
    if reuse_original is not None:
        result['original_reuse']={'capture':binding(observed/'capture.json'),
            'reason':'Native-only rebuild/recapture after shared-source update; original target, candidate, mask and raw state unchanged.'}
    for platform,directory in (('original',observed),('native',native_dir)):
        if platform=='original' and reuse_original is not None and (directory/'observation.json').exists():
            result[platform]=binding(directory/'observation.json')
            continue
        files={role:binding(directory/name)for role,name in [('stdout','stdout.bin'),('stderr','stderr.bin'),('state','state.json')]}
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
            'config_snapshot':binding(directory/'config.json'),'case':'help','arguments':['/?'],'files':files}
        if platform=='native':record['native_build']=binding(directory/'native-build.json')
        elif reuse_original is not None:
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
    args=parser.parse_args();run(args.candidate,args.label,args.reuse_original)
