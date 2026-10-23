"""Verify and encode an explicit cached target hardware sample; no Wine."""
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import struct
from mask import ROOT


def prepare(source,destination):
    receipt_bytes=(source/'receipt.json').read_bytes();receipt=json.loads(receipt_bytes)
    assert receipt['original_sha256']=='04c892989f4cc8076ef45e033215eb7aefbdf1459cbee9b13a81f8ddb718b579'
    assert receipt['actual_loaded_crt_verified'] and receipt['actual_loaded_target_hfile_verified']
    assert receipt['mode']=='masked' and receipt['helper_status']==0
    selected=['events.jsonl','system-before.bin','system-after.bin','memory-before.bin','memory-after.bin']
    for name in selected:
        data=(source/name).read_bytes();expected=receipt['files'][name]
        assert len(data)==expected['bytes'] and hashlib.sha256(data).hexdigest()==expected['sha256']
    events=[json.loads(line) for line in (source/'events.jsonl').read_text().splitlines()]
    calls=[e for e in events if e['event']=='system_observation']
    assert [(e['site'],e['bytes']) for e in calls]==[(0x44f571,36),(0x44f577,36),(0x44f586,32),(0x44f58c,32)]
    assert calls[0]['buffer']==calls[1]['buffer'] and calls[2]['buffer']==calls[3]['buffer']
    system=(source/'system-after.bin').read_bytes();memory=(source/'memory-after.bin').read_bytes()
    assert struct.unpack_from('<I',system,4)[0]==4096
    assert struct.unpack_from('<I',memory,8)[0]==0x7fffffff
    output=destination/'environment';output.mkdir()
    (output/'reference-receipt.json').write_bytes(receipt_bytes)
    for name in selected:shutil.copyfile(source/name,output/name)
    spec=importlib.util.spec_from_file_location('shared_system_profile',ROOT/'lib/recon/system_profile.py')
    shared=importlib.util.module_from_spec(spec);spec.loader.exec_module(shared)
    reference=hashlib.sha256(receipt_bytes).hexdigest();profile=output/'system-profile.bin'
    shared.write_profile(profile,reference,system,memory)
    (output/'binding.json').write_text(json.dumps(dict(source=str(source),reference_sha256=reference,
        consumed_fields={'SYSTEM_INFO.dwPageSize':4096,'MEMORYSTATUS.dwTotalPhys':0x7fffffff},
        volatile_fields='Full captured sample replayed exactly; no other target consumption is approved.'),indent=2)+'\n')
    return profile,reference
