"""Exact imported-delete support identity/ABI; never application coverage."""
import hashlib
import importlib
import json
from pathlib import Path
import re
import struct
from binary import PE, COFF, u16

BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
NAME='import:??3@YAXPAX@Z'
PROOF=BINARY/'cases/help/operator-delete/provider-proof.json'
sha=lambda data:hashlib.sha256(data).hexdigest()


def check(record, proof=None):
    assert record['name']==NAME and record['classification']=='import'
    assert record['library']=='MSVCRT.LIB'
    assert record['start']==0x4651b6 and record['end']==0x4651bc
    assert record['status']=='implemented' and not record.get('pending_extent')
    proof=json.loads(PROOF.read_text()) if proof is None else proof
    assert proof['identity']=='msvcrt.dll!??3@YAXPAX@Z'
    assert proof['abi']=='cdecl void(void*), stack4 caller cleanup, provider RET0'
    original=PE(ROOT/'toolchains/5.10/bin/LINK.EXE')
    assert proof['target_thunk']['begin']==record['start'] and proof['target_thunk']['end']==record['end'] and proof['target_thunk']['iat']==0x401214
    assert original.read(record['start'],6)==bytes.fromhex(proof['target_thunk']['bytes'])==b'\xff\x25\x14\x12\x40\0'
    catalog=importlib.import_module('import-residual')
    assert catalog.slots(original)[0x401214]==proof['identity']
    provider=proof['provider'];data=(ROOT/provider['path']).read_bytes()
    assert sha(data)==provider['sha256']=='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
    pe=PE(ROOT/provider['path']);begin,end=provider['operator_delete_rva']
    assert [begin,end]==[0x40d2,0x40e0]
    assert pe.read(pe.base+begin,end-begin).hex()==provider['operator_delete_bytes']
    alias=proof['alias'];alias_bytes=(BINARY/alias['path']).read_bytes()
    assert sha(alias_bytes)==alias['sha256'] and u16(alias_bytes,2)==0
    obj=COFF(alias_bytes);symbols={s['name']:s for s in obj.symbols.values()}
    assert symbols['_crt_operator_delete']['storage']==105 and symbols['??3@YAXPAX@Z']['storage']==2
    assert not obj.sections
    assert struct.unpack_from('<II',alias_bytes,38)==(2,3), 'wrong weak alias target/search characteristic'
    rebuilt=PE(BINARY/'build/link.exe');text=(BINARY/'build/link.map').read_text()
    publics={n:int(a,16)for n,a in re.findall(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',text,re.M)}
    address=publics['_crt_operator_delete'];assert address==publics['??3@YAXPAX@Z']
    body=rebuilt.read(address,6);assert body[:2]==b'\xff\x25'
    assert catalog.slots(rebuilt)[int.from_bytes(body[2:],'little')]==proof['identity']
    controlled=BINARY/proof['controlled_probe']['path']
    receipt_bytes=(controlled/'receipt.json').read_bytes();receipt=json.loads(receipt_bytes)
    assert sha(receipt_bytes)==proof['controlled_probe']['raw_receipt_sha256']
    assert receipt['actual_target_hfile_verified'] and receipt['actual_loaded_canonical_provider']==[proof['controlled_probe']['actual_provider_module']]
    assert receipt['files'][proof['controlled_probe']['actual_provider_module']]['sha256']==provider['sha256']
    assert sha((BINARY/proof['controlled_probe']['owned_provider_copy']).read_bytes())==provider['sha256']
    assert sha((controlled/'delete-probe.bin').read_bytes())==proof['controlled_probe']['artifact_sha256']
    validation=json.loads((controlled/'validation.json').read_text());assert validation['status']=='pass' and len(validation['records'])==12
    assert validation['receipt_sha256']==sha(receipt_bytes) and validation['artifact_sha256']==proof['controlled_probe']['artifact_sha256']
    for item in validation['records']:
        assert item['errno_before']==item['errno_after'] and item['last_error_before']==item['last_error_after']
    observed=proof['actual_target_observation'];owned=BINARY/observed['owned_path']
    target_receipt=(owned/'receipt.json').read_bytes();assert sha(target_receipt)==observed['receipt_sha256']
    target=json.loads(target_receipt);assert target['actual_loaded_crt_verified'] and target['actual_loaded_target_hfile_verified']
    events=[json.loads(line)for line in (owned/'events.jsonl').read_text().splitlines()]
    new=next(e for e in events if e.get('name')=='observe:new_return_4')
    calls=[e for e in events if e['event']=='operator_delete_observation']
    assert calls==[observed['delete_call'],observed['delete_return']]
    assert calls[0]['argument']==calls[1]['argument']==new['eax']
    assert calls[0]['stack_pointer']==calls[1]['stack_pointer']
    assert calls[0]['descriptor']==calls[1]['descriptor']==[new['eax'],1024,1024]
    return dict(name=NAME,classification='support/import',application_credit=0,original_bytes=6,proof_sha256=sha(PROOF.read_bytes()),qualified_identity=proof['identity'])
