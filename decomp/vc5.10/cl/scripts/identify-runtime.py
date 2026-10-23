#!/usr/bin/env python3
"""Exact shipped dynamic-CRT startup code, relocation and contextual-data proof."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
from binary import PE,COFF,archive

ROOT=Path(__file__).resolve().parents[4]
PROJECT=Path(__file__).resolve().parents[1]
TC=ROOT/'toolchains/5.10'
sha=lambda data:hashlib.sha256(data).hexdigest()


def imports(pe):
    slots={};thunks={}
    p=pe.offset(pe.base+struct.unpack_from('<I',pe.data,pe.optional+104)[0])
    while True:
        lookup,_,_,name,iat=struct.unpack_from('<IIIII',pe.data,p)
        if not name:break
        q=pe.offset(pe.base+lookup);i=0
        while True:
            value=struct.unpack_from('<I',pe.data,q+4*i)[0]
            if not value:break
            assert not value&0x80000000,'ordinal import not expected'
            symbol=pe.string(pe.base+value+2)
            slots['__imp__'+symbol]=pe.base+iat+4*i;i+=1
        p+=20
    reverse={address:symbol for symbol,address in slots.items()}
    for section in pe.sections:
        if not section['flags']&0x20:continue
        data=pe.data[section['off']:section['off']+section['virtual_size']]
        for i in range(len(data)-5):
            if data[i:i+2]==b'\xff\x25':
                at=struct.unpack_from('<I',data,i+2)[0]
                if at in reverse:thunks['_'+reverse[at][len('__imp__'):]]=pe.base+section['rva']+i
    return slots,thunks


def public_map(path):
    result={}
    for line in path.read_text().splitlines():
        match=re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',line)
        if match:result[match[1]]=int(match[2],16)
    return result


def prove(path,rebuilt=False):
    pe=PE(path);slots,thunks=imports(pe)
    fixed={'_mainCRTStartup':0x4058e0,'__setargv':0x405880,'___setargv':0x4058d0,
        '__setdefaultprecision':0x405a40,'__matherr':0x405a60,'_main':0x401743,
        '__dowildcard':0x4097b4,'__newmode':0x4097b8,'__commode':0x4097bc,
        '__fmode':0x4097c0,'___onexitend':0x4097c4,'___onexitbegin':0x4097c8,
        '__adjust_fdiv':0x4097cc,'___defaultmatherr':0x4091b0,
        '___xi_z':0x40900c,'___xi_a':0x409008,'___xc_z':0x409004,'___xc_a':0x409000,
        '__except_list':0}
    addresses=public_map(PROJECT/'build/cl.map') if rebuilt else fixed
    addresses={**addresses,**thunks,**slots,'__except_list':0}
    members={name:data for name,data in archive(TC/'lib/MSVCRT.LIB')}
    objects=[('_mainCRTStartup','build\\intel\\dll_obj\\crtexe.obj'),
        ('___setargv','build\\intel\\dll_obj\\dll_argv.obj'),
        ('__setdefaultprecision','build\\intel\\dll_obj\\fp8.obj'),
        ('__matherr','build\\intel\\dll_obj\\merr.obj')]
    objects.append(('__setargv','SETARGV.OBJ'))
    records=[]
    defaults={}
    matherr_object=COFF(members['build\\intel\\dll_obj\\merr.obj'])
    matherr_default=next(section['data'] for section in matherr_object.sections if section['name']=='.data')
    assert len(matherr_default)==4
    for name in ('__fmode','__commode','__newmode','__dowildcard','___defaultmatherr','__adjust_fdiv','___onexitbegin','___onexitend'):
        address=addresses[name]
        section=next(s for s in pe.sections if pe.base+s['rva']<=address<pe.base+s['rva']+s['virtual_size'])
        relative=address-pe.base-section['rva']
        raw=pe.read(address,4) if relative+4<=section['size'] else bytes(4)
        expected=matherr_default if name=='___defaultmatherr' else bytes(4)
        assert raw==expected,'wrong CRT initial data '+name
        defaults[name]={'address':address,'bytes':raw.hex(),'source':'merr.obj .data' if name=='___defaultmatherr' else 'zero-initialized common storage'}
    for name in ('___xi_a','___xi_z','___xc_a','___xc_z'):
        assert pe.read(addresses[name],4)==bytes(4),'nonempty CRT initializer boundary'
    for function,member in objects:
        source=(TC/'lib/SETARGV.OBJ').read_bytes() if member=='SETARGV.OBJ' else members[member]
        obj=COFF(source);text=next(s for s in obj.sections if s['name']=='.text')
        start=addresses[function];section_bases={text['index']:start};bindings=[]
        pending=[text]
        while pending:
            section=pending.pop(0);base=section_bases[section['index']]
            original=pe.read(base,len(section['data']));mask=bytearray(b'\1'*len(original))
            for offset,symbol,kind in section['relocs']:
                assert kind in (6,20),'unsupported runtime relocation'
                addend=struct.unpack_from('<i' if kind==20 else '<I',section['data'],offset)[0]
                field=struct.unpack_from('<i' if kind==20 else '<I',original,offset)[0]
                actual=(base+offset+4+field if kind==20 else field)-addend
                if symbol['section']>0:
                    symbol_section=symbol['section']
                    if symbol_section not in section_bases:
                        section_bases[symbol_section]=actual-symbol['value']
                        pending.append(obj.sections[symbol_section-1])
                    expected=section_bases[symbol_section]+symbol['value']
                else:
                    assert symbol['name'] in addresses,'unbound runtime external '+symbol['name']
                    expected=addresses[symbol['name']]
                assert actual==expected,'wrong runtime binding '+symbol['name']
                bindings.append({'section':section['name'],'offset':offset,'symbol':symbol['name'],
                    'kind':kind,'actual':actual,'expected':expected})
                mask[offset:offset+4]=b'\0'*4
            assert all(not keep or a==b for a,b,keep in zip(original,section['data'],mask)),function+' bytes differ'
        evidence={'library':'SETARGV.OBJ' if member=='SETARGV.OBJ' else 'MSVCRT.LIB',
            'library_sha256':sha((TC/'lib/SETARGV.OBJ' if member=='SETARGV.OBJ' else TC/'lib/MSVCRT.LIB').read_bytes()),
            'member':member,'member_sha256':sha(source),'bindings':bindings,
            'context_sections':{str(i):base for i,base in section_bases.items()},
            'initial_data':defaults if function=='_mainCRTStartup' else {}}
        records.append({'name':function,'start':start,'end':start+len(text['data']),'evidence':evidence})
    support=json.loads((PROJECT/'cases/help/import-support.json').read_text())
    for symbol in ['__except_handler3','__XcptFilter','__initterm','__controlfp']+['_'+name for name in support]:
        start=thunks[symbol]
        proof={'kind':'exact FF25 IAT import thunk','iat':struct.unpack('<I',pe.read(start+2,4))[0],'import_symbol':symbol}
        if symbol[1:] in support:
            # Resolve the exact slot through the real PE descriptor, not a name
            # guessed from a public map or the thunk's lack of RET.
            descriptor=pe.base+struct.unpack_from('<I',pe.data,pe.optional+104)[0]
            found=None
            while True:
                lookup,_,_,dll,iat=struct.unpack('<5I',pe.read(descriptor,20))
                if not dll:break
                i=0
                while True:
                    entry=struct.unpack('<I',pe.read(pe.base+lookup+4*i,4))[0]
                    if not entry:break
                    if pe.base+iat+4*i==proof['iat']:
                        assert not entry&0x80000000 and pe.string(pe.base+entry+2)==symbol[1:]
                        found={'dll':pe.string(pe.base+dll).lower(),'name':symbol[1:]}
                    i+=1
                descriptor+=20
            assert found,'unbound support import slot'
            proof['qualified_identity']=found
            assert proof['qualified_identity']['dll']=='msvcrt.dll'
            proof['abi']=support[symbol[1:]]
        records.append({'name':'import:'+symbol[1:],'start':start,'end':start+6,'evidence':proof})
    return records


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rebuilt',action='store_true');parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    path=PROJECT/'build/cl.exe' if args.rebuilt else TC/'bin/CL.EXE'
    result=prove(path,args.rebuilt)
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print('verified',len(result),'startup/archive/import records;',sum(r['end']-r['start'] for r in result),'extent bytes')
