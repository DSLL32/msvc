"""Register an explicitly captured, matching original/native case frontier.

No reference process is run here. Captures are supplied explicitly and remain
frozen; the resulting ordinary regression executes only native cached fixtures.
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
from case_paths import paths, CAPTURE_ROOT

BINARY=Path(__file__).resolve().parents[1]


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',required=True)
    parser.add_argument('--original',type=Path,required=True)
    parser.add_argument('--native',type=Path,required=True)
    args=parser.parse_args()
    started=time.perf_counter()
    config_path=BINARY/'functions.json';config=json.loads(config_path.read_text())
    case,fixture,reference,frontiers=paths(config,args.case)
    assert not case.get('frontier_extensions') and case['accepted_from']==len(config['accepted'])
    original=json.loads((args.original/'receipt.json').read_text())
    native=json.loads((args.native/'checkpoint.json').read_text())
    assert 'first_entry_projection' in original, 'fresh original entry plan required'
    assert original['configuration_sha256']==sha(config_path)
    assert original['case']==args.case and original['candidate'] is None and original['accepted_count']==len(config['accepted'])
    assert native['signal']==5 and native['function']==re.sub(r'@\d+$','',original['terminal'][1:])
    events=(args.original/'events.txt').read_text()
    assert native['buffers']==[m[1] for m in re.findall(r'^buffer \d+ (\d+) ([0-9a-f]*)$',events,re.M)]
    assert native['states']==[int(x,16) for x in re.findall(r'^state \d+ ([0-9a-f]+)$',events,re.M)]
    expected={p.name:p.read_bytes() for p in (args.original/'case').iterdir() if p.is_file()}
    assert expected=={p.name:p.read_bytes() for p in (args.native/'case').iterdir() if p.is_file()}
    for filename in ('stdout.bin','stderr.bin'):
        assert (args.original/filename).read_bytes()==(args.native/filename).read_bytes()
    seed=BINARY/'cases'/(args.case+'-seed');seed.mkdir(exist_ok=False)
    for directory,label,names in [(args.original,'original',('receipt.json','events.txt','configuration.json','stdout.bin','stderr.bin')),
                                  (args.native,'native',('checkpoint.json','gdb.log','timing.json','stdout.bin','stderr.bin'))]:
        for filename in names:shutil.copyfile(directory/filename,seed/(label+'-'+filename))
    case.update(terminal=original['terminal'],terminal_evidence=str((seed/'original-events.txt').relative_to(BINARY)),original_seed=str(seed.relative_to(BINARY)))
    config_path.write_text(json.dumps(config,indent=2)+'\n')
    from original_entries import register
    register(args.case,args.original,seed/'entry-reference')
    reference.mkdir(exist_ok=False)
    snapshot=reference/'canonical-map.txt';snapshot.write_text(''.join(line.rstrip()+'\n' for line in (BINARY/'build/c1xx.map').read_text().splitlines()))
    shutil.copyfile(snapshot,BINARY/'cases/canonical-map.txt')
    metadata=json.loads((BINARY/'cases/native-cpp-basic/manifest.json').read_text())
    metadata.update(case=args.case,scope='explicit matching original/native initial frontier; accepted prefix seeded',
        reference_receipt=str((seed/'original-receipt.json').relative_to(BINARY)),reference_receipt_sha256=sha(seed/'original-receipt.json'),
        canonical_map_sha256=sha(snapshot),terminal=dict(kind='frontier',logical=original['terminal'],native_signal=5,original_exception='80000003'),
        expected_stdout=(args.original/'stdout.bin').read_bytes().decode('latin1'),expected_stderr=(args.original/'stderr.bin').read_bytes().decode('latin1'),
        capture_seconds=original['capture_seconds'])
    phase=json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    metadata['dependencies']=phase.get('toolchain_dependencies',[])
    metadata['outputs']=[dict(path=item['output_name']) for item in phase['outputs']]
    for output in metadata['outputs']:
        target=reference/output['path'];target.write_bytes(expected[output['path']]);output.update(bytes=target.stat().st_size,sha256=sha(target))
    (reference/'manifest.json').write_text(json.dumps(metadata,indent=2)+'\n')
    subprocess.run([sys.executable,'scripts/native-traces.py','--write'],cwd=BINARY,check=True)
    result=subprocess.run([sys.executable,'scripts/check-native.py','--trace'],cwd=BINARY,capture_output=True,text=True)
    (seed/'native-fixture.log').write_text(result.stdout+result.stderr)
    assert result.returncode==0,result.stdout+result.stderr
    receipt=Path(re.search(r'receipt: (.+)',result.stdout)[1]);shutil.copyfile(receipt,seed/'native-fixture-receipt.json')
    report=dict(status='matched case seed; no new function accepted',case=args.case,frontier=original['terminal'],accepted_prefix=len(config['accepted']),pending_buffers=len(native['buffers']),checkpoint_fields=len(native['states']),original_capture_seconds=original['capture_seconds'],total_registration_seconds=time.perf_counter()-started)
    (seed/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    current_path=BINARY/'validation.json';current=json.loads(current_path.read_text())
    current.update(status=args.case+' reconstruction in progress',active_case=args.case,complete_frontend_case=False,current_frontier=original['terminal'],original_frontier_va=hex(next(r['start'] for r in config['driver'] if r['name']==original['terminal'])),latest_verification=str((seed/'validation.json').relative_to(BINARY)))
    current.setdefault('cases',{}).update(cpp_basic=dict(status='complete',terminal='exit:0'),**{args.case:dict(status='partial',frontier=original['terminal'])})
    current['graceful_pause'].update(paused=False,current_frontier=original['terminal'],original_va=current['original_frontier_va'])
    current_path.write_text(json.dumps(current,indent=2)+'\n')
    print(report)


if __name__=='__main__':main()
