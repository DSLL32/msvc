"""Offline verification of the explicitly observed source-file close boundary."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct
from binary import PE, COFF, archive, match_section
from win32_indexes import imports, indexes, publics

BINARY=Path(__file__).resolve().parents[1]
NAMES=('__close','import:CloseHandle','__free_osfhnd')

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def identities(canonical=False):
    config=json.loads((BINARY/'functions.json').read_text())
    original=PE(BINARY.parents[2]/'toolchains/1.0-nt/bin/C1XX3232.EXE')
    assert hashlib.sha256(original.data).hexdigest()==config['target']['sha256'],'original target changed'
    old_records=json.loads((BINARY/'runtime.json').read_text())+imports(original)+config['driver']
    if canonical:
        original, rebuilt, config, old_records, new_records=indexes()
    library=BINARY.parents[2]/'toolchains/1.0-nt/lib/LIBC.LIB'
    objects=dict(archive(library))
    mapping=publics() if canonical else {}
    old_globals={}
    result=[]
    for name in NAMES:
        old=next(r for r in old_records if r['name']==name)
        new=next(r for r in new_records if r['name']==name) if canonical else None
        if name.startswith('import:'):
            assert old['end']-old['start']==6
            if canonical:assert new['end']-new['start']==6
            result.append(dict(name=name,record=old,identity='same named import thunk in original and canonical reconstruction'))
            continue
        assert old['library']=='LIBC.LIB'
        if canonical:assert old['object']==new['object'] and old['library']==new['library']
        data=objects[old['object']]
        obj=COFF(data)
        symbol=next(s for s in obj.symbols.values() if s['name']==name)
        section=obj.sections[symbol['section']-1]
        size=old['end']-old['start']
        if canonical:assert size==new['end']-new['start']
        begin=symbol['value']
        fragment=dict(section,data=section['data'][begin:begin+size],relocs=[])
        for offset,target,kind in section['relocs']:
            if begin<=offset<begin+size:fragment['relocs'].append((offset-begin,target,kind))
        assert match_section(original.read(old['start'],size),fragment)==[0],name+' original member bytes differ'
        if canonical:assert match_section(rebuilt.read(new['start'],size),fragment)==[0],name+' canonical member bytes differ'
        old_edges=set()
        relocations=[]
        for offset,target,kind in fragment['relocs']:
            addend=struct.unpack_from('<I',fragment['data'],offset)[0]
            a=struct.unpack('<I',original.read(old['start']+offset,4))[0]
            b=struct.unpack('<I',rebuilt.read(new['start']+offset,4))[0] if canonical else None
            if kind==6:
                address=(a-addend)&0xffffffff
                assert old_globals.setdefault(target['name'],address)==address,'split CRT global'
                if canonical:assert b==((mapping[target['name']]+addend)&0xffffffff),'canonical CRT data target differs'
            elif kind==20:
                logical=target['name']
                if logical.startswith('_') and '@' in logical:logical='import:'+logical[1:].rsplit('@',1)[0]
                address=(old['start']+offset+4+a-addend)&0xffffffff
                assert any(r['name']==logical and r['start']==address for r in old_records),'original CRT call relocation differs'
                if canonical:
                    address=(new['start']+offset+4+b-addend)&0xffffffff
                    assert any(r['name']==logical and r['start']==address for r in new_records),'canonical CRT call relocation differs'
                old_edges.add(logical)
            else:raise AssertionError('unsupported CRT relocation kind')
            relocations.append(dict(offset=offset,symbol=target['name'],kind=kind))
        result.append(dict(name=name,record=old,library_sha256=sha(library),object_sha256=hashlib.sha256(data).hexdigest(),
            member_bytes=size,member_sha256=hashlib.sha256(fragment['data']).hexdigest(),
            original_body_sha256=hashlib.sha256(original.read(old['start'],size)).hexdigest(),
            relocation_identity='exact nonrelocation bytes, named direct/indirect call edges, shared DIR32 globals and canonical public data targets',
            relocations=relocations,call_targets=sorted(old_edges)))
    return dict(records=result,original_crt_globals=old_globals)


def check(path):
    evidence=json.loads(path.read_text())
    config=json.loads((BINARY/'functions.json').read_text())
    assert evidence['target_sha256']==config['target']['sha256']
    assert evidence['runtime_order']==list(NAMES),'unobserved runtime boundary'
    left,right=evidence['original'],evidence['native']
    assert left['runtime_order']==list(NAMES)
    assert left['return_va']==0x1e9bc and right['caller']=='close_source_file'
    assert all(left[k]==right[k] for k in ('descriptor','open_flag','result','closed_flag')),'close boundary differs'
    assert left['open_flag']&1 and left['result']==0 and left['closed_flag']==0,'close did not close the observed source descriptor'
    assert evidence['identity']==identities(),'CRT library/import identity changed'
    for item in evidence['evidence_files']:
        assert sha(BINARY/item['path'])==item['sha256'],'runtime observation evidence changed'
    raw=(BINARY/evidence['original_observation']).read_text()
    assert re.findall(r'^runtime (\S+) argument ',raw,re.M)==list(NAMES)
    assert int(re.search(r'^return_va ([0-9a-f]+)$',raw,re.M)[1],16)==left['return_va']
    assert int(re.search(r'^runtime __free_osfhnd argument ([0-9a-f]+)$',raw,re.M)[1],16)==left['descriptor']
    native=json.loads((BINARY/evidence['native_observation']).read_text())
    for key in ('descriptor','open_flag','result','closed_flag'):
        assert int(re.search(r'^'+key+r' (\d+)$',raw,re.M)[1])==left[key]
        assert native[key]==right[key]
    assert native['caller']==right['caller']
    return evidence
