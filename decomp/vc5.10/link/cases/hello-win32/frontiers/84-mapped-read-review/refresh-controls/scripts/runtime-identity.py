#!/usr/bin/env python3
"""Exact anchored shipped-archive identity analysis; does not accept/unmask code."""
import hashlib
import argparse
import json
from pathlib import Path
import struct
import re
from binary import PE, COFF, archive, u32
from mask import ROOT, BINARY


def analyze(target=None, main_address=0x40de90, atexit_address=0x465250):
    target = target or ROOT / 'toolchains/5.10/bin/LINK.EXE'
    library = ROOT / 'toolchains/5.10/lib/MSVCRT.LIB'
    pe = PE(target)
    definitions = {}
    for member, data in archive(library):
        try: obj = COFF(data)
        except (AssertionError, IndexError, ValueError, struct.error): continue
        for symbol in obj.symbols.values():
            if symbol['storage'] == 2 and symbol['section'] > 0:
                definitions.setdefault(symbol['name'], []).append((member, data, obj, symbol))
    pins = {'_mainCRTStartup':pe.base + pe.entry, '_main':main_address, '_atexit':atexit_address, '__except_list':0}
    reasons = {name:'verified target entry/application main/Win32 FS exception-list offset'
               for name in pins}
    descriptor = pe.base + u32(pe.data, pe.optional + 104)
    while u32(pe.read(descriptor + 12,4),0):
        names, _, _, dll_name, iat = struct.unpack('<IIIII', pe.read(descriptor,20))
        dll = pe.string(pe.base + dll_name)
        index = 0
        while True:
            value = u32(pe.read(pe.base + names + index*4,4),0)
            if not value:break
            if not value & 0x80000000:
                name = pe.string(pe.base + value + 2)
                decorated = '__imp__' + name
                pins[decorated] = pe.base + iat + index*4
                reasons[decorated] = 'actual PE import slot: ' + dll + '!' + name
            index += 1
        descriptor += 20
    queue = ['_mainCRTStartup', '_atexit']
    verified, unresolved = {}, []
    while queue:
        name = queue.pop(0)
        if name in verified or name == '_main' or name.startswith('__imp_') or name == '__except_list':continue
        options = definitions.get(name, [])
        matched = []
        for member, raw, obj, symbol in options:
            section = obj.sections[symbol['section']-1]
            if not section['flags'] & 0x20: continue
            start = pins[name] - symbol['value']
            try: actual = pe.read(start, len(section['data']))
            except ValueError:continue
            masked = set()
            for offset, _, kind in section['relocs']:
                if kind not in (6,20):break
                masked.update(range(offset,offset+4))
            else:
                if all(a==b for i,(a,b) in enumerate(zip(actual,section['data'])) if i not in masked):
                    matched.append((member,raw,obj,symbol,section,start))
        if len(matched)!=1:
            unresolved.append(dict(name=name,address=pins[name],matching_objects=len(matched)))
            continue
        member, raw, obj, symbol, section, start = matched[0]
        section_bases = {section['index']:start}
        todo = [section]
        checked_sections = []
        relocations = []
        for selected in todo:
            address = section_bases[selected['index']]
            actual = pe.read(address,len(selected['data']))
            relocation_bytes = {i for off,_,_ in selected['relocs'] for i in range(off,off+4)}
            assert all(a==b for i,(a,b) in enumerate(zip(actual,selected['data'])) if i not in relocation_bytes), (name,member,selected['name'],hex(address),actual.hex(),selected['data'].hex())
            checked_sections.append(dict(name=selected['name'],start=address,end=address+len(selected['data'])))
            for offset, referred, kind in selected['relocs']:
                assert kind in (6,7,20), 'unsupported relocation kind'
                site = address + offset
                value = u32(actual,offset)
                addend = u32(selected['data'],offset)
                resolved = ((value + site + 4) if kind==20 else (value+pe.base if kind==7 else value)) - addend
                resolved &= 0xffffffff
                if referred['section'] > 0:
                    if member.lower() == 'msvcrt.dll' and referred['name'].startswith('__imp_'):
                        # Import hints can differ between archived import metadata
                        # and the executable. The executable's actual DLL/name
                        # descriptor, not that advisory hint, binds this thunk.
                        assert pins[referred['name']] == resolved, 'import thunk binds wrong actual PE slot'
                        relocations.append(dict(site=site,name=referred['name'],kind=kind,target=resolved,
                                                proof='actual executable DLL/name import descriptor; no import-hint identity claim'))
                        continue
                    if referred['type']==32:
                        pins[referred['name']]=resolved
                        reasons[referred['name']]='exact same-object code relocation at %08x'%site
                        if referred['name'] not in verified:queue.append(referred['name'])
                    newbase = resolved - referred['value']
                    if referred['section'] in section_bases:
                        assert section_bases[referred['section']] == newbase
                    else:
                        section_bases[referred['section']] = newbase
                        todo.append(obj.sections[referred['section']-1])
                else:
                    if referred['name'] in pins:
                        assert pins[referred['name']] == resolved, 'relocation identity differs: ' + referred['name']
                    else:
                        pins[referred['name']] = resolved
                        reasons[referred['name']] = '%s:%s relocation at %08x' % (member,name,site)
                    if referred['type']==32 and referred['name'] not in verified:
                        queue.append(referred['name'])
                relocations.append(dict(site=site,name=referred['name'],kind=kind,target=resolved))
        verified[name] = dict(name=name,start=pins[name],member=member,
                              object_sha256=hashlib.sha256(raw).hexdigest(),
                              verified_sections=checked_sections,relocations=relocations)
    return dict(kind='exact anchored runtime identity analysis; no acceptance',
                target_sha256=hashlib.sha256(target.read_bytes()).hexdigest(),
                library_sha256=hashlib.sha256(library.read_bytes()).hexdigest(),
                functions=list(verified.values()), unresolved=unresolved,
                pins={name:dict(address=value,reason=reasons[name]) for name,value in pins.items()})


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rebuilt',action='store_true')
    args=parser.parse_args()
    if args.rebuilt:
        symbols={}
        for line in (BINARY/'build/link.map').read_text().splitlines():
            match=re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',line)
            if match:symbols[match[1]]=int(match[2],16)
        result=analyze(BINARY/'build/link.exe',symbols['_main'], symbols['_atexit'])
        assert not result['unresolved'], 'rebuilt CRT closure is not verified'
        records=[]
        for record in result['functions']:
            section=next(s for s in record['verified_sections'] if s['name']=='.text')
            records.append(dict(name=record['name'],start=record['start'],end=section['end'],evidence='build/runtime-identity.json'))
        (BINARY/'build/runtime.json').write_text(json.dumps(records,indent=2)+'\n')
        graph=json.loads((BINARY/'graph.json').read_text())
        graph['functions']['_mainCRTStartup']['rebuilt']['roots']=[symbols['_mainCRTStartup']+0x10e,symbols['_mainCRTStartup']+0x123]
        configuration=json.loads((BINARY/'functions.json').read_text())
        if any(r['name']=='_main' and r['status']=='implemented' for r in configuration['driver']):
            rebuilt=PE(BINARY/'build/link.exe')
            assert rebuilt.read(symbols['_main']+3,3)==b'\x6a\xff\x68'
            scope=u32(rebuilt.read(symbols['_main']+6,4),0)
            previous,filter_entry,handler_entry=struct.unpack('<III',rebuilt.read(scope,12))
            assert previous==0xffffffff
            graph['functions']['_main']['rebuilt']['roots']=[filter_entry,handler_entry]
        (BINARY/'build/graph.json').write_text(json.dumps(graph,indent=2)+'\n')
        destination=BINARY/'build/runtime-identity.json'
    else:
        result=analyze()
        destination=BINARY/'cases/runtime-entry-identity.json'
    destination.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(verified=len(result['functions']),unresolved=result['unresolved']),indent=2))
