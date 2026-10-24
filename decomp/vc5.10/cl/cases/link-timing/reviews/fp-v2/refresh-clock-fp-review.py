#!/usr/bin/env python3
"""Derive an explicit stronger FP validator sidecar without rerunning either image."""
import argparse,copy,hashlib,json
from pathlib import Path
from accept import checked_file
from state import project

sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
binding=lambda p:{'path':str(Path(p).resolve()),'sha256':sha(p)}
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('pair',type=Path);parser.add_argument('output',type=Path);args=parser.parse_args()
assert not args.output.exists();args.output.mkdir()
receipt=json.loads(args.pair.read_bytes());assert receipt['case']=='link-timing'and receipt['candidate']=='_start_timer'
new=copy.deepcopy(receipt);captures={}
for platform in('original','native'):
    observation=checked_file(receipt[platform]);record=json.loads(observation.read_bytes())
    for item in record['files'].values():checked_file(item)
    old_path=checked_file(record['files']['state']);directory=old_path.parent
    old=json.loads(old_path.read_bytes());current=project(platform,directory)
    before=copy.deepcopy(old['fields']);after=copy.deepcopy(current['fields'])
    assert 'FP_scope'not in before['clock_response']
    scope=after['clock_response'].pop('FP_scope');assert scope['selector_pair']=={'original':[0x23,0x2b],'native':[0,0]}
    assert before==after,'validator refresh must not change prior execution-state interpretation'
    payload=(json.dumps(current,indent=2)+'\n').encode();digest=hashlib.sha256(payload).hexdigest()
    target=directory/('state-fp-v2-'+digest[:16]+'.json');assert not target.exists();target.write_bytes(payload)
    record['files']['state']=binding(target)
    record['validator_refresh']={'prior_observation':binding(observation),'prior_state':binding(old_path),
        'reason':'Read-only cross-image FP strengthening and exact inert selector qualification; raw execution/config/binaries unchanged.'}
    out=args.output/(platform+'-observation.json');out.write_text(json.dumps(record,indent=2)+'\n');new[platform]=binding(out)
    capture=directory/'capture.json';captures[platform]=binding(capture)
for name in('clock_state.py','clock_provider.py','state.py'):
    source=Path(__file__).parent/name;target=args.output/name;target.write_bytes(source.read_bytes())
new['validator_refresh']={'prior_pair':binding(args.pair),'execution_captures':captures,
    'validators':{name:binding(args.output/name)for name in('clock_state.py','clock_provider.py','state.py')},
    'reason':'No execution rerun; stronger architectural FP/actual inert-selector checks over retained raw. Historical pair/observations/state unchanged.'}
destination=args.output/'pair.json';destination.write_text(json.dumps(new,indent=2)+'\n');print(destination)
