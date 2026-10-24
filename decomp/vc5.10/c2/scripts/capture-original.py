#!/usr/bin/env python3
"""Explicit original-only reference/frontier capture. Not used by fixtures."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import time
from state import BINARY, ROOT, TARGET, CANONICAL_CRT, load, masked, offset, sha, validate, source_hashes, trial_configuration, audit_input_hashes

spec=importlib.util.spec_from_file_location('c2_original_transport',BINARY/'scripts/wine-tool.py')
transport=importlib.util.module_from_spec(spec);spec.loader.exec_module(transport)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--unmasked',action='store_true')
    parser.add_argument('--candidate',action='append',default=[])
    parser.add_argument('--observe-messages',action='store_true',help='unmasked analysis only; capture actually reached message IDs/results')
    parser.add_argument('--observe-startup',action='store_true',help='unmasked analysis only; capture exact CRT IAT argument/results')
    args=parser.parse_args()
    begin=time.monotonic()
    base,original,inventory=load()
    base_snapshot=(BINARY/'functions.json').read_bytes()
    config,snapshot=trial_configuration(base_snapshot)
    validate(config,original,inventory)
    from case_inputs import manifest,materialize,artifacts
    case_directory,case_value,target_environment=manifest(config)
    child_cwd=config['case'].get('child_cwd','S:\\vc5.10\\c2\\original')
    assert child_cwd.startswith('S:\\')
    work=transport.SCRATCH/child_cwd[3:].replace('\\','/')
    input_names=materialize(case_directory,case_value,work)
    assert args.candidate==config['trial'],'candidate must be explicit in immutable trial configuration'
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    (output/'base_configuration.json').write_bytes(base_snapshot)
    if config.get('writer_callback_boundary'):
        content=(BINARY/config['writer_callback_boundary']).read_bytes()
        assert sha(content)==config['writer_callback_boundary_sha256']
        (output/'writer-callback-boundary.json').write_bytes(content)
    if config.get('output_callback_boundary'):
        content=(BINARY/config['output_callback_boundary']).read_bytes()
        assert sha(content)==config['output_callback_boundary_sha256']
        (output/'output-callback-boundary.json').write_bytes(content)
    if config.get('directive_callback_contract'):
        content=(BINARY/config['directive_callback_contract']).read_bytes()
        assert sha(content)==config['directive_callback_contract_sha256']
        (output/'directive-callback-contract.json').write_bytes(content)
    (output/'configuration.json').write_bytes(snapshot)
    if config.get('intrinsic_model'):
        data=(BINARY/config['intrinsic_model']).read_bytes();assert sha(data)==config['intrinsic_model_sha256']
        (output/'intrinsic-model.json').write_bytes(data)
    from data_evidence import model,plan
    data_model=model(config)
    if data_model:
        (output/'data-model.json').write_bytes((BINARY/config['data_model']).read_bytes())
        (output/'data-plan.txt').write_text(''.join('%08x %d\n'%(address,size)for name,address,size in plan(data_model)))
    from audit import run as run_audit
    audit=run_audit(output/'configuration.json',output/'graph-audit.json')
    (output/'audit-receipt.json').write_text(json.dumps(audit,indent=2)+'\n')
    shutil.copyfile(BINARY/'build/c2.exe',output/'rebuilt.exe')
    shutil.copyfile(BINARY/'build/c2.map',output/'rebuilt.map')
    assert sha((output/'rebuilt.exe').read_bytes())==audit['rebuilt_sha256']
    assert sha((output/'rebuilt.map').read_bytes())==audit['rebuilt_map_sha256']
    static_snapshots={}
    for source,name in [('runtime.json','runtime-original.json'),('build/runtime.json','runtime-rebuilt.json'),('build/graph-metadata.json','graph-metadata.json')]:
        shutil.copyfile(BINARY/source,output/name)
        static_snapshots[name]={'input':source,'sha256':sha((output/name).read_bytes())}
    image=original if args.unmasked else masked(config,original,inventory,args.candidate)
    entries=[]
    if args.observe_messages:
        assert args.unmasked and not args.candidate,'message observations are unmasked analysis only'
        image=bytearray(image)
        for point in (0x48b798,0x48b79f,0x48b7a5):
            p=offset(inventory,point);entries.append((point,image[p]));image[p]=0xcc
        image=bytes(image)
    if args.observe_startup:
        assert args.unmasked and not args.candidate,'startup observations are unmasked analysis only'
        for point in (0x452a18,0x452a1e,0x4527c6,0x4527cc,0x4527e3,0x4527e9,0x4527f1,0x4527f7,0x45280c,0x45281e,0x452855,0x45285b,0x452870,0x452876):
            p=offset(inventory,point);entries.append((point,image[p]))
        image=bytearray(image)
        for point,_ in entries:image[offset(inventory,point)]=0xcc
        image=bytes(image)
    if not args.unmasked:
        image=bytearray(image)
        for r in config['driver']+config['support']:
            if r['name'] in config['accepted']+args.candidate:
                p=offset(inventory,r['start']);entries.append((r['start'],image[p]));image[p]=0xcc
        image=bytes(image)
    if config.get('directive_callback_contract')and not args.unmasked:
        image=bytearray(image);point=0x453df4;p=offset(inventory,point)
        assert image[p]==original[p]==0xff,'ABI observer must belong to restored real dispatcher code'
        entries.append((point,image[p]));image[p]=0xcc;image=bytes(image)
    (output/'C2.EXE').write_bytes(image)
    for name in ('MSVCRT.DLL','MSPDB50.DLL'):
        shutil.copyfile(ROOT/'toolchains/5.10/bin'/name,output/name)
    assert sha((output/'MSVCRT.DLL').read_bytes())==CANONICAL_CRT
    (output/'plan.txt').write_text(''.join('%x %x\n'%r for r in entries))
    child_environment=b'\0'.join(v.encode()for v in config['case']['child_environment_input'])+b'\0\0'
    (output/'requested-environment.bin').write_bytes(child_environment)
    helper_inputs=json.loads((BINARY/'build/helper/input-provenance.json').read_text())
    assert helper_inputs['binary_sha256']==sha((BINARY/'build/debug-run.exe').read_bytes())
    shutil.copyfile(BINARY/'build/debug-run.exe',output/'capture-helper.exe')
    shutil.copyfile(BINARY/'build/helper/input-provenance.json',output/'helper-inputs.json')
    shutil.copytree(helper_inputs['snapshot'],output/'helper-inputs')
    arguments=config['case']['arguments'];assert arguments in ([],['/?'])
    command=['/usr/bin/wine',transport.windows(output/'capture-helper.exe'),
             transport.windows(output/'C2.EXE'),case_value['canonical_argv'][0],' '.join(arguments),transport.windows(output/'plan.txt'),transport.windows(output/'requested-environment.bin'),child_cwd]
    setup=time.monotonic()-begin;run=time.monotonic()
    result=subprocess.run(command,cwd=output,env=transport.environment(),capture_output=True,timeout=45)
    elapsed=time.monotonic()-run
    (output/'host.stdout').write_bytes(result.stdout);(output/'host.stderr').write_bytes(result.stderr)
    provider=output/'loaded-msvcrt.bin'
    receipt=dict(case_id=config['case']['id'],case_inputs={name:sha((work/name).read_bytes())for name in input_names},artifacts=artifacts(work,case_value,output),helper_inputs_sha256=sha((output/'helper-inputs.json').read_bytes()),data_model_sha256=sha((output/'data-model.json').read_bytes())if data_model else None,data_state_sha256=sha((output/'data-state.bin').read_bytes())if(output/'data-state.bin').exists()else None,explicit_child_environment=True,child_cwd=child_cwd,kind='explicit-original-only',observe_messages=args.observe_messages,observe_startup=args.observe_startup,unmasked=args.unmasked,candidates=args.candidate,
                 command=command,base_configuration_sha256=sha(base_snapshot),configuration_sha256=sha(snapshot),target_sha256=sha(original),
                 executable_sha256=sha(image),debugger_sha256=sha((BINARY/'build/debug-run.exe').read_bytes()),
                 rebuilt_snapshot_sha256=sha((output/'rebuilt.exe').read_bytes()),rebuilt_map_snapshot_sha256=sha((output/'rebuilt.map').read_bytes()),
                 static_input_snapshots=static_snapshots,
                 audit_receipt_sha256=sha((output/'audit-receipt.json').read_bytes()),rebuilt_sha256=sha((BINARY/'build/c2.exe').read_bytes()),
                 audit_inputs=audit_input_hashes(),source_hashes=source_hashes(),events_sha256=sha((output/'events.txt').read_bytes())if(output/'events.txt').exists()else None,
                 environment=config['case']['environment'],provider_sha256=sha(provider.read_bytes())if provider.exists()else None,
                 execution_inputs={name:sha((output/name).read_bytes())if(output/name).exists()else None for name in (('child-environment-input.bin','child-command-input.bin','data-plan.txt')if data_model else('child-environment-input.bin','child-command-input.bin'))},
                 helper_status=result.returncode,seconds={'setup':setup,'execution':elapsed,'total':time.monotonic()-begin})
    if any(r.get('state_policy')=='globals-complete'for r in config['stubs']):
        receipt['globals_complete_hashes']={name:sha((output/name).read_bytes())for name in ('unmodeled-original-data.bin','pool-block.bin','header-streams.bin')+ (('writer-pool.bin','output-file.bin')if any(r.get('frontier_contract')in ('object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input','coff-symbol-input','coff-symbols-complete','coff-directive-input')for r in config['stubs'])else())+ (('directive-pool.bin',)if any(r.get('frontier_contract')in ('output-directives-complete','object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input','coff-symbol-input','coff-symbols-complete','coff-directive-input')for r in config['stubs'])else())+ (('initializer-buffer.bin',)if any(r.get('frontier_contract')in ('initializers-complete','backend-inputs-open','output-callbacks-selected','output-directive-input','output-directives-complete','object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input','coff-symbol-input','coff-symbols-complete','coff-directive-input') for r in config['stubs'])else())+ (('backend-streams.bin',)if any(r.get('frontier_contract')in ('backend-inputs-open','output-callbacks-selected','output-directive-input','output-directives-complete','object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input','coff-symbol-input','coff-symbols-complete','coff-directive-input') for r in config['stubs'])else())+ (('coff-symbol-entries.bin',)if any(r.get('frontier_contract')in ('coff-symbols-complete','coff-directive-input')for r in config['stubs'])else())+ (('output-directive-value.bin',)if any(r.get('frontier_contract')=='output-directive-input'for r in config['stubs'])else())}
    if(output/'unmodeled-original-data.bin').exists():receipt['unmodeled_original_data']={'classification':'UNMODELED; preservation only, no payload/extent/data credit','bytes':(output/'unmodeled-original-data.bin').stat().st_size,'sha256':sha((output/'unmodeled-original-data.bin').read_bytes())}
    for stream in ('stdout','stderr'):
        path=output/(stream+'.bin')
        receipt[stream]={'bytes':path.stat().st_size,'sha256':sha(path.read_bytes())}if path.exists()else None
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    assert result.returncode==0, 'original debugger failed; preserved '+str(output)
    assert provider.exists() and receipt['provider_sha256']==CANONICAL_CRT,'canonical provider not proved; preserved '+str(output)
    assert (BINARY/'functions.json').read_bytes()==base_snapshot,'configuration changed during capture'
    print((output/'events.txt').read_text())
    print(output/'receipt.json')

if __name__=='__main__':main()
