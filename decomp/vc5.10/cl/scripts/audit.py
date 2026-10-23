#!/usr/bin/env python3
"""Run the shared Rust graph tool and bind the exact files it actually read."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time
from import_residuals import verify as verify_import_residuals

PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
TOOL=Path(os.environ.get('GRAPH_AUDIT_BIN',str(ROOT/'tooling/graph-audit/target/release/graph-audit'))).resolve()
FLAGS=['--discovery','cfg','--check-ret','--check-references','--check-ownership','--check-extents','--include-evidenced-runtime','--check-accepted-extents']
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()


def input_paths(project=PROJECT,root=ROOT,config=None):
    result={'original':root/'toolchains/5.10/bin/CL.EXE','rebuilt':project/'build/cl.exe',
        'map':project/'build/cl.map','runtime':project/'runtime.json',
        'rebuilt_runtime':project/'build/runtime.json','metadata':project/'graph-metadata.json',
        'tool':TOOL,
        'runner':project/'scripts/audit.py','config':config or project/'functions.json',
        'import_residuals':project/'cases/help/import-residuals.json','import_residual_verifier':project/'scripts/import_residuals.py'}
    if '_find_rule' in json.loads((config or project/'functions.json').read_text())['accepted']:
        result.update(native_grammar=project/'build/native-trace/CL.EXE',grammar_proof=project/'cases/help/rule-grammar.json',grammar_verifier=project/'scripts/rules.py')
    if '_classify_input' in json.loads((config or project/'functions.json').read_text())['accepted']:
        result.update(input_kind_proof=project/'cases/minimal-c/input-kinds.json',input_kind_verifier=project/'scripts/input_kinds.py')
    if '_cleanup_files' in json.loads((config or project/'functions.json').read_text())['accepted']:
        result.update(temporary_suffix_proof=project/'cases/minimal-c/temporary-suffixes.json',temporary_suffix_verifier=project/'scripts/temporary_suffixes.py')
    if '_prepare_jobs' in json.loads((config or project/'functions.json').read_text())['accepted']:
        result.update(mre_proof=project/'cases/minimal-c/mre-imports.json',mre_verifier=project/'scripts/mre_imports.py',
            mre_host=project/'scripts/mre-host.c',mre_definition=project/'scripts/mspdb50.def',mre_provider=root/'toolchains/5.10/bin/MSPDB50.DLL',mre_library=project/'build/mspdb50.lib')
    if '_find_file' in json.loads((config or project/'functions.json').read_text())['accepted']:
        result.update(compiler_dependency_proof=project/'cases/minimal-c/dependencies.json',compiler_input_verifier=project/'scripts/case_inputs.py')
    if '_execute_pass' in json.loads((config or project/'functions.json').read_text())['accepted']:
        result['gpa_preservation']=project/'cases/minimal-c/getprocaddress-preservation.json'
        result['gpa_verifier']=project/'scripts/gpa_preservation.py'
        witness=json.loads(result['gpa_preservation'].read_text())
        for key,item in witness['raw'].items():result['gpa_raw:'+key]=project/item['path']
        for key,item in witness['headers'].items():result['gpa_header:'+key]=root/item['path']
    return result


# These two bodies cache qualified IAT values in callee-saved registers. The
# optional direct-import engine cannot resolve those calls; retain exact narrow
# evidence instead of reshaping target source or silently accepting unknowns.
CACHED_IMPORT_RESIDUALS={'_main','_split_arguments','_format_message','_validate_value','_format_number','_define_macro','_set_output_option','_show_help','_append_escaped','_find_file','_quoted_length','_quote_argument'}


def import_leaves(config):
    result=set()
    for record in config.get('runtime',[]):
        if record['name'] not in config['accepted'] or not record['name'].startswith('import:'):continue
        proof=record['evidence']
        assert proof['kind']=='exact FF25 IAT import thunk' and record['end']-record['start']==6
        assert proof.get('qualified_identity') and proof.get('abi'),'import boundary lacks qualified identity/typed ABI'
        result.add(record['name'])
    return result


def pass_configurations(config):
    leaves=import_leaves(config)
    if not leaves:return {'strict':(config,FLAGS)}
    strict=copy.deepcopy(config);strict['accepted']=[name for name in config['accepted']if name not in leaves]
    direct=copy.deepcopy(config);direct['accepted']=[name for name in config['accepted']if name not in CACHED_IMPORT_RESIDUALS]
    full_flags=[flag for flag in FLAGS if flag!='--check-ret']
    direct_flags=full_flags+['--check-imports']
    if '_execute_pass' in config['accepted']:direct_flags+=['--resolve-register-imports']
    return {'full_support':(config,full_flags),'strict_ret':(strict,FLAGS),
        'qualified_imports':(direct,direct_flags)}


def run(config,report):
    inputs=input_paths(config=config)
    before={name:{'path':str(path),'sha256':sha(path)} for name,path in inputs.items()}
    config_value=json.loads(config.read_text())
    trial_identity=hashlib.sha256(json.dumps(config_value,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    report.parent.mkdir(parents=True,exist_ok=True)
    passes={};started=time.perf_counter();status=0
    configurations=pass_configurations(config_value)
    for label,(value,flags) in configurations.items():
        path=config if value==config_value else report.with_name(report.stem+'-'+label+'-config.json')
        if path!=config:path.write_text(json.dumps(value,indent=2)+'\n')
        selected=input_paths(config=path)
        bound={name:{'path':str(file),'sha256':sha(file)} for name,file in selected.items()}
        command=[str(TOOL),'--profile','cl','--repo',str(ROOT),'--project',str(PROJECT)]
        for name in ('original','rebuilt','map','runtime','rebuilt_runtime','metadata','config'):
            command+=['--'+name.replace('_','-'),str(selected[name])]
        command+=flags
        result=subprocess.run(command,cwd=PROJECT,capture_output=True)
        destination=report if label in ('strict','full_support') else report.with_name(report.stem+'-'+label+'.json')
        destination.write_bytes(result.stdout);destination.with_suffix('.stderr').write_bytes(result.stderr)
        for name,file in selected.items():assert sha(file)==bound[name]['sha256'],'audit pass input changed'
        passes[label]={'status':result.returncode,'command':command,'flags':flags,'inputs':bound,
            'config_snapshot':bound['config'],'report':{'path':str(destination),'sha256':sha(destination)}}
        if result.returncode:
            status=result.returncode
            print(result.stderr.decode(),end='',file=__import__('sys').stderr);break
        decoded=json.loads(result.stdout)
        assert decoded['discovery']=='cfg' and set(decoded['functions'])==set(value['accepted']),'audited subset differs'
        if label=='qualified_imports' and '_execute_pass' in value['accepted']:
            from gpa_preservation import verify
            verify(decoded,PROJECT,ROOT)
    elapsed=time.perf_counter()-started
    for name,path in inputs.items():assert sha(path)==before[name]['sha256'],'audit input changed during execution'
    receipt={'schema':1,'status':status,'flags':FLAGS,'mode':'verified-import-projection' if len(configurations)>1 else 'strict-single-pass',
        'trial_identity':trial_identity,'config_snapshot':before['config'],'inputs':before,'passes':passes,
        'report':{'path':str(report),'sha256':sha(report)},'seconds':elapsed}
    if len(configurations)>1:
        assert verify_import_residuals(config_value,PROJECT,ROOT)==sorted(set(config_value['accepted']) & (CACHED_IMPORT_RESIDUALS|{'_final_pass'}))
        residual=PROJECT/'cases/help/import-residuals.json'
        receipt['cached_import_residuals']={'path':str(residual),'sha256':sha(residual)}
        receipt['strict_excluded_imports']=sorted(import_leaves(config_value))
        receipt['qualification']='Exact typed FF25 import leaves have no local RET; every actual selected application/CRT body retains strict RET checks. Qualified import checks retain explicit cached-register residuals.'
    if '_find_rule' in config_value['accepted']:
        from rules import verify
        receipt['grammar']=verify(PROJECT,ROOT)
    if '_classify_input' in config_value['accepted']:
        from input_kinds import verify
        receipt['input_kinds']=verify(PROJECT,ROOT)
    if '_cleanup_files' in config_value['accepted']:
        from temporary_suffixes import verify
        receipt['temporary_suffixes']=verify(PROJECT,ROOT)
    if '_prepare_jobs' in config_value['accepted']:
        from mre_imports import verify
        receipt['mre_imports']=verify(PROJECT,ROOT)
    report.with_suffix('.receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    if status==0:print(json.dumps({'status':0,'passes':list(passes),'seconds':elapsed}))
    return status


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config',type=Path,default=PROJECT/'functions.json')
    parser.add_argument('--report',type=Path,default=PROJECT/'build/graph-audit.json')
    args=parser.parse_args();raise SystemExit(run(args.config.resolve(),args.report.resolve()))
