#!/usr/bin/env python3
"""Identify shipped LIBC functions by bytes AND all code relocations.

Matching bytes are candidates only: an external call must resolve to the actual
named archive function/import (or explicitly declared application callback).
Global-address relocation bindings are retained for consistency/state review.
"""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import struct
from binary import PE, COFF, archive, match_section, u32
from state import ROOT, BINARY

def imports(pe):
    directory=u32(pe.data,pe.optional+104)
    if not directory: return [],{}
    pointer=pe.offset(pe.base+directory)
    iat={}
    while u32(pe.data,pointer+12):
        names,addresses=u32(pe.data,pointer),u32(pe.data,pointer+16)
        for index in range(10000):
            value=u32(pe.data,pe.offset(pe.base+names+4*index))
            if not value: break
            name='ordinal_%d'%(value&65535) if value&0x80000000 else pe.string(pe.base+value+2)
            iat[pe.base+addresses+4*index]=name
        pointer+=20
    records=[]
    for section in pe.sections:
        if not section['flags']&0x20: continue
        code=pe.data[section['off']:section['off']+section['virtual_size']]
        for offset in range(len(code)-5):
            if code[offset:offset+2]==b'\xff\x25' and u32(code,offset+2) in iat:
                address=pe.base+section['rva']+offset
                records.append(dict(name='import:'+iat[u32(code,offset+2)],start=address,
                                    end=address+6,iat=u32(code,offset+2)))
    return records,iat

def identify(path, application):
    pe=PE(path)
    text=next(s for s in pe.sections if s['name']=='.text')
    code=pe.data[text['off']:text['off']+text['virtual_size']]
    library=ROOT/'toolchains/1.0-nt/lib/LIBC.LIB'
    anchors=[]
    anchor_path=BINARY/'runtime-anchors.json'
    if Path(path).resolve()==(ROOT/'toolchains/1.0-nt/bin/CVPACK.EXE').resolve() and anchor_path.exists():
        anchors=json.loads(anchor_path.read_text())['anchors']
        for anchor in anchors:
            assert anchor['classification']=='reviewed-variant-identity-only'
            assert not anchor['accepted'] and not anchor['unmask']
            raw=pe.read(anchor['start'],anchor['end']-anchor['start'])
            assert hashlib.sha256(raw).hexdigest()==anchor['bytes_sha256']
            evidence=BINARY/anchor['evidence']
            assert evidence.is_file() and hashlib.sha256(evidence.read_bytes()).hexdigest()==anchor['evidence_sha256'],'runtime anchor proof changed'
    candidates=[]
    for member,data in archive(library):
        obj=COFF(data)
        for section in obj.sections:
            if not section['flags']&0x20: continue
            groups=defaultdict(list)
            for symbol in obj.symbols.values():
                if symbol['section']==section['index'] and (symbol['type']==0x20 or symbol['storage']==2) and not symbol['name'].startswith('.'):
                    groups[symbol['value']].append(symbol['name'])
            positions=sorted(groups)
            for index,bias in enumerate(positions):
                end=positions[index+1] if index+1<len(positions) else len(section['data'])
                part=dict(data=section['data'][bias:end],relocs=[(offset-bias,sym,kind)
                          for offset,sym,kind in section['relocs'] if bias<=offset<end])
                if len(part['data'])<6: continue
                for offset in match_section(code,part):
                    start=pe.base+text['rva']+offset
                    candidates.append(dict(names=groups[bias],start=start,end=start+end-bias,
                        member=member,index=section['index'],bias=bias,part=part,
                        nonrelocation_bytes=len(part['data'])-4*len(part['relocs'])))
    imported,iat=imports(pe)
    import_names={r['name'][7:]:r['start'] for r in imported}
    rejected=[]
    while True:
        known=defaultdict(set)
        for name,address in application.items(): known[name].add(address)
        for anchor in anchors:known[anchor['name']].add(anchor['start'])
        for candidate in candidates:
            for name in candidate['names']:known[name].add(candidate['start'])
        kept=[]
        for candidate in candidates:
            bindings=[]; failure=None
            for offset,symbol,kind in candidate['part']['relocs']:
                assert kind in (6,20),kind
                name=symbol['name']
                actual=u32(pe.read(candidate['start']+offset,4),0)
                addend=struct.unpack_from('<i',candidate['part']['data'],offset)[0]
                if kind==20: actual=(actual+candidate['start']+offset+4)&0xffffffff
                target=(actual-addend)&0xffffffff
                expected=None
                if symbol['section']==candidate['index']:
                    expected={candidate['start']+symbol['value']-candidate['bias']}
                elif name.startswith('__imp_'):
                    imported_name=name[6:].lstrip('_').split('@')[0]
                    expected={address for address,identity in iat.items() if identity==imported_name}
                elif name.startswith('_') and '@' in name:
                    address=import_names.get(name[1:].split('@')[0])
                    expected=set() if address is None else {address}
                elif name in known:
                    expected=known[name]
                elif kind==20:
                    # Unknown/target-specific callees cannot certify a CRT wrapper.
                    expected=set()
                elif symbol['type']==0x20:
                    expected=set()
                if expected is not None and target not in expected:
                    failure=dict(offset=offset,symbol=name,kind=kind,actual=target,expected=sorted(expected))
                    break
                bindings.append(dict(offset=offset,symbol=name,kind=kind,target=target,
                                     section=symbol['section'],value=symbol['value']))
            if failure:
                rejected.append(dict(names=candidate['names'],start=candidate['start'],member=candidate['member'],failure=failure))
            else:
                candidate['bindings']=bindings;kept.append(candidate)
        if len(kept)==len(candidates):break
        candidates=kept
    records=[]
    for candidate in candidates:
        for name in candidate['names']:
            records.append(dict(name=name,start=candidate['start'],end=candidate['end'],
                object=candidate['member'],library='LIBC.LIB',
                nonrelocation_bytes=candidate['nonrelocation_bytes'],
                relocations=candidate['bindings']))
    globals=defaultdict(set)
    for record in records:
        for relocation in record['relocations']:
            if relocation['kind']==6:
                globals[relocation['symbol']].add(relocation['target'])
    return dict(schema=1,image_sha256=hashlib.sha256(Path(path).read_bytes()).hexdigest(),
        library_sha256=hashlib.sha256(library.read_bytes()).hexdigest(),
        functions=sorted(records,key=lambda r:(r['start'],r['name'])),imports=imported,
        global_bindings={name:sorted(addresses) for name,addresses in sorted(globals.items())},
        rejected=rejected,callee_identity_anchors=anchors)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--image',type=Path,default=ROOT/'toolchains/1.0-nt/bin/CVPACK.EXE')
    parser.add_argument('--main',type=lambda s:int(s,0),default=0x17000)
    parser.add_argument('--output',type=Path,default=BINARY/'runtime.json')
    args=parser.parse_args()
    result=identify(args.image,{'_main':args.main})
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(len(result['functions']),'strict runtime candidates;',len(result['rejected']),'relocation mismatches rejected')

if __name__=='__main__': main()
