#!/usr/bin/env python3
"""Project policy adapter around the shared Rust CFG engine (no Python CFG)."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time
from protocol import HERE, ROOT, candidate, sha, normalize_configuration
from importlib.util import spec_from_file_location,module_from_spec

ENGINE=Path(os.environ.get('GRAPH_AUDIT_BIN',str(ROOT/'tooling/graph-audit/target/release/graph-audit')))
def prepare(config,out):
    config=normalize_configuration(config)
    trial=config.get('trial_function')
    accepted=config['accepted']+([trial] if trial else [])
    assert len(accepted)==len(set(accepted))
    driver=[]
    for f in config['functions']:
        symbol='_'+f['name']+'@'+str(f['stack_bytes']) if f['abi']=='stdcall' else '_'+f['name']
        driver.append(dict(name=f['name'],symbol=symbol,start=int(f['start'],16)+0x71c20000,end=int(f['end'],16)+0x71c20000,shared=[(int(a,16)+0x71c20000,int(b,16)+0x71c20000)for a,b in f.get('text_data',[])]))
    assert set(accepted)<=set(f['name']for f in driver),'unknown accepted/trial function'
    imported=[f['name']for f in config.get('support_imports',[])if f['accepted']]
    result=dict(target=dict(path='toolchains/5.10/bin/RCDLL.DLL',sha256=sha(ROOT/'toolchains/5.10/bin/RCDLL.DLL')),accepted=accepted+imported,driver=driver,crt_defaults={})
    (out/'graph-input.json').write_text(json.dumps(result,indent=2)+'\n')
    original_runtime=[];rebuilt_runtime=[]
    if config.get('support_compiler'):
        spec=spec_from_file_location('compiler_support',HERE/'scripts/check-compiler-support.py');module=module_from_spec(spec);spec.loader.exec_module(module)
        support=module.verify();rebuilt=support['rebuilt']
        original_runtime=[dict(name='__chkstk',start=0x71c32df0,end=0x71c32e1f,evidence=dict(abi='custom-eax-esp',provider=support['member_sha256']))]
        rebuilt_runtime=[dict(name='__chkstk',start=int(rebuilt['address'],16),end=int(rebuilt['end'],16),evidence=dict(abi='custom-eax-esp',provider=support['member_sha256']))]
        (out/'compiler-support-identity.json').write_text(json.dumps(support,indent=2)+'\n')
    (out/'original-runtime.json').write_text(json.dumps(original_runtime,indent=2)+'\n')
    (out/'rebuilt-runtime.json').write_text(json.dumps(rebuilt_runtime,indent=2)+'\n')
    metadata=dict(schema=1,functions={},callback_tables=[])
    if any(f['name']=='rc_apply_option'and not f.get('stub')for f in config['functions']):
        spec=spec_from_file_location('dispatcher_switch',HERE/'scripts/check-dispatcher-switch.py');module=module_from_spec(spec);spec.loader.exec_module(module)
        switch=module.verify()
        metadata['functions']['rc_apply_option']=dict(rebuilt={key:switch[key]for key in ['text_data','switches']})
        (out/'dispatcher-switch.json').write_text(json.dumps(switch,indent=2)+'\n')
    if any(f['name']=='rc_copy_macro_text'and not f.get('stub')for f in config['functions']):
        spec=spec_from_file_location('copy_seh',HERE/'scripts/check-copy-seh.py');module=module_from_spec(spec);spec.loader.exec_module(module)
        seh=module.verify()
        metadata['functions']['rc_copy_macro_text']={side:dict(roots=seh[side]['roots'])for side in ['original','rebuilt']}
        (out/'copy-seh.json').write_text(json.dumps(seh,indent=2)+'\n')
    (out/'graph-metadata.json').write_text(json.dumps(metadata,indent=2)+'\n')
    return accepted

def run(config_path=None,out=None):
    config_path=config_path or HERE/'functions.json';out=out or HERE/'build'
    out.mkdir(parents=True,exist_ok=True)
    accepted=prepare(json.loads(config_path.read_text()),out)
    command=[str(ENGINE),'--profile','link','--repo',str(ROOT),'--project',str(HERE),'--original',str(ROOT/'toolchains/5.10/bin/RCDLL.DLL'),'--rebuilt',str(HERE/'build/RCDLL.DLL'),'--map',str(HERE/'build/rcdll.map'),'--config',str(out/'graph-input.json'),'--runtime',str(out/'original-runtime.json'),'--rebuilt-runtime',str(out/'rebuilt-runtime.json'),'--metadata',str(out/'graph-metadata.json'),'--discovery','cfg','--check-ret','--check-references','--check-ownership','--check-extents','--check-accepted-extents']
    started=time.perf_counter();p=subprocess.run(command,capture_output=True,text=True);elapsed=time.perf_counter()-started
    (out/'graph-audit.stdout').write_text(p.stdout);(out/'graph-audit.stderr').write_text(p.stderr)
    receipt=dict(engine_sha256=sha(ENGINE),command=command,status=p.returncode,elapsed_seconds=elapsed,functions=accepted,input_sha256=sha(config_path),candidate=candidate())
    receipt['audit_inputs']={name:sha(out/name)for name in ['graph-input.json','original-runtime.json','rebuilt-runtime.json','graph-metadata.json','compiler-support-identity.json','dispatcher-switch.json','copy-seh.json']if (out/name).is_file()}
    if not p.returncode:
        report=json.loads(p.stdout)
        assert report['checked_functions']==len(accepted),'prefix-only audit omitted trial'
        receipt['report']=report
    (out/'graph-audit-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    return receipt
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--config',type=Path);parser.add_argument('--output',type=Path)
    args=parser.parse_args();result=run(args.config,args.output)
    if result['status']:
        print((args.output or HERE/'build').joinpath('graph-audit.stderr').read_text(),end='')
    else:print(json.dumps(dict(checked_functions=result['report']['checked_functions'],elapsed_seconds=result['elapsed_seconds'])))
    raise SystemExit(result['status'])
