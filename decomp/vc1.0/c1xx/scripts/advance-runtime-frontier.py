"""Accept one application function plus its explicitly observed runtime chain.

Original-only captures remain separate from native fixtures. Every runtime
first-entry trap is recorded, library identity and close-call behavior are
checked, and the final application frontier receives the ordinary full oracle.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time
from case_paths import paths, CAPTURE_ROOT

BINARY=Path(__file__).resolve().parents[1]

def module(name,filename):
    spec=importlib.util.spec_from_file_location(name,BINARY/'scripts'/filename)
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',default='cpp_basic')
    parser.add_argument('--native-entry',type=Path,help='explicit GDB runtime exit-entry capture when terminal is exit')
    parser.add_argument('--candidate',required=True)
    parser.add_argument('--evidence',type=Path,required=True)
    parser.add_argument('--original',type=Path,required=True)
    parser.add_argument('--runtime-captures',type=Path,required=True)
    parser.add_argument('--build-timing',type=Path,required=True)
    args=parser.parse_args()
    started=time.perf_counter()
    path=BINARY/'functions.json';config=json.loads(path.read_text())
    case,fixture,reference,frontier_root=paths(config,args.case)
    candidate=args.candidate
    assert case['terminal']==candidate
    records=[next(r for r in config['driver'] if r['name']==candidate)]
    runtime=module('c1xx_runtime_evidence','runtime-boundary.py')
    evidence_path=args.evidence.resolve()
    proof=runtime.check(evidence_path)
    captures=[args.original]+[Path(p).parent for p in json.loads(args.runtime_captures.read_text())]
    names=[candidate]+proof['runtime_order']
    assert len(captures)==len(names)
    originals=[json.loads((p/'receipt.json').read_text()) for p in captures]
    for index,(name,receipt) in enumerate(zip(names,originals)):
        assert receipt.get('case','cpp_basic')==args.case
        assert receipt['candidate']==name and receipt['accepted_count']==len(config['accepted'])+index
        assert receipt['configuration_sha256']==sha(path) and receipt['original_sha256']==config['target']['sha256']
        assert receipt.get('pending_accepted',[])==names[:index]
        if index+1<len(names):assert receipt['terminal']==names[index+1]
    assert proof['configuration_sha256']==sha(path) and proof['original_capture_sha256']==sha(args.original/'receipt.json')
    trace=module('c1xx_names','native-traces.py')
    assert 'first_entry_projection' in originals[-1], 'final runtime capture must include accepted application entry plan'
    terminal=originals[-1]['terminal']
    complete=terminal.startswith('exit:')
    if complete:
        assert terminal=='exit:0' and args.native_entry and proof['kind']=='exit'
        native_directory=args.native_entry
        native=json.loads((native_directory/'checkpoint.json').read_text())
        assert native['signal']==5 and native['function']=='recon_exit' and native['exit_code']==0
        events=(captures[0]/'events.txt').read_text()
    else:
        native_directory=Path(tempfile.mkdtemp(prefix='native-frontier-',dir=CAPTURE_ROOT))
        native=module('c1xx_native_capture','capture-native.py').capture(native_directory,case_name=args.case)
        assert native['signal']==5 and native['function']==trace.native_name(terminal),(terminal,native['function'])
        events=(captures[-1]/'events.txt').read_text()
    buffers=[m[1] for m in re.findall(r'^buffer \d+ (\d+) ([0-9a-f]*)$',events,re.M)]
    states=[int(s,16) for s in re.findall(r'^state \d+ ([0-9a-f]+)$',events,re.M)]
    assert buffers==native['buffers'] and states==native['states'],'downstream buffers/state differ'
    for stream in ('stdout.bin','stderr.bin'):
        assert (captures[-1]/stream).read_bytes()==(native_directory/stream).read_bytes(),stream
    expected={p.name:p.read_bytes() for p in (captures[-1]/'case').iterdir() if p.is_file()}
    actual={p.name:p.read_bytes() for p in (native_directory/'case').iterdir() if p.is_file()}
    assert expected==actual,'downstream visible files differ'
    audit=module('c1xx_graph','audit-graph.py')
    graph=audit.verify_candidate(candidate)
    left=set(graph['report']['functions'][candidate]['edges'])
    relative=lambda p:str(p.relative_to(BINARY))
    base_record=None
    for index,(name,capture,receipt) in enumerate(zip(names,captures,originals)):
        number=len(config['accepted'])
        slug=re.sub(r'[^A-Za-z_0-9-]','-',name.lstrip('_@'))
        record=frontier_root/('%04d-'%number+slug)
        record.mkdir(parents=True,exist_ok=True)
        if base_record is None:base_record=record
        for filename in ('receipt.json','events.txt','stdout.bin','stderr.bin','configuration.json'):
            shutil.copyfile(capture/filename,record/('original-'+filename))
        (record/'outputs').mkdir(exist_ok=True)
        for p in (capture/'case').iterdir():
            if p.is_file():shutil.copyfile(p,record/'outputs'/p.name)
        extension=dict(schema=1,accepted_before=number,accepted=name,terminal=receipt['terminal'],
            original_capture=relative(record/'original-receipt.json'),original_capture_sha256=sha(record/'original-receipt.json'),
            original_events=relative(record/'original-events.txt'),original_events_sha256=sha(record/'original-events.txt'),
            original_configuration=relative(record/'original-configuration.json'),
            scope='Explicit runtime first-entry sequence, indexed library/import identity, original/native runtime boundary and downstream full checkpoint',
            runtime_evidence=relative(evidence_path),runtime_evidence_sha256=sha(evidence_path),
            downstream_frontier=terminal,downstream_comparison=relative(base_record/'validation.json'))
        extension_path=record/'frontier-extension.json';extension_path.write_text(json.dumps(extension,indent=2)+'\n')
        config['accepted'].append(name)
        case.setdefault('frontier_extensions',[]).append(relative(extension_path))
        if index:
            (record/'validation.json').write_text(json.dumps(dict(status='accepted runtime library/import identity',function=name,
                next_frontier=receipt['terminal'],boundary_evidence=relative(evidence_path),downstream_comparison=relative(base_record/'validation.json')),indent=2)+'\n')
    for filename in ('checkpoint.json','gdb.log','timing.json','stdout.bin','stderr.bin'):
        shutil.copyfile(native_directory/filename,base_record/('downstream-native-'+filename))
    import gzip
    (base_record/'graph-audit.json.gz').write_bytes(gzip.compress(graph['raw'],mtime=0))
    (base_record/'graph-policy.json').write_text(json.dumps(dict(tool_sha256=graph['tool_sha256'],flags=graph['flags'],scope='Shared Rust CFG, ABI, references, callbacks, ownership and extent audit'),indent=2)+'\n')
    shutil.copyfile(args.build_timing,base_record/'build-timing.json')
    case['terminal']=terminal;case['terminal_evidence']=relative(record/'original-events.txt')
    path.write_text(json.dumps(config,indent=2)+'\n')
    from original_entries import register
    register(args.case,captures[-1],base_record/'entry-reference')
    snapshot=reference/'canonical-map.txt'
    snapshot.write_text(''.join(line.rstrip()+'\n' for line in (BINARY/'build/c1xx.map').read_text().splitlines()))
    shutil.copyfile(snapshot,BINARY/'cases/canonical-map.txt')
    metadata_path=reference/'manifest.json';metadata=json.loads(metadata_path.read_text())
    metadata['terminal']=(dict(kind='exit',logical=terminal,status=0) if complete else dict(metadata['terminal'],logical=terminal))
    metadata.update(reference_receipt=relative(record/'original-receipt.json'),reference_receipt_sha256=sha(record/'original-receipt.json'),
        canonical_map_sha256=sha(snapshot),capture_seconds=sum(r['capture_seconds'] for r in originals),
        scope='%d accepted frontiers; explicit runtime boundary and downstream %s frontend terminal'%(len(config['accepted']),'completed' if complete else 'partial'))
    for item in metadata['outputs']:
        p=reference/item['path'];p.write_bytes(expected[item['path']]);item.update(bytes=p.stat().st_size,sha256=sha(p))
    metadata_path.write_text(json.dumps(metadata,indent=2)+'\n')
    subprocess.run([sys.executable,str(BINARY/'scripts/native-traces.py'),'--write'],cwd=BINARY,check=True)
    checked=time.perf_counter()
    fixture=subprocess.run([sys.executable,str(BINARY/'scripts/check-native.py'),'--trace'],cwd=BINARY,text=True,capture_output=True)
    (base_record/'native-fixture.log').write_text(fixture.stdout+fixture.stderr)
    assert fixture.returncode==0,fixture.stdout+fixture.stderr
    shutil.copyfile(Path(re.search(r'receipt: (.+)',fixture.stdout)[1]),base_record/'native-fixture-receipt.json')
    report=dict(status='accepted',function=records[0],runtime_acceptance=names[1:],distinct_direct_callees=sorted(left),
        next_frontier=terminal,pending_buffers=len(buffers),checkpoint_fields=len(states),
        runtime_boundary=relative(evidence_path),fixture_seconds=time.perf_counter()-checked,
        total_operation_seconds=time.perf_counter()-started,original_capture_seconds=sum(r['capture_seconds'] for r in originals),
        boundary_original_seconds=proof['original_seconds'],boundary_native_seconds=proof['native_seconds'])
    (base_record/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    cache=json.loads((BINARY/'cases/native-traces.json').read_text());current_path=BINARY/'validation.json';current=json.loads(current_path.read_text())
    application=len(cache['accepted_application']);coverage=json.loads(subprocess.check_output([sys.executable,str(BINARY/'scripts/coverage.py')],cwd=BINARY,text=True))
    current.update(accepted_frontiers=len(config['accepted']),driver_functions=application,reconstructed_source_functions=application+current['older_crt_functions'],
        current_frontier=terminal,original_frontier_va=None if complete else hex(next(r['start'] for r in config['driver'] if r['name']==terminal)),
        current_native_application_entries=len(cache['cases'][args.case]['expected']),latest_verification=relative(base_record/'validation.json'),coverage=coverage,accepted_extent_coverage=coverage)
    current['graceful_pause'].update(paused=False,current_frontier=terminal,original_va=current['original_frontier_va'])
    if complete:
        current.update(status=args.case+' case complete; binary partial',complete_frontend_case=True)
    current['active_case']=args.case
    current.setdefault('cases',{})[args.case]=dict(status='complete' if complete else 'partial',frontier=terminal,latest_verification=relative(base_record/'validation.json'))
    current['next_operation']='Continue '+current['current_frontier']+' reconstruction'
    current_path.write_text(json.dumps(current,indent=2)+'\n')
    from status_documentation import refresh
    refresh()
    print(' -> '.join(names+[terminal]));print(base_record/'validation.json')

if __name__=='__main__':main()
