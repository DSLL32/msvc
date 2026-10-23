#!/usr/bin/env python3
"""Mask all original executable sections, restoring only indexed accepted ranges."""
import argparse
import hashlib
import json
from pathlib import Path
from binary import PE

ROOT=Path(__file__).resolve().parents[4]
PROJECT=Path(__file__).resolve().parents[1]


def construct(config):
    target=ROOT/config['target']['path']
    pe=PE(target)
    assert hashlib.sha256(pe.data).hexdigest()==config['target']['sha256'],'target changed'
    result=bytearray(pe.data)
    intervals=[]
    for section in pe.sections:
        if section['flags']&0x20:
            start=section['off'];end=start+section['size']
            result[start:end]=b'\xcc'*(end-start)
            intervals.append((pe.base+section['rva'],pe.base+section['rva']+section['size']))
    accepted=set(config['accepted'])
    records={r['name']:r for r in config['driver']+config.get('runtime',[])}
    assert len(records)==len(config['driver'])+len(config.get('runtime',[])),'duplicate identity'
    assert accepted<=records.keys(),'unknown accepted identity'
    runtime_names={r['name'] for r in config.get('runtime',[])}
    effective=set(accepted)
    for name in accepted:
        support=records[name].get('support_closure',[])
        assert not support or name in runtime_names,'application attempted implicit support unmask'
        for dependency in support:
            assert dependency in runtime_names and records[dependency].get('evidence'),'unproved support dependency'
            assert not records[dependency].get('support_closure'),'nested support closure is not declared here'
            effective.add(dependency)
    verified_runtime={r['name']:r for r in json.loads((PROJECT/'runtime.json').read_text())}
    for name in effective&runtime_names:
        proof=verified_runtime.get(name)
        assert proof is not None,'runtime is not in independently verified identity index'
        for key in ('start','end','evidence'):
            assert records[name][key]==proof[key],'runtime mask/identity index differs: '+name
    owned=[]
    for name in effective:
        record=records[name]
        assert record.get('evidence'),'accepted range lacks evidence'
        for start,end in [(record['start'],record['end']),*record.get('text_data',[])]:
            assert start<end and any(a<=start<end<=b for a,b in intervals),'unbacked accepted extent'
            assert all(end<=a or start>=b for a,b in owned),'accepted ownership overlap'
            owned.append((start,end))
            a,b=pe.offset(start),pe.offset(end-1)+1
            result[a:b]=pe.data[a:b]
    assert bytes(result[:0x400])==pe.data[:0x400],'loader headers changed'
    for section in pe.sections:
        if not section['flags']&0x20:
            a=section['off'];b=a+section['size']
            assert result[a:b]==pe.data[a:b],'loader or data section changed'
    return bytes(result)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config',type=Path,default=PROJECT/'functions.json')
    parser.add_argument('--output',type=Path,default=PROJECT/'build/masked-original.exe')
    args=parser.parse_args()
    config=json.loads(args.config.read_text());data=construct(config)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data)
    print('masked image',len(data),'bytes',hashlib.sha256(data).hexdigest())
