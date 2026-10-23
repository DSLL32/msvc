"""Verify the exact canonical _write callee-saved ESI witness, outside app credit."""
import hashlib
import json
from pathlib import Path
from binary import PE
from import_residuals import qualified_slots


def verify(project,root,proof=None):
    proof=proof or json.loads((project/'cases/link-object/write-preservation.json').read_text())
    target=PE(root/'toolchains/5.10/bin/CL.EXE')
    assert hashlib.sha256(target.data).hexdigest()==proof['target_sha256']
    assert proof['identity']=={'dll':'msvcrt.dll','name':'_write'} and proof['iat']==0x40a298
    assert qualified_slots(target)[proof['iat']]==proof['identity']
    assert proof['load']=={'address':0x40526f,'bytes':'8b3598a24000'}
    assert target.read(proof['load']['address'],6)==bytes.fromhex(proof['load']['bytes'])
    assert proof['calls']==[0x405288,0x40529c]
    assert all(target.read(site,2)==b'\xff\xd6'for site in proof['calls'])
    provider=proof['provider'];pe=PE(root/provider['path'])
    assert provider['sha256']=='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
    assert hashlib.sha256(pe.data).hexdigest()==provider['sha256']
    assert provider['ordinal']==533 and provider['entry_rva']==0x3303
    assert [(r['start_rva'],r['end_rva'])for r in provider['ranges']]==[(0x3303,0x3361),(0xbb68,0xbb84)]
    for row in provider['ranges']:
        data=pe.read(pe.base+row['start_rva'],row['end_rva']-row['start_rva'])
        assert data==bytes.fromhex(row['bytes']) and hashlib.sha256(data).hexdigest()==row['sha256']
    assert pe.read(pe.base+0x3308,1)==b'\x56'
    assert pe.read(pe.base+0x335e,3)==b'\x5f\x5e\xc3'
    assert pe.read(pe.base+0xbb82,2)==b'\x5e\xc3'
    return {'qualified_import':'msvcrt.dll!_write','preserves':['esi'],'sites':proof['calls']}


def verify_report(report,project,root):
    verify(project,root)
    flow=report['functions']['_write_link_message']['register_imports']
    expected={'dll':'msvcrt.dll','name':'_write'}
    assert flow['require_all_register_calls_resolved'] is True
    assert flow['original']['resolved_register_calls']=={str(site):expected for site in (0x405288,0x40529c)}
    assert flow['original']['direct_import_calls']=={}
    assert flow['original']['roots']==[0x405259]
    assert all(flow[side]['unresolved_register_calls']==[] and flow[side]['other_unresolved_calls']==[]for side in ('original','rebuilt'))
    assert flow['rebuilt']['resolved_register_calls']=={}
    from importlib.machinery import SourceFileLoader
    identity=SourceFileLoader('cl_write_runtime_identity',str(project/'scripts/identify-runtime.py')).load_module()
    start=identity.public_map(project/'build/cl.map')['_write_link_message']
    assert flow['rebuilt']['roots']==[start]
    assert flow['rebuilt']['direct_import_calls']=={str(start+offset):expected for offset in (0x3f,0x5b)}
    rebuilt=PE(project/'build/cl.exe');slots=qualified_slots(rebuilt)
    import struct
    for offset in (0x3f,0x5b):
        data=rebuilt.read(start+offset,6)
        assert data[:2]==b'\xff\x15' and slots[struct.unpack('<I',data[2:])[0]]==expected
    return flow
