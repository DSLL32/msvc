"""Narrow live scratch allocation identities, with explicit defined-byte masks."""
import json,hashlib,re
from state import BINARY

def validate(config,pe):
    record=config.get('allocation_observations')
    if not record:return None
    path=BINARY/record['evidence']
    assert hashlib.sha256(path.read_bytes()).hexdigest()==record['evidence_sha256'],'stale allocation proof'
    proof=json.loads(path.read_text())
    actual_sources={str(p.relative_to(BINARY))for p in (BINARY/'src').glob('*')if p.suffix in ('.c','.h')and p.name!='debug-run.c'}
    assert set(proof['sources'])==actual_sources,'allocation owner source inventory changed'
    for source,digest in proof['sources'].items():
        assert hashlib.sha256((BINARY/source).read_bytes()).hexdigest()==digest,'allocation owner source changed'
    owner=next(r for r in config['functions']if r['name']==proof['owner'])
    assert owner['status']!='stub'
    for allocation in proof['allocations']:
        r=allocation['original_callsite']
        assert owner['extent'][0]<=r['start']<r['end']<=owner['extent'][1],'allocation callsite outside reviewed owner'
        assert hashlib.sha256(pe.read(r['start'],r['end']-r['start'])).hexdigest()==r['sha256'],'allocation creation instructions changed'
        assert allocation['lifetime']=='creation-through-first-module-entry'
        assert allocation['defined_ranges']==[], 'unreviewed scratch definedness'
    return proof

def context(config,pe,events,state,frontier,case):
    proof=validate(config,pe)
    if not proof:return {}
    assert case==proof['allowed_case'],'unreviewed scratch case'
    assert frontier and frontier['name']in proof['allowed_frontiers'],'scratch lifetime reached an unreviewed consumer'
    assert state['frames'][0]==frontier['native']
    if proof.get('first_module_only'):
        word=int(re.search(r'^stack 1 ([0-9a-f]+)$',events,re.M)[1],16)
        assert word&0xffff==state['parameters']['module']==0,'scratch lifetime is not first module entry'
    memory={}
    for line in events.splitlines():
        if line.startswith('memory '):
            _,address,raw=line.split();memory.update((int(address,16)+i,v)for i,v in enumerate(bytes.fromhex(raw)))
    globals={r['name']:r for r in json.loads((BINARY/'data.json').read_text())}
    result={}
    for item in proof['allocations']:
        owner=globals[item['size_global']]
        count=int.from_bytes(bytes(memory[owner['original']+i]for i in range(owner['size'])),'little')
        observed=int.from_bytes(bytes.fromhex(state['globals'][owner['name']]),'little')
        assert count==observed,'scratch requested size differs'
        count*=item.get('element_size',1)
        if 'maximum'in item:count=min(count,item['maximum'])
        assert count<=65536
        result[item['name']]=dict(size=count,defined_ranges=item['defined_ranges'],original_callsite=item['original_callsite'],lifetime=item['lifetime'])
    return result

def mask_undefined(name,left,right,definitions):
    proof=definitions[name]
    assert len(left)==len(right)==proof['size'],'scratch allocation extent differs'
    assert proof['defined_ranges']==[],'unreviewed defined scratch ranges'
    # Raw bytes remain in original events/native GDB receipts. A zero comparison
    # mask here is valid solely in the reviewed pre-consumer lifetime above.
    left[:]=right[:]=b'\0'*len(left)
    return proof
