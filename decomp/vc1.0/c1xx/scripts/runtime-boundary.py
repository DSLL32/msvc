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

def identities(canonical=False, names=NAMES):
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
    for name in names:
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
                local=target['storage']==3 and target['section']>0
                key=old['object']+':'+target['name'] if local else target['name']
                assert old_globals.setdefault(key,address)==address,'split CRT global'
                if local:
                    data_section=obj.sections[target['section']-1]
                    assert not data_section['relocs'],'local CRT data needs relocation-aware analysis'
                    # Static section symbols are not map publics. Anchor the
                    # canonical section by its exported symbols and verify the
                    # encountered object's initialized bytes in the original.
                    assert original.read(address-target['value'],len(data_section['data']))==data_section['data'],'original CRT local data differs'
                    if canonical:
                        anchors=[s for s in obj.symbols.values() if s['section']==target['section'] and s['storage']==2 and s['name'] in mapping]
                        assert anchors,'CRT local data has no public section anchor'
                        bases={mapping[s['name']]-s['value'] for s in anchors}
                        assert len(bases)==1,'canonical CRT section aliases split'
                        base=bases.pop()
                        assert b==((base+target['value']+addend)&0xffffffff),'canonical CRT local data target differs'
                        assert rebuilt.read(base,len(data_section['data']))==data_section['data'],'canonical CRT local data differs'
                elif canonical:assert b==((mapping[target['name']]+addend)&0xffffffff),'canonical CRT data target differs'
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
    if evidence.get('kind')=='fflush':
        return check_stream(path)
    if evidence.get('kind')=='exit':
        return check_exit(path)
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


def check_stream(path):
    """Require the explicit successful buffered fflush transition and proof."""
    evidence=json.loads(path.read_text())
    config=json.loads((BINARY/'functions.json').read_text())
    assert evidence['target_sha256']==config['target']['sha256']
    assert evidence['runtime_order']==['_fflush']
    left,right=evidence['original'],evidence['native']
    assert left['return_va']==0x74324 and right['caller']=='close_intermediate'
    assert all(left[k]==right[k] for k in ('before','after','result')),'fflush boundary differs'
    before,after=left['before'],left['after']
    assert left['result']==0 and before['used']>0 and before['flags']&0x80 and (before['flags']&3)==2
    assert after==dict(before,flags=before['flags']&~2,count=0,used=0),'fflush stream transition differs'
    assert evidence['identity']==identities(names=('_fflush',)),'fflush library identity changed'
    for item in evidence['evidence_files']:
        assert sha(BINARY/item['path'])==item['sha256'],'fflush observation evidence changed'
    raw=(BINARY/evidence['original_observation']).read_text()
    assert re.findall(r'^runtime (\S+)$',raw,re.M)==['_fflush']
    assert int(re.search(r'^return_va ([0-9a-f]+)$',raw,re.M)[1],16)==left['return_va']
    assert int(re.search(r'^result (-?\d+)$',raw,re.M)[1])==left['result']
    for stage in ('before','after'):
        values=re.search(r'^'+stage+r' descriptor (\d+) flags (\d+) count (-?\d+) used (-?\d+) size (\d+)$',raw,re.M)
        assert dict(zip(('descriptor','flags','count','used','size'),map(int,values.groups())))==left[stage]
    assert json.loads((BINARY/evidence['native_observation']).read_text())==right
    if 'current_native_observation' in evidence:
        assert json.loads((BINARY/evidence['current_native_observation']).read_text())==right
    return evidence


def check_exit(path):
    """Verify explicit exit entry state plus every ordered original CRT entry."""
    evidence=json.loads(path.read_text())
    config=json.loads((BINARY/'functions.json').read_text())
    assert evidence['target_sha256']==config['target']['sha256']
    names=evidence['runtime_order']
    assert names==['_exit','_doexit','__endstdio','__flushall','_flsall','import:ExitProcess'],'unobserved termination sequence'
    assert evidence['identity']==identities(names=names),'termination library identity changed'
    for item in evidence['evidence_files']:
        assert sha(BINARY/item['path'])==item['sha256'],'termination evidence changed'
    raw=(BINARY/evidence['original_observation']).read_text()
    native=json.loads((BINARY/evidence['native_observation']).read_text())
    assert native['function']=='recon_exit' and native['exit_code']==0 and native['signal']==5
    assert int(re.search(r'^argument 0 ([0-9a-f]+)$',raw,re.M)[1],16)==native['exit_code']
    assert int(re.search(r'^caller ([0-9a-f]+)$',raw,re.M)[1],16)==0x59a7b
    assert native['states']==[int(x,16) for x in re.findall(r'^state \d+ ([0-9a-f]+)$',raw,re.M)]
    assert native['buffers']==[x[1] for x in re.findall(r'^buffer \d+ (\d+) ([0-9a-f]*)$',raw,re.M)]==['']*4
    previous='_exit'
    for name,entry in zip(names,evidence['original_captures']):
        receipt=json.loads((BINARY/entry).read_text())
        assert receipt['candidate']==name==previous
        previous=receipt['terminal']
    assert previous=='exit:0'
    return evidence
