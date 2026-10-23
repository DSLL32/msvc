#!/usr/bin/env python3
"""Target source/mask checks plus the shared Rust CFG engine."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import re
import shutil
import importlib.util
from binary import PE
from mask import BINARY,ROOT,load,build,sha
from runtime import identify
from inputs import fingerprints
from crt_layout import verify as verify_crt

def check_source(config):
    extents=[]
    pe=PE(ROOT/config['target']['path'])
    for f in config['functions']:
        assert f['start']<f['end']
        extents.append((f['start'],f['end'],f['name']))
        cleanup=f['cleanup']
        tail=b'\xc3' if cleanup==0 else b'\xc2'+cleanup.to_bytes(2,'little')
        for address in f.get('ret_sites',[f['end']-len(tail)]):
            assert f['start']<=address and address+len(tail)<=f['end']
            assert pe.read(address,len(tail))==tail,('declared ABI/return differs',f['name'])
        if cleanup:assert f['symbol'].endswith('@'+str(cleanup))
        text=(BINARY/f['source']).read_text()
        assert '<recon.h>' in text
        if f['status']=='stub':
            pattern=r'\b'+re.escape(f['name'])+r'\s*\([^)]*\)\s*\{\s*__asm\s*\{\s*int\s+3\s*\}'
            assert re.search(pattern,text),('missing named INT3 stub',f['name'])
        else:
            assert f['status'] in ('candidate','accepted')
            assert '__asm' not in text,'INT3 candidate/accepted implementation'
    for left,right in zip(sorted(extents),sorted(extents)[1:]):
        assert left[1]<=right[0],('application ownership overlap',left,right)
    verify_crt(config.get('crt_data',[]))
    runtime=identify()
    for a,b,name in extents:
        assert not any(a<r['end'] and r['start']<b for r in runtime['functions']+runtime['data']),('runtime/data overlap',name)
    return runtime

def projection(config):
    accepted=[f['symbol'] for f in config['functions'] if f['status'] in ('accepted','candidate')]
    driver=[dict(name=f['symbol'],start=f['start'],end=f['end'],shared=f.get('text_data',[])) for f in config['functions']]
    cfg=dict(target=config['target'],accepted=accepted,driver=driver)
    metadata=dict(schema=1,functions={f['symbol']:{'original':{'roots':f.get('roots',[]),'text_data':f.get('text_data',[]),'switches':f.get('switches',{})},'rebuilt':{}} for f in config['functions']})
    return cfg,metadata

def import_residual(config):
    if not any(f['name']=='main' and f['status'] in ('accepted','candidate') for f in config['functions']):return None
    spec=importlib.util.spec_from_file_location('cvtres_import_residual',BINARY/'scripts/import-residual.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    original=PE(ROOT/config['target']['path']);rebuilt=PE(BINARY/'build/CVTRES.EXE');maptext=(BINARY/'build/cvtres.map').read_text()
    result=dict(main=module.check(original,rebuilt,(BINARY/'build/main.obj').read_bytes(),maptext))
    if any(f['name']=='usage' and f['status'] in ('accepted','candidate') for f in config['functions']):result['usage']=module.check_usage(original,rebuilt,(BINARY/'build/usage.obj').read_bytes(),maptext)
    return result

def audit_paths(config,out):
    paths=dict(tool=ROOT/'tooling/graph-audit/target/release/graph-audit',
        original=ROOT/config['target']['path'],rebuilt=BINARY/'build/CVTRES.EXE',
        native=BINARY/'build/native/CVTRES',map=BINARY/'build/cvtres.map',
        config=out/'graph-config.json',metadata=out/'metadata.json',
        runtime=out/'runtime.json',rebuilt_runtime=out/'runtime.json',trial=out/'trial.json')
    if any(f['name']=='main' and f['status'] in ('accepted','candidate') for f in config['functions']):
        paths.update(import_checker=BINARY/'scripts/import-residual.py',main_object=BINARY/'build/main.obj',import_residual=out/'import-residual.json')
    if any(f['name']=='usage' and f['status'] in ('accepted','candidate') for f in config['functions']):paths['usage_object']=BINARY/'build/usage.obj'
    if any(f['name']=='load_message' and f['status'] in ('accepted','candidate') for f in config['functions']):
        paths.update(strict_import_config=out/'strict-import-config.json',resource_source=BINARY/'src/messages.rc',resource_cache=BINARY/'cases/help/resources/messages.res',resource_proof=BINARY/'cases/help/resources/manifest.json',resource_encoder=ROOT/'lib/recon/resources.py',resource_adapter=BINARY/'scripts/prepare-resources.py')
    return paths

def audit_command(config,out):
    paths=audit_paths(config,out)
    return [str(paths['tool']),'--profile','c2','--project',str(BINARY),'--repo',str(ROOT),'--discovery','cfg',
       '--original',str(paths['original']),'--rebuilt',str(paths['rebuilt']),
       '--map',str(paths['map']),'--config',str(paths['config']),
       '--runtime',str(paths['runtime']),'--rebuilt-runtime',str(paths['rebuilt_runtime']),
       '--metadata',str(paths['metadata']),'--check-ret','--check-references','--check-ownership','--check-extents','--check-accepted-extents']

def strict_import_command(config,out):
    if not any(f['name']=='load_message' and f['status'] in ('accepted','candidate') for f in config['functions']):return None
    command=audit_command(config,out)
    command[command.index('--config')+1]=str(out/'strict-import-config.json')
    return command+['--check-imports']

def audit_hashes(config,out):
    return {name:dict(path=str(path),sha256=sha(path.read_bytes())) for name,path in audit_paths(config,out).items()}

def graph(config,out):
    import time
    start=time.perf_counter()
    out.mkdir(parents=True,exist_ok=False)
    cfg,metadata=projection(config)
    (out/'trial.json').write_text(json.dumps(config,indent=2)+'\n')
    (out/'graph-config.json').write_text(json.dumps(cfg,indent=2)+'\n')
    (out/'metadata.json').write_text(json.dumps(metadata,indent=2)+'\n')
    (out/'runtime.json').write_text('[]\n')
    shutil.copyfile(BINARY/'build/native/inputs.json',out/'native-build.json')
    shutil.copyfile(BINARY/'build/original-inputs.json',out/'original-build.json')
    residual=import_residual(config)
    if residual is not None:(out/'import-residual.json').write_text(json.dumps(residual,indent=2)+'\n')
    strict_command=strict_import_command(config,out)
    if strict_command:
        strict_cfg=dict(cfg,accepted=['_load_message@4']);(out/'strict-import-config.json').write_text(json.dumps(strict_cfg,indent=2)+'\n')
    command=audit_command(config,out)
    before=audit_hashes(config,out);sources=fingerprints();engine=time.perf_counter()
    r=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    strict=None
    if strict_command:
        strict=subprocess.run(strict_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        (out/'strict-import.stdout').write_bytes(strict.stdout);(out/'strict-import.stderr').write_bytes(strict.stderr)
        assert strict.returncode==0,strict.stderr.decode()
        strict=json.loads(strict.stdout);assert strict['checked_functions']==1
    engine_seconds=time.perf_counter()-engine
    after=audit_hashes(config,out)
    assert before==after and sources==fingerprints(),'audit inputs changed during execution'
    (out/'graph.stdout').write_bytes(r.stdout);(out/'graph.stderr').write_bytes(r.stderr)
    assert r.returncode==0,r.stderr.decode()
    result=json.loads(r.stdout)
    assert result['checked_functions']==len(cfg['accepted']),('candidate graph omitted',len(cfg['accepted']),result)
    return dict(command=command,report=result,strict_import_command=strict_command,strict_import_report=strict,trial_sha256=sha(json.dumps(config,sort_keys=True).encode()),
         execution_inputs_before=before,execution_inputs_after=after,
         target_sha256=before['original']['sha256'],rebuilt_sha256=before['rebuilt']['sha256'],
         native_sha256=before['native']['sha256'],native_build_snapshot_sha256=sha((out/'native-build.json').read_bytes()),original_build_snapshot_sha256=sha((out/'original-build.json').read_bytes()),source_inputs_at_audit=sources,map_sha256=before['map']['sha256'],
         graph_config_sha256=before['config']['sha256'],metadata_sha256=before['metadata']['sha256'],
         seconds=dict(engine=engine_seconds,total=time.perf_counter()-start))

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--trial',type=Path);a=p.parse_args()
    config=json.loads(a.trial.read_text()) if a.trial else load()[0]
    runtime=check_source(config)
    result=graph(config,a.output.resolve())
    result['runtime_identity_count']=len(runtime['functions'])
    (a.output/'receipt.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result['report'],indent=2))
