#!/usr/bin/env python3
"""Reject damaged owned-method observations without executing an original."""
import copy
import json
import re
from pathlib import Path

import importlib.util
HERE=Path(__file__).resolve().parent


def load(name):
    spec=importlib.util.spec_from_file_location(name,HERE/(name+'.py'))
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result)
    return result


def changed(text,name,offset,value):
    pattern=r'^(pdb-memory '+re.escape(name)+r' [0-9a-f]+ \d+ )([0-9a-f]+)$'
    def replace(match):
        raw=bytearray.fromhex(match[2]);raw[offset:offset+len(value)]=value
        return match[1]+raw.hex()
    result,count=re.subn(pattern,replace,text,flags=re.M);assert count==1
    return result


def main():
    checker=load('check-pdb-objects');contract=load('pdb-object-contract');runner=load('pdb-native')
    manifest=json.loads((checker.CASE/'manifest.json').read_text())
    checker.validate_reference(manifest,runner,contract)
    directory=checker.CASE/'evidence/direct/native-ABI-0'
    raw=(directory/'report').read_text();expected=manifest['direct_method_oracles'][0]['contract']
    def valid(text):
        assert checker.canonical(contract.record(text,0,True))==expected
    valid(raw)
    def result_field(offset,value):
        return changed(changed(raw,'object-result',offset,value),'allocation-after-0',offset,value)
    mutations={
        'missing allocation owner':re.sub(r'^pdb-allocation 6 .*\n','',raw,flags=re.M),
        'foreign type vector':result_field(64,b'\x01\0\0\0'),
        'wrong descriptor state':result_field(100,b'\xff'*4),
        'record payload differs':changed(raw,'allocation-after-5',12,b'\x99'),
        'type-entry padding overwritten':changed(raw,'allocation-after-6',22,b'\x99'),
        'unwritten block tail overwritten':changed(raw,'allocation-after-6',240,b'\x99'),
        'foreign hash entry':changed(changed(raw,'index-result-0',0,b'\x01\0\0\0'),
                                     'allocation-after-4',0,b'\x01\0\0\0'),
        'changed error state':re.sub(r'^(pdb-errors result .* )\S+$',r'\g<1>ffffffff',raw,flags=re.M),
        'wrong caller stack':re.sub(r'^(pdb-native-return 1 1 )\S+',r'\g<1>00000000',raw,flags=re.M),
        'changed floating control':re.sub(r'^(pdb-native-return 1 .* )\S+$',r'\g<1>0000',raw,flags=re.M),
        'missing guarded stack':raw.replace('pdb-native-stack-guards intact',''),
    }
    rejected=[]
    for label,damaged in mutations.items():
        try:valid(damaged)
        except (AssertionError,KeyError):rejected.append(label)
        else:raise AssertionError('accepted '+label)
    bad=copy.deepcopy(manifest)
    first=bad['direct_method_oracles'][0]['baselines'][0]
    first['files']['events']='0'*64
    try:checker.validate_reference(bad,runner,contract)
    except AssertionError:rejected.append('original cache hash differs')
    else:raise AssertionError('accepted changed cache hash')
    positive=raw
    for offset in (0,96,112):
        for label in ('allocation-before-0','object-constructor'):
            positive=changed(positive,label,offset,b'\x53\x46\x79\x82')
    for label in ('allocation-before-6','allocation-after-6'):
        positive=changed(positive,label,22,b'\x53\x46')
    valid(positive)
    print(json.dumps(dict(status='pass',rejected=rejected,
        own_unwritten_constructor_and_entry_padding_positive=True),indent=2))


if __name__=='__main__':
    main()
