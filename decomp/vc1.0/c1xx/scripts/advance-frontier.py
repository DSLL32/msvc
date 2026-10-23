"""Accept one explicitly reconstructed frontend function from fresh evidence.

Reconstruction/reference operation only: never imported or run by fixtures.
Requires a completed canonical/native build and a reviewed complete extent.
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
import time
from case_paths import paths

BINARY=Path(__file__).resolve().parents[1]


def module(name, filename):
    spec=importlib.util.spec_from_file_location(name,BINARY/'scripts'/filename)
    result=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',default='cpp_basic')
    parser.add_argument('--candidate',required=True)
    parser.add_argument('--original',type=Path)
    parser.add_argument('--native',type=Path)
    parser.add_argument('--build-timing',type=Path)
    args=parser.parse_args()
    started=time.perf_counter()
    config_path=BINARY/'functions.json'
    config=json.loads(config_path.read_text())
    case,fixture,reference,frontier_root=paths(config,args.case)
    assert args.candidate==case['terminal'] and args.candidate not in config['accepted']
    candidate=next(r for r in config['driver'] if r['name']==args.candidate)
    assert candidate['end']>candidate['start']+1,'complete original extent required'
    if args.original is None:
        captured=subprocess.check_output([sys.executable,str(BINARY/'scripts/capture-original.py'),'--candidate',args.candidate,'--case',args.case],cwd=BINARY,text=True)
        args.original=Path(captured.strip().splitlines()[-1]).parent
    original=json.loads((args.original/'receipt.json').read_text())
    assert original['configuration_sha256']==sha(config_path)
    assert original.get('case','cpp_basic')==args.case
    assert original['candidate']==args.candidate and original['accepted_count']==len(config['accepted'])
    trace=module('c1xx_frontier_names','native-traces.py')
    if args.native is None:
        import tempfile
        args.native=Path(tempfile.mkdtemp(prefix='native-frontier-',dir=Path.home()/'.ai-tmp/vc1.0-c1xx'))
        module('c1xx_native_capture','capture-native.py').capture(args.native,case_name=args.case)
    native=json.loads((args.native/'checkpoint.json').read_text())
    assert native['signal']==5,'native did not stop on INT3'
    assert native['function']==trace.native_name(original['terminal']),(original['terminal'],native['function'])
    events=(args.original/'events.txt').read_text()
    buffers=[m[1] for m in re.findall(r'^buffer \d+ (\d+) ([0-9a-f]*)$',events,re.M)]
    states=[int(s,16) for s in re.findall(r'^state \d+ ([0-9a-f]+)$',events,re.M)]
    assert buffers==native['buffers'],'pending output buffers differ'
    assert states==native['states'],'checkpoint state differs'
    for stream in ('stdout.bin','stderr.bin'):
        assert (args.original/stream).read_bytes()==(args.native/stream).read_bytes(),stream+' differs'
    expected={p.name:p.read_bytes() for p in (args.original/'case').iterdir() if p.is_file()}
    actual={p.name:p.read_bytes() for p in (args.native/'case').iterdir() if p.is_file()}
    assert expected==actual,'visible input/output files differ'
    audit=module('c1xx_candidate_graph','audit-graph.py')
    rebuilt=next(r for r in audit.ri if r['name']==args.candidate)
    left=audit.edges(audit.orig,audit.oi,candidate)
    right=audit.edges(audit.rec,audit.ri,rebuilt)
    assert left==right,(left-right,right-left)
    number=len(config['accepted'])
    name=re.sub(r'[^A-Za-z_0-9-]','-',args.candidate.lstrip('_@'))
    record=frontier_root/('%04d-'%number+name)
    record.mkdir(parents=True)
    for directory,label,names in [(args.original,'original',['receipt.json','events.txt','stdout.bin','stderr.bin','configuration.json']),
                                 (args.native,'native',['checkpoint.json','gdb.log','timing.json','stdout.bin','stderr.bin'])]:
        for filename in names:
            path=directory/filename
            if path.exists():shutil.copyfile(path,record/(label+'-'+filename))
    if args.build_timing:
        shutil.copyfile(args.build_timing,record/'build-timing.json')
    (record/'outputs').mkdir()
    for filename,data in expected.items():(record/'outputs'/filename).write_bytes(data)
    relative=lambda p:str(p.relative_to(BINARY))
    extension=dict(schema=1,accepted_before=number,accepted=args.candidate,terminal=original['terminal'],
        original_capture=relative(record/'original-receipt.json'),original_capture_sha256=sha(record/'original-receipt.json'),
        original_events=relative(record/'original-events.txt'),original_events_sha256=sha(record/'original-events.txt'),
        original_configuration=relative(record/'original-configuration.json'),
        scope='Fresh masked original and native checkpoint: exact buffers/state/files/streams; canonical call graph verified')
    extension_path=record/'frontier-extension.json'
    extension_path.write_text(json.dumps(extension,indent=2)+'\n')
    config['accepted'].append(args.candidate)
    case['terminal']=original['terminal']
    case['terminal_evidence']=relative(record/'original-events.txt')
    case.setdefault('frontier_extensions',[]).append(relative(extension_path))
    config_path.write_text(json.dumps(config,indent=2)+'\n')
    snapshot=reference/'canonical-map.txt'
    snapshot.write_text(''.join(line.rstrip()+'\n' for line in (BINARY/'build/c1xx.map').read_text().splitlines()))
    shutil.copyfile(snapshot,BINARY/'cases/canonical-map.txt')
    reference_path=reference/'manifest.json'
    metadata=json.loads(reference_path.read_text())
    metadata['terminal']['logical']=original['terminal']
    metadata['reference_receipt']=relative(record/'original-receipt.json')
    metadata['reference_receipt_sha256']=sha(record/'original-receipt.json')
    metadata['canonical_map_sha256']=sha(snapshot)
    metadata['capture_seconds']=original['capture_seconds']
    metadata['scope']='%d accepted frontiers; explicit original-backed current partial frontend terminal'%len(config['accepted'])
    for output in metadata['outputs']:
        path=reference/output['path']
        path.write_bytes(expected[output['path']])
        output.update(bytes=path.stat().st_size,sha256=sha(path))
    reference_path.write_text(json.dumps(metadata,indent=2)+'\n')
    subprocess.run([sys.executable,str(BINARY/'scripts/native-traces.py'),'--write'],cwd=BINARY,check=True)
    checked=time.perf_counter()
    fixture=subprocess.run([sys.executable,str(BINARY/'scripts/check-native.py'),'--trace'],cwd=BINARY,text=True,capture_output=True)
    (record/'native-fixture.log').write_text(fixture.stdout+fixture.stderr)
    assert fixture.returncode==0,fixture.stdout+fixture.stderr
    fixture_receipt=Path(re.search(r'receipt: (.+)',fixture.stdout)[1])
    shutil.copyfile(fixture_receipt,record/'native-fixture-receipt.json')
    report=dict(status='accepted',function=candidate,distinct_direct_callees=sorted(left),next_frontier=original['terminal'],
        pending_buffers=len(buffers),checkpoint_fields=len(states),fixture_seconds=time.perf_counter()-checked,
        total_operation_seconds=time.perf_counter()-started,original_capture_seconds=original['capture_seconds'])
    (record/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    cache=json.loads((BINARY/'cases/native-traces.json').read_text())
    application_count=len(cache['accepted_application'])
    current_path=BINARY/'validation.json'
    current=json.loads(current_path.read_text())
    current.update(active_case=args.case,status=args.case+' reconstruction in progress',complete_frontend_case=False,accepted_frontiers=len(config['accepted']),driver_functions=application_count,
        reconstructed_source_functions=application_count+current['older_crt_functions'],
        current_frontier=original['terminal'],
        original_frontier_va=hex(next(r['start'] for r in config['driver'] if r['name']==original['terminal'])),
        current_native_application_entries=len(cache['cases'][args.case]['expected']),
        latest_verification=relative(record/'validation.json'))
    current.setdefault('cases',{})[args.case]=dict(status='partial',frontier=original['terminal'],latest_verification=relative(record/'validation.json'))
    current['graceful_pause'].update(paused=False, current_frontier=original['terminal'],
        original_va=current['original_frontier_va'])
    coverage=json.loads(subprocess.check_output([sys.executable,str(BINARY/'scripts/coverage.py')],cwd=BINARY,text=True))
    current['coverage']=coverage
    current['accepted_extent_coverage']=coverage
    current_path.write_text(json.dumps(current,indent=2)+'\n')
    readme_path=BINARY/'README.txt'
    readme=readme_path.read_text()
    readme=re.sub(r'PARTIAL (?:binary )?reconstruction:\s*\d+ accepted frontiers, including\s*\d+ frontend',
        'PARTIAL binary reconstruction: %d accepted frontiers, including %d frontend'%(len(config['accepted']),application_count),readme,count=1)
    readme=re.sub(r'trap is _[A-Za-z_0-9@]+ at original VA 0x[0-9a-f]+\.',
        'trap is %s at original VA %s.'%(original['terminal'],current['original_frontier_va']),readme,count=1)
    readme=re.sub(r'all \d+ application definitions','all %d application definitions'%len(cache['application_definitions']),readme,count=1)
    readme=re.sub(r'case expects \d+ first\nentries: \d+ accepted frontend functions plus [A-Za-z_0-9]+',
        'case expects %d first\nentries: %d accepted frontend functions plus %s'%(len(cache['cases'][args.case]['expected']),application_count,trace.native_name(original['terminal'])),readme,count=1)
    readme_path.write_text(readme)
    print(args.candidate+' -> '+original['terminal'])
    print(record/'validation.json')


if __name__=='__main__':main()
