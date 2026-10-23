"""Verify narrow, manually inspected cached-IAT register lifetime evidence.

This is not general CFG/register-dataflow discovery. The shared graph engine
cannot recover cached imports, so these exact target instruction spans retain
that explicit limitation instead of claiming the generic indirect edge proves
import identity.
"""
import hashlib
import json
from pathlib import Path
import struct
from binary import PE


def qualified_slots(pe):
    result={};descriptor=pe.base+struct.unpack_from('<I',pe.data,pe.optional+104)[0]
    while True:
        lookup,_,_,dll,iat=struct.unpack('<5I',pe.read(descriptor,20))
        if not dll:return result
        i=0
        while True:
            entry=struct.unpack('<I',pe.read(pe.base+lookup+4*i,4))[0]
            if not entry:break
            assert not entry&0x80000000
            result[pe.base+iat+4*i]={'dll':pe.string(pe.base+dll).lower(),'name':pe.string(pe.base+entry+2)}
            i+=1
        descriptor+=20


def verify(config,project,root):
    record=json.loads((project/'cases/help/import-residuals.json').read_text())
    pe=PE(root/config['target']['path']);slots=qualified_slots(pe)
    assert record['target_sha256']==hashlib.sha256(pe.data).hexdigest(),'cached-import evidence targets different PE'
    expected=set(config['accepted']) & {'_main','_split_arguments','_format_message','_validate_value','_format_number'}
    selected=[r for r in record['residuals'] if r['function'] in expected]
    assert {r['function'] for r in selected}==expected,'missing cached-import body evidence'
    for entry in selected:
        assert slots[entry['iat']]==entry['identity'],'cached IAT slot identity differs'
        for point in entry['instruction_sites']:
            data=bytes.fromhex(point['bytes'])
            assert pe.read(point['address'],len(data))==data,'cached import load/call site changed'
        span=entry['inspected_lifetime_span']
        assert hashlib.sha256(pe.read(span['start'],span['end']-span['start'])).hexdigest()==span['sha256'],'inspected register lifetime span changed'
        assert entry['register'] in ('esi','edi','ebx') and entry['lifetime_evidence'],'missing explicit lifetime residual'
    return sorted(expected)
