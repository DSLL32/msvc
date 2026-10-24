"""Focused allowlist negatives from the genuinely observed browser temporary."""
from pathlib import Path
import copy
import hashlib
import json
import sys
import time
BINARY=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(BINARY/'scripts'))
from checkpoint_files import declared_outputs
started=time.perf_counter()
config=json.loads((BINARY/'functions.json').read_text())
case=config['cases']['member_registry_browser']
phase=json.loads((BINARY.parent/'fixtures/c1xx-member-registry-browser/manifest.json').read_text())['phases']['c1']
checks=[]
for name in ('control','stale-proof','wrong-name','duplicate-output','directory-escape','missing-proof'):
    proposed=copy.deepcopy(case)
    item=proposed['transient_outputs'][0]
    if name=='stale-proof':item['evidence_sha256']='0'*64
    elif name=='wrong-name':item['output_name']='invented.tmp'
    elif name=='duplicate-output':proposed['transient_outputs'].append(copy.deepcopy(item))
    elif name=='directory-escape':item['output_name']='../registry.sbt'
    elif name=='missing-proof':item.pop('evidence')
    try: names=declared_outputs(phase,proposed)
    except (AssertionError,KeyError) as error:
        assert name!='control',str(error)
        checks.append(dict(test=name,result='held',reason=str(error)))
    else:
        assert name=='control' and len(names)==7 and names[-1]=='registry.sbt'
        checks.append(dict(test=name,result='verified',outputs=names))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
report=dict(schema=1,scope=__doc__,checks=checks,checker_sha256=sha(Path(__file__)),helper_sha256=sha(BINARY/'scripts/checkpoint_files.py'),discovery_sha256=sha(Path(__file__).with_name('transient-discovery.json')),configuration_sha256=sha(BINARY/'functions.json'),elapsed_seconds=time.perf_counter()-started,original_invocations=0)
Path(__file__).with_name('transient-negatives.json').write_text(json.dumps(report,indent=2)+'\n')
print(Path(__file__).with_name('transient-negatives.json'))
