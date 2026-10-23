"""Verify the narrow cached GetProcAddress witness and common CFG site inventory."""
import hashlib
import json
from pathlib import Path
import struct
from binary import PE
from import_residuals import qualified_slots


def verify(report, project, root):
    proof = json.loads((project/'cases/minimal-c/getprocaddress-preservation.json').read_text())
    assert proof['preserves'] == ['edi']
    assert proof['qualified_import'] == {'dll':'kernel32.dll','name':'GetProcAddress'}
    assert proof['original'] == {'load':0x40564f,'iat':0x40a1d8,
        'calls':[0x405655,0x405660],'genuine_invoke':0x4056c9}, 'qualified source site witness changed'
    original = PE(root/'toolchains/5.10/bin/CL.EXE')
    assert hashlib.sha256(original.data).hexdigest() == proof['target_sha256']
    blobs = {}
    for name, item in proof['raw'].items():
        data = (project/item['path']).read_bytes()
        assert hashlib.sha256(data).hexdigest() == item['sha256']
        blobs[name] = data
    for name, item in proof['headers'].items():
        data = (root/item['path']).read_bytes()
        assert hashlib.sha256(data).hexdigest() == item['sha256']
    assert proof['helper_build']['sources']['scripts/dependency-observer.inc'] == proof['raw']['observer-source.txt']['sha256']
    observer = blobs['observer-source.txt']
    assert b'memory(target_base+0xa1d8,&function,4);' in observer
    assert b'if (context->Edi!=function || parameters[0]!=dependency_c1) ExitProcess(105);' in observer
    edi = []
    for phase in ('get-invoke-before','get-invoke-after','get-abort-before','get-abort-after'):
        context = blobs[phase+'-context.bin']
        assert len(context) == 204
        edi.append(struct.unpack_from('<I',context,156)[0])
    assert len(set(edi)) == 1 and edi[0]
    for api, address, cleanup in (('get-invoke',0x10612229,8),('get-abort',0x10643975,8)):
        before, after = (blobs[api+'-'+phase+'-context.bin'] for phase in ('before','after'))
        assert struct.unpack_from('<I',after,176)[0] == address
        assert struct.unpack_from('<I',after,196)[0]-struct.unpack_from('<I',before,196)[0] == cleanup
        assert struct.unpack_from('<I',blobs[api+'-before-stack.bin'])[0] == 0x10600000
    assert qualified_slots(original)[0x40a1d8] == proof['qualified_import']
    assert original.read(0x40564f,6) == bytes.fromhex('8b3dd8a14000')
    for address in (0x405655,0x405660):assert original.read(address,2) == bytes.fromhex('ffd7')
    assert original.read(0x4056c9,3) == bytes.fromhex('ff5618')
    # The report below is produced by the common CFG/dataflow implementation.
    # Its exact register-import and genuinely indirect site inventory is checked
    # here; there is no second target-specific graph discovery engine.
    flow = report['functions']['_execute_pass']['register_imports']
    assert flow['require_all_register_calls_resolved'] is False
    expected = {'dll':'kernel32.dll','name':'GetProcAddress'}
    left, right = flow['original'], flow['rebuilt']
    assert left['resolved_register_calls'] == {str(address):expected for address in (0x405655,0x405660)}, 'cached GPA site inventory differs'
    assert left['unresolved_register_calls'] == []
    assert left['other_unresolved_calls'] == [0x4056c9], 'unobserved original indirect call'
    assert left['roots'] == [0x40550d,0x4056d7,0x4056f5], 'missing original SEH roots'
    from importlib.machinery import SourceFileLoader
    identity = SourceFileLoader('cl_gpa_runtime_identity',str(project/'scripts/identify-runtime.py')).load_module()
    start = identity.public_map(project/'build/cl.map')['_execute_pass']
    rebuilt = PE(project/'build/cl.exe')
    assert right['resolved_register_calls'] == {} and right['unresolved_register_calls'] == []
    assert right['other_unresolved_calls'] == [start+0x289], 'unobserved rebuilt indirect call'
    assert rebuilt.read(start+0x289,3) == bytes.fromhex('ff5118'), 'rebuilt Invoke slot/shape differs'
    gpa_sites = {int(address):value for address,value in right['direct_import_calls'].items() if value == expected}
    assert gpa_sites == {start+0x1cc:expected,start+0x1de:expected}, 'rebuilt GPA sites differ'
    assert set(right['roots']) == {start,start+0x298,start+0x2c8}, 'missing rebuilt SEH roots'
    inputs = report['register_import_inputs']
    witness_path = project/'cases/minimal-c/getprocaddress-preservation.json'
    contract = {'target':{'import':expected},'preserves':['edi'],
                'evidence':{'path':'cases/minimal-c/getprocaddress-preservation.json',
                            'sha256':hashlib.sha256(witness_path.read_bytes()).hexdigest()}}
    expected_contracts=[contract]
    write_witness=project/'cases/link-object/write-preservation.json'
    if write_witness.is_file():
        from write_preservation import verify as verify_write
        verify_write(project,root)
        expected_contracts.append({'target':{'import':{'dll':'msvcrt.dll','name':'_write'}},
            'preserves':['esi'],'evidence':{'path':'cases/link-object/write-preservation.json',
                'sha256':hashlib.sha256(write_witness.read_bytes()).hexdigest()}})
    assert inputs['preservation_contracts'] == expected_contracts, 'missing or broader preservation contract'
    assert inputs['original_sha256'] == hashlib.sha256(original.data).hexdigest()
    assert inputs['rebuilt_sha256'] == hashlib.sha256(rebuilt.data).hexdigest()
    metadata=json.loads((project/'graph-metadata.json').read_text())
    canonical=json.dumps(metadata,sort_keys=True,separators=(',',':'),ensure_ascii=False).encode()
    assert inputs['metadata_sha256'] == hashlib.sha256(canonical).hexdigest()
    return flow
