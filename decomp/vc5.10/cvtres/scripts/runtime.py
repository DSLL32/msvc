"""Exact shipped MSVCRT.LIB code/data with relocation and import bindings."""
import hashlib
import json
from pathlib import Path
from binary import PE, COFF, archive, u32
from layout import import_addresses

ROOT=Path(__file__).resolve().parents[4]
TARGET=ROOT/'toolchains/5.10/bin/CVTRES.EXE'
LIBRARY=ROOT/'toolchains/5.10/lib/MSVCRT.LIB'

def identify():
    pe=PE(TARGET)
    members={name.rsplit('\\',1)[-1].lower():(name,data,COFF(data))
             for name,data in archive(LIBRARY) if data[:2]==b'\x4c\x01'}
    imports=import_addresses(pe)
    bindings={}
    records=[]
    def verify_member(member,start,logical):
        name,data,obj=members[member]
        section=next(s for s in obj.sections if s['flags']&0x20)
        expected=bytearray(section['data']);actual=pe.read(start,len(expected))
        relocated=[]
        for off,sym,kind in section['relocs']:
            assert kind in (6,20),(member,kind)
            linked=u32(actual,off)
            addend=u32(expected,off)
            value=((linked-addend) if kind==6 else (linked+start+off+4-addend))&0xffffffff
            old=bindings.setdefault(sym['name'],value)
            assert old==value,(sym['name'],old,value)
            if sym['section']>0 and sym['section']==section['index']:
                assert value==start+sym['value'],(member,sym['name'])
            if sym['name']=='__except_list':assert value==0
            if sym['name'].startswith('__imp_'):
                symbol=sym['name'][6:]
                assert value in imports,(member,sym['name'],hex(value))
                assert imports[value][0].lower()=='msvcrt.dll'
                # COFF's leading C underscore precedes the actual DLL export name.
                assert symbol=='_'+imports[value][1],(sym['name'],imports[value])
            expected[off:off+4]=actual[off:off+4]
            relocated.append(dict(offset=off,symbol=sym['name'],kind=kind,binding=value))
        assert bytes(expected)==actual,'non-relocated runtime bytes differ: '+member
        rec=dict(name=logical,start=start,end=start+len(actual),member=name,
                 member_sha256=hashlib.sha256(data).hexdigest(),relocations=relocated)
        records.append(rec)
        return obj,section
    obj,section=verify_member('crtexe.obj',0x1003180,'mainCRTStartup')
    assert bindings['_main']==0x10014a5
    for member,symbol,logical in [('fp8.obj','__setdefaultprecision','setdefaultprecision'),
                                  ('merr.obj','__matherr','matherr'),
                                  ('dllargv.obj','__setargv','setargv')]:
        verify_member(member,bindings[symbol],logical)
    for symbol,name in [('__except_handler3','_except_handler3'),('__XcptFilter','_XcptFilter'),
                        ('__initterm','_initterm'),('__controlfp','_controlfp')]:
        start=bindings[symbol];body=pe.read(start,6)
        assert body[:2]==b'\xff\x25'
        address=u32(body,2)
        assert imports[address]==('MSVCRT.dll',name),(symbol,imports[address])
        records.append(dict(name='import:'+name,start=start,end=start+6,iat=address))
    # Startup's scope table has exact internal filter/handler relocations.
    scope=next(s for s in obj.sections if s['name']=='.rdata')
    start=bindings['$T7942'];expected=bytearray(scope['data']);actual=pe.read(start,len(expected))
    for off,sym,kind in scope['relocs']:
        assert kind==6 and sym['section']==section['index']
        value=0x1003180+sym['value']+u32(expected,off)
        assert u32(actual,off)==value
        expected[off:off+4]=actual[off:off+4]
    assert bytes(expected)==actual
    data=[dict(start=start,end=start+len(actual),kind='verified-crtexe-scope-table')]
    return dict(schema=1,target_sha256=hashlib.sha256(pe.data).hexdigest(),
                library_sha256=hashlib.sha256(LIBRARY.read_bytes()).hexdigest(),
                functions=records,data=data,bindings=bindings,
                policy='Runtime identities only; no application acceptance or coverage credit.')
if __name__=='__main__':print(json.dumps(identify(),indent=2))
