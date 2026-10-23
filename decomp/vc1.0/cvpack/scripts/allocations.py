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

def context(config,pe,events,state,frontier,case,native_reads=None,partial_reads=None):
    proof=validate(config,pe)
    if not proof:return {}
    assert case==proof['allowed_case'],'unreviewed scratch case'
    assert frontier and frontier['name']in proof['allowed_frontiers'],'scratch lifetime reached an unreviewed consumer'
    assert state['frames'][0]==frontier['native']
    if proof.get('first_module_only')and proof.get('first_module_mode')!='current_module_index0':
        word=int(re.search(r'^stack 1 ([0-9a-f]+)$',events,re.M)[1],16)
        assert word&0xffff==state['parameters']['module']==0,'scratch lifetime is not first module entry'
    memory={}
    for line in events.splitlines():
        if line.startswith('memory '):
            _,address,raw=line.split();memory.update((int(address,16)+i,v)for i,v in enumerate(bytes.fromhex(raw)))
    globals={r['name']:r for r in json.loads((BINARY/'data.json').read_text())}
    if proof.get('first_module_mode')=='current_module_index0':
        from objects import original_walks
        walks=original_walks(events,json.loads((BINARY/'walks.json').read_text()))
        old=int.from_bytes(bytes.fromhex(walks['module_index'][0]['bytes'])[8:12],'little')
        new=int.from_bytes(bytes.fromhex(state['walks']['module_index'][0]['bytes'])[8:12],'little')
        address=globals['current_module']['original']
        assert int.from_bytes(bytes(memory[address+i]for i in range(4)),'little')==old and state['globals']['current_module']['address']==new and old and new,'scratch lifetime is not first module'
    if proof.get('first_module_mode')=='explicit_module_index':
        from objects import original_walks
        walks=original_walks(events,json.loads((BINARY/'walks.json').read_text()))
        allowed=[r for r in proof['allowed_module_boundaries']if r['frontier']==frontier['name']]
        assert len(allowed)==1,'unreviewed scratch module boundary'
        r=allowed[0];index=r['index']*12
        def owner(raw):
            raw=bytes.fromhex(raw);assert int.from_bytes(raw[index:index+2],'little')==r['module'],'scratch module number differs'
            return int.from_bytes(raw[index+8:index+12],'little')
        old=owner(walks['module_index'][0]['bytes']);new=owner(state['walks']['module_index'][0]['bytes'])
        address=globals['current_module']['original']
        assert int.from_bytes(bytes(memory[address+i]for i in range(4)),'little')==old and state['globals']['current_module']['address']==new and old and new,'scratch current module identity differs'
    result={}
    for item in proof['allocations']:
        owner=globals[item['size_global']]
        count=int.from_bytes(bytes(memory[owner['original']+i]for i in range(owner['size'])),'little')
        observed=int.from_bytes(bytes.fromhex(state['globals'][owner['name']]),'little')
        assert count==observed,'scratch requested size differs'
        count*=item.get('element_size',1)
        if 'maximum'in item:count=min(count,item['maximum'])
        assert count<=65536
        from reads import original as original_reads
        if item['name']=='global_symbol_block0':
            old_address=walks['global_symbol_blocks'][0]['arrays']['0']['address']
            new_address=state['walks']['global_symbol_blocks'][0]['arrays']['0']['address']
        else:
            base=globals[item['name']]['original']
            old_address=int.from_bytes(bytes(memory[base+i]for i in range(4)),'little')
            new_address=state['globals'][item['name']]['address']
        ranges=[];writes=[]
        for a,b in zip(original_reads(events),native_reads or []):
            inside_old=old_address<=a['address']<old_address+count
            inside_new=new_address<=b['address']<new_address+count
            assert inside_old==inside_new,'scratch write allocation identity differs'
            if not inside_old:continue
            assert a['site']in item.get('allowed_write_sites',[]),'unreviewed scratch write site'
            offset=a['address']-old_address
            assert offset==b['address']-new_address,'scratch write allocation-relative pointer differs'
            if 'returned'in a or 'returned'in b:
                assert 'returned'in a and 'returned'in b and a['returned']==b['returned'],'incomplete scratch defining write'
                length=max(a['returned'],0)
            else:
                records=[r for r in partial_reads or []if r['site']==a['site']and r['original_address']==a['address']and r['native_address']==b['address']and r['count']==a['count']==b['count']]
                assert len(records)==1 and records[0]['pending']is True,'missing actual partial scratch write'
                length=records[0]['copied']
            assert 0<=offset<=offset+length<=count,'scratch defining write exceeds allocation'
            ranges.append((offset,offset+length));writes.append(dict(site=a['site'],offset=offset,length=length))
        merged=[]
        for lo,hi in sorted(ranges):
            if merged and lo<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],hi)
            else:merged.append([lo,hi])
        for lo,hi in item.get('required_defined',[]):
            assert any(a<=lo<hi<=b for a,b in merged),'scratch consumer reads before initialization'
        result[item['name']]=dict(size=count,defined_ranges=merged,write_evidence=writes,original_callsite=item['original_callsite'],lifetime=item['lifetime'])
    return result

def mask_undefined(name,left,right,definitions):
    proof=definitions[name]
    assert len(left)==len(right)==proof['size'],'scratch allocation extent differs'
    ranges=proof['defined_ranges']
    assert all(0<=lo<hi<=len(left)for lo,hi in ranges),'defined scratch range exceeds allocation'
    assert all(a[1]<b[0]for a,b in zip(ranges,ranges[1:])),'defined scratch ranges overlap'
    known_left,known_right=bytearray(len(left)),bytearray(len(right))
    for lo,hi in ranges:
        assert left[lo:hi]==right[lo:hi],'defined scratch bytes differ'
        known_left[lo:hi]=left[lo:hi];known_right[lo:hi]=right[lo:hi]
    # Raw bytes remain in original events/native GDB receipts. A zero comparison
    # mask covers only bytes that lack a proven defining write in this lifetime.
    left[:]=known_left;right[:]=known_right
    return proof
