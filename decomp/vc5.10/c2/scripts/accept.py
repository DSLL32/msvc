#!/usr/bin/env python3
"""Accept one reviewed, actually audited trial from fresh matching raw evidence."""
import argparse
import json
from pathlib import Path
import shutil
import gzip
import tarfile
import io
from compare import compare
from state import BINARY, load, sha, trial_configuration

def preflight(name,original,native,validation=None):
    base,_,_=load()
    assert base['trial']==[name],'accept exactly the single declared trial candidate'
    trial,_=trial_configuration((BINARY/'functions.json').read_bytes())
    records=[r for r in trial['driver']if r['name']==name]
    assert len(records)==1,'source candidate needs explicit original graph vertex'
    assert name not in base['accepted'],'candidate already accepted'
    result=compare(original,native,validation)
    assert result['matched']!=name,'candidate did not advance to a new frontier'
    audit=json.loads(Path(result['graph_report']).read_text())
    expected={r['name']for r in trial['driver']if r['name']in trial['accepted']}
    assert set(audit['functions'])==expected,'trial graph candidate/prefix was not audited'
    assert name in audit['functions'],'candidate graph absent from raw Rust report'
    assert audit['checked_functions']>=len(expected)>0,'empty/prefix-only audit cannot accept candidate'
    return base,result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--function',required=True);p.add_argument('--original',required=True,type=Path);p.add_argument('--native',required=True,type=Path);p.add_argument('--proof',required=True,type=Path);p.add_argument('--validation',type=Path);p.add_argument('--dry-run',action='store_true');a=p.parse_args()
    before=(BINARY/'functions.json').read_bytes()
    base,result=preflight(a.function,a.original,a.native,a.validation)
    gate=json.loads((BINARY/'cases/review-gate.json').read_text())
    assert gate['approved']is True,'orchestrator candidate-protocol review pending'
    if a.dry_run:
        print(json.dumps(result,indent=2));return
    a.proof.mkdir(parents=True,exist_ok=False)
    original_files=['base_configuration.json','configuration.json','receipt.json','events.txt','stdout.bin','stderr.bin','graph-audit.json','audit-receipt.json','child-environment-input.bin','child-command-input.bin']
    native_files=['base_configuration.json','configuration.json','receipt.json','state.json','stdout.bin','stderr.bin','gdb.stdout','gdb.stderr','gdb-prefix.py','gdb-state.py','native-maps.txt','compiled-inputs.json','target-environment.bin']
    for side,source,files in [('original',a.original,original_files),('native',a.native,native_files)]:
        destination=a.proof/side;destination.mkdir()
        if side=='original':
            files += [name for name in ('main-argv.bin','main-environment.bin','command-argv.bin','matcher-input.bin','data-plan.txt','data-model.json','data-state.bin','wildcard-input.bin','wildcard-pattern.bin','option-value.bin','fatal-argument.bin','fatal-phase.bin','current-option.bin','message-cache.bin','format-pattern.bin','format-prefix.bin','integer-prefix.bin','copy-flags.bin','integer-input.bin','option-operand.bin','header-streams.bin','il-buffer.bin','symbol-hash.bin')if(source/name).exists()]
        if side=='original':files += [p.name for p in source.glob('section-name-*.bin')]+[p.name for p in source.glob('section-class-*.bin')]
        if side=='original':files += [p.name for p in source.glob('cache-message-*.bin')]+[p.name for p in source.glob('diagnostic-format.bin')]
        files += [p.name for p in source.glob('artifact-*.bin')]
        if side=='original':files += [p.name for p in source.glob('intermediate-*.bin')]
        if side=='original':files += [name for name in ('runtime-original.json','runtime-rebuilt.json','graph-metadata.json','pool-block.bin','pool-descriptors.bin','real-records.bin','extended-destination.bin','x87-state.bin','extended-after.bin','x87-after.bin','x87-final.bin','intrinsic-model.json','intrinsic-table.bin','section-name.bin','section-class.bin','section-globals.bin','section-node.bin','section-list.bin','unmodeled-original-data.bin','sections.bin','section-addresses.bin','symbol-scope.bin','symbol-template.bin','symbol-name.bin','source-name-pool.bin','decorated-name.bin','function-symbol.bin','filename-node.bin','initializer-buffer.bin')if(source/name).exists()]
        for name in files:shutil.copyfile(source/name,destination/name)
    def archive_inputs(directory,manifest,destination):
        with destination.open('wb')as raw,gzip.GzipFile(filename='',fileobj=raw,mode='wb',mtime=0)as compressed,tarfile.open(fileobj=compressed,mode='w')as archive:
            for name in sorted(manifest['inputs']):
                data=(directory/name).read_bytes();assert sha(data)==manifest['inputs'][name]
                info=tarfile.TarInfo(name);info.size=len(data);info.mode=0o644;info.mtime=0
                archive.addfile(info,io.BytesIO(data))
    if(a.native/'data-model.json').exists():shutil.copyfile(a.native/'data-model.json',a.proof/'native/data-model.json')
    if(a.native/'intrinsic-model.json').exists():shutil.copyfile(a.native/'intrinsic-model.json',a.proof/'native/intrinsic-model.json')
    if(a.original/'helper-inputs.json').exists():
        shutil.copyfile(a.original/'helper-inputs.json',a.proof/'original/helper-inputs.json')
        archive_inputs(a.original/'helper-inputs',json.loads((a.original/'helper-inputs.json').read_text()),a.proof/'original/helper-inputs.tar.gz')
    archive_inputs(a.native/'compiled-inputs',json.loads((a.native/'compiled-inputs.json').read_text()),a.proof/'native/compiled-inputs.tar.gz')
    validation_folder=a.proof/'current-validation';validation_folder.mkdir()
    shutil.copyfile(result['validation'],validation_folder/'receipt.json')
    shutil.copyfile(result['graph_report'],validation_folder/'graph-audit.json')
    shutil.copyfile(Path(result['graph_report']).parent/'import-residuals.json',validation_folder/'import-residuals.json')
    shutil.copyfile(Path(result['graph_report']).parent/'data-evidence.json',validation_folder/'data-evidence.json')
    shutil.copyfile(Path(result['graph_report']).parent/'win32-inputs.json',validation_folder/'win32-inputs.json')
    archive_inputs(Path(result['graph_report']).parent/'win32-inputs',json.loads((Path(result['graph_report']).parent/'win32-inputs.json').read_text()),validation_folder/'win32-inputs.tar.gz')
    for name in ('configuration.json','pending-correction.json'):
        source=Path(result['validation']).parent/name
        if source.exists(): shutil.copyfile(source,validation_folder/name)
    record={'function':a.function,'match':result,'original_capture':str(a.original.resolve()),'native_capture':str(a.native.resolve()),'base_configuration_sha256':sha(before),'review':gate}
    (a.proof/'acceptance.json').write_text(json.dumps(record,indent=2)+'\n')
    assert (BINARY/'functions.json').read_bytes()==before,'configuration changed before acceptance write'
    base['accepted'].append(a.function);base['trial']=[]
    (BINARY/'functions.json').write_text(json.dumps(base,indent=2)+'\n')
    print(json.dumps(record,indent=2))

if __name__=='__main__':main()
