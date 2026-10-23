#!/usr/bin/env python3
"""Host-only guards for captured decoder states; no executable is launched."""
import copy,json,time
from pathlib import Path
from decoder_state import compare
start=time.perf_counter()
progress=json.loads(Path('cases/disassembler-x86-monitor/progress.json').read_text())
positive=[]
for record in progress['accepted']:
    receipt=json.loads(Path(record['frontier_receipt']).read_text())
    if receipt.get('decoder_state'):
        sample=receipt['decoder_state']
        original=Path(record['original_events'])
        observations=[json.loads(line) for line in original.read_text().splitlines()
                      if json.loads(line)['event']=='decoder_state']
        compare(observations,sample)
        positive.append(record['name'])
assert positive, 'Required original-backed state inventory is empty'
rejected=[]
def deny(label,observations,actual):
    try:compare(observations,actual)
    except AssertionError:rejected.append(label)
    else:raise AssertionError('Invalid decoder observation accepted: '+label)
for field in range(56):
    changed=copy.deepcopy(sample);changed['words'][field]^=1
    deny('word%d'%field,[sample],changed)
for field in ('operand','cursor'):
    changed=copy.deepcopy(sample);changed[field]^=1
    deny(field,[sample],changed)
deny('both-empty',[],{})
deny('original-missing',[],sample)
deny('native-missing',[sample],{})
deny('original-duplicate',[sample,sample],sample)
print(json.dumps(dict(status='pass',original_backed_samples=len(positive),normalized='54scalar words, ModRM relative to instruction bytes (or NULL sentinel), byte-pointer non-NULL, operand and cursor',rejected=rejected,seconds=time.perf_counter()-start),indent=2))
