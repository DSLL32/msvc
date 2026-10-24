"""Narrow, explicit static-only pending ownership correction; never executes code."""
import json
from state import BINARY, ROOT, TARGET, sha, ranges, offset, masked, source_hashes, trial_configuration

FIELDS = {'code_chunks', 'shared', 'extent_evidence'}


def delta(old_bytes, current_bytes, correction, original, inventory):
    assert correction['schema'] == 1
    assert sha(old_bytes) == correction['historical_base_sha256'], 'wrong historical configuration'
    assert sha(current_bytes) == correction['current_base_sha256'], 'unreviewed configuration delta'
    old = json.loads(old_bytes); current = json.loads(current_bytes)
    assert old != current, 'static correction is empty'
    assert old['target'] == current['target'] and sha(original) == correction['target_sha256'] == old['target']['sha256']
    assert len(old['stubs']) == len(current['stubs'])
    changes = []
    for a, b in zip(old['stubs'], current['stubs']):
        if a == b: continue
        name = a['name']
        assert name == b['name'] and name not in old['accepted'] + old['trial'], 'accepted/candidate owner changed'
        for record in (a, b):
            assert record['kind'] == 'application' and record['extent_known_for_tail_abi'] is True
            assert record['pending_extent'] is False and record.get('extent_evidence'), 'not an extent-known pending stub'
        changed = {key for key in a.keys() | b.keys() if a.get(key) != b.get(key)}
        assert changed and changed <= FIELDS, 'nonstatic pending property changed'
        changes.append({'name': name, 'old': {k:a.get(k) for k in sorted(changed)}, 'new': {k:b.get(k) for k in sorted(changed)}})
    assert changes == correction['changes'], 'delta exceeds explicit reviewed correction'
    old['stubs'] = current['stubs']
    assert old == current, 'configuration outside pending metadata changed'
    assert changes, 'no pending metadata changed'
    # Exact physical interval hashes prevent an envelope, shortened chunk or
    # owner substitution. This is byte/ownership validation, not a CFG walker.
    owners = []
    for record in current['driver'] + current['support'] + current['text_data'] + current['stubs']:
        if record in current['stubs'] and not record.get('extent_known_for_tail_abi'): continue
        for start, end in ranges(record):
            assert start < end
            offset(inventory, start, end-start)
            assert all(end <= x or y <= start for x,y,_ in owners), 'pending code overlaps an existing owner'
            owners.append((start,end,record['name']))
    expected = []
    for change in changes:
        record = next(r for r in current['stubs'] if r['name'] == change['name'])
        for kind, intervals in (('code', ranges(record)), ('data', record.get('shared', []))):
            for start,end in intervals:
                assert start < end
                if kind == 'data': assert all(end <= a or b <= start for a,b,_ in owners), 'pending data intersects code'
                pos=offset(inventory,start,end-start)
                expected.append({'owner':record['name'],'kind':kind,'start':start,'end':end,'sha256':sha(original[pos:pos+end-start])})
    assert expected == correction['owned_intervals'], 'exact pending owned intervals changed'
    return changes


def certificate(original_path, native_path, current_bytes, correction_path, original, inventory):
    correction_bytes=correction_path.read_bytes(); correction=json.loads(correction_bytes)
    old_bytes=(original_path/'base_configuration.json').read_bytes()
    assert (native_path/'base_configuration.json').read_bytes()==old_bytes, 'historical pair bases differ'
    changes=delta(old_bytes,current_bytes,correction,original,inventory)
    for item in correction['evidence']:
        assert sha((BINARY/item['path']).read_bytes()) == item['sha256'], 'correction evidence changed'
    old=json.loads(old_bytes); current=json.loads(current_bytes)
    historical_trial,historical_bytes=trial_configuration(old_bytes)
    current_trial,current_trial_bytes=trial_configuration(current_bytes)
    original_receipt=json.loads((original_path/'receipt.json').read_text())
    native_receipt=json.loads((native_path/'receipt.json').read_text())
    for path, receipt in ((original_path,original_receipt),(native_path,native_receipt)):
        assert (path/'configuration.json').read_bytes()==historical_bytes
        assert receipt['base_configuration_sha256']==sha(old_bytes) and receipt['configuration_sha256']==sha(historical_bytes)
        assert receipt['source_hashes']==source_hashes(), 'source changed during static-only correction'
    assert native_receipt['native_sha256']==sha((BINARY/'build/native/c2').read_bytes())==sha((native_path/'native.elf').read_bytes()), 'ELF changed'
    assert original_receipt['rebuilt_sha256']==sha((BINARY/'build/c2.exe').read_bytes())==sha((original_path/'rebuilt.exe').read_bytes()), 'Win32 executable changed'
    assert original_receipt['rebuilt_map_snapshot_sha256']==sha((BINARY/'build/c2.map').read_bytes()), 'MAP changed'
    assert original_receipt['debugger_sha256']==sha((BINARY/'build/debug-run.exe').read_bytes())==sha((original_path/'capture-helper.exe').read_bytes()), 'live capture helper changed'
    helper=json.loads((original_path/'helper-inputs.json').read_text())
    for name,digest in helper['inputs'].items():
        # Shared worktree drift does not rewrite the immutable compiled input;
        # the actual helper binary must remain identical above.
        assert sha((original_path/'helper-inputs'/name).read_bytes())==digest
        if name.startswith(str(BINARY.relative_to(ROOT))+'/'):
            assert sha((ROOT/name).read_bytes())==digest, 'owned controller/build input changed'
    old_runtime=json.loads((original_path/'runtime-original.json').read_text())
    new_runtime=json.loads((BINARY/'runtime.json').read_text())
    projected=json.loads(json.dumps(old_runtime))
    for change in changes:
        entry=next(r for r in projected if r['name']==change['name'])
        assert entry['kind']=='unaccepted-application-tail-target' and entry['classification']=='application'
        assert entry['accepted'] is False and entry['evidence'] is None and entry['pending_extent'] is False
        for key,value in change['new'].items():
            if value is None: entry.pop(key,None)
            else: entry[key]=value
    assert projected==new_runtime, 'runtime delta exceeds corrected pending metadata'
    for historical,live in (('runtime-rebuilt.json','build/runtime.json'),('graph-metadata.json','build/graph-metadata.json')):
        assert (original_path/historical).read_bytes()==(BINARY/live).read_bytes(), 'rebuilt/graph metadata changed'
    image=bytearray(masked(current_trial,original,inventory,current_trial['trial']))
    historical_image=bytearray(masked(historical_trial,original,inventory,historical_trial['trial']))
    assert image==historical_image, 'physical unmask changed'
    entries=[]
    for r in current_trial['driver']+current_trial['support']:
        if r['name'] in current_trial['accepted']+current_trial['trial']:
            p=offset(inventory,r['start']);entries.append((r['start'],image[p]));image[p]=0xcc
    plan=''.join('%x %x\n'%r for r in entries).encode()
    assert plan==(original_path/'plan.txt').read_bytes(), 'breakpoint controller plan changed'
    assert bytes(image)==(original_path/'C2.EXE').read_bytes() and sha(image)==original_receipt['executable_sha256'], 'masked execution bytes changed'
    return {'mode':'pending-metadata-only-v1','correction_path':str(correction_path.relative_to(BINARY)),
        'correction_sha256':sha(correction_bytes),'changes':changes,
        'original_execution_receipt_sha256':sha((original_path/'receipt.json').read_bytes()),
        'native_execution_receipt_sha256':sha((native_path/'receipt.json').read_bytes()),
        'historical_base_sha256':sha(old_bytes),'historical_trial_sha256':sha(historical_bytes),
        'current_base_sha256':sha(current_bytes),'current_trial_sha256':sha(current_trial_bytes),
        'physical':{'masked_sha256':sha(image),'plan_sha256':sha(plan),'helper_sha256':original_receipt['debugger_sha256'],
            'native_sha256':native_receipt['native_sha256'],'rebuilt_sha256':original_receipt['rebuilt_sha256'],
            'historical_runtime_sha256':sha((original_path/'runtime-original.json').read_bytes()),
            'current_runtime_sha256':sha((BINARY/'runtime.json').read_bytes())}}
