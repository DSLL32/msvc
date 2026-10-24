"""Concrete comparator input owners; no full type/module heap replay."""
import re

def parse(events,label,names,hexes=()):
    rows=[line.split()for line in events.splitlines()if line.startswith(label+' ')]
    assert len(rows)==1,'missing actual '+label
    words=rows[0];assert words[1::2]==names
    return {name:(words[2+2*i]if name=='bytes'else int(words[2+2*i],16 if name in hexes else 10))for i,name in enumerate(names)}

def compare(events,words,parameters):
    local=parse(events,'recursive-local',['table','capacity','count','first','type','entry','bytes'],('table','entry'))
    data=parse(events,'recursive-local-data',['module','input','requested','defined','directory','offset','record','bytes'],('input','record'))
    published=parse(events,'recursive-published',['index','next','page','record','block','data','used','capacity','bytes'],('page','record','block','data'))
    hashed=parse(events,'recursive-hash',['bucket','head','count','position','cursor','available','arena','node','bytes'],('head','cursor','arena','node'))
    assert words[0]==0x1ad0d and local['entry']==words['ecx']and local['type']==(words['edx']&0xffff)and published['record']==words[1]and published['index']==(words[2]&0xffff),'comparator actual ABI differs'
    new_local=parameters['entry']['typed']['local'];new_data=parameters['entry']['typed']['input'];new_pub=parameters['record']['typed']['published'];new_hash=parameters['record']['typed']['hash']
    assert local['capacity']==new_local['capacity']==12000 and local['count']==new_local['count']and local['first']==new_local['first']and local['type']==new_local['type'],'local table input differs'
    for r in(local,new_local):assert 0<=r['type']-r['first']<r['count']<=r['capacity']and r['entry']==r['table']+(r['type']-r['first'])*28
    old,new=bytes.fromhex(local['bytes']),bytes.fromhex(new_local['bytes']);assert len(old)==len(new)==28 and old[4:]==new[4:],'initialized TypeRecord fields differ'
    assert int.from_bytes(old[:4],'little')==data['record']and int.from_bytes(new[:4],'little')==new_data['record']
    assert all(data[k]==new_data[k]for k in('module','requested','defined','directory','offset','bytes')),'actual local type payload differs'
    for r in(data,new_data):
        raw=bytes.fromhex(r['bytes']);assert 4<=len(raw)<=4096 and len(raw)==int.from_bytes(raw[:2],'little')+2 and r['input']+4<=r['record']and r['record']+len(raw)<=r['input']+r['defined']<=r['input']+r['requested'],'local initialized input span differs'
    assert all(published[k]==new_pub[k]for k in('index','next','used','capacity','bytes')),'actual published type input differs'
    for r in(published,new_pub):
        raw=bytes.fromhex(r['bytes']);assert 0x1000<=r['index']<r['next']and len(raw)==int.from_bytes(raw[:2],'little')+2 and r['data']<=r['record']and r['record']+len(raw)<=r['data']+r['used']<=r['data']+r['capacity']
    assert all(hashed[k]==new_hash[k]for k in('bucket','count','position','available')),'actual candidate hash path differs'
    for r,p in((hashed,published),(new_hash,new_pub)):
        raw=bytes.fromhex(r['bytes']);assert len(raw)==12 and int.from_bytes(raw[:4],'little')==p['record']and int.from_bytes(raw[4:6],'little')==p['index']
        assert r['arena']==r['cursor']-(1024-r['available'])*12 and r['arena']<=r['node']<r['cursor']and (r['node']-r['arena'])%12==0 and 0<=r['position']<r['count']
    assert local['entry']==words['ecx']and new_local['entry']==parameters['entry']['address']and published['record']==words[1]and new_pub['record']==parameters['record']['address']
    return dict(original=dict(local=local,input=data,published=published,hash=hashed),native=dict(local=new_local,input=new_data,published=new_pub,hash=new_hash),scope='Actual local-table entry and defined module-type bytes; published global type/page/buffer and recursive hash-node identity. Raw pointer values retained; 2 reserved node bytes and unconsumed next link receive no equality credit. Expanded recursive arrays pending.')


def compare_numeric(events,words,actual):
    old=parse(events,'numeric-type-input',['module','input','requested','defined','directory','source','argument','size','bytes'],('input','argument'))
    new=actual['typed'];assert words[0]==0x27766 and old['argument']==words[1]and new['argument']==actual['address'],'numeric input caller/argument differs'
    assert all(old[k]==new[k]for k in('module','requested','defined','directory','source','size','bytes')),'actual numeric tag/span differs'
    for r in(old,new):
        assert r['input']+4<=r['argument']and r['argument']+2<=r['input']+r['defined']<=r['input']+r['requested']and r['size']==2 and len(bytes.fromhex(r['bytes']))==2
    assert old['argument']-old['input']==new['argument']-new['input'],'numeric input module-relative identity differs'
    return dict(original=old,native=new,scope='Actual two-byte numeric selector inside the fully read current module-type subsection. Only the selector consumed on this reached LONG path is compared; no undefined capacity tail or speculative numeric payload reads.')


def compare_precompiled(events,words,actual):
    old=parse(events,'precompiled-type-input',['module','input','capacity','defined','directory','source','argument','size','bytes'],('input','argument'));new=actual['typed']
    offset={0x1a0e3:2,0x1a2f1:12}.get(words[0]);assert offset is not None and old['argument']==words[1]and new['argument']==actual['address'],'precompiled input actual caller/ABI differs'
    assert all(old[k]==new[k]for k in('module','capacity','defined','directory','source','size','bytes')),'precompiled input initialized payload differs'
    for r in(old,new):
        raw=bytes.fromhex(r['bytes']);assert r['argument']==r['input']+offset and len(raw)==r['size']==int.from_bytes(raw[:2],'little')+2 and 14<=r['size']<=min(1024,r['defined'])<=r['capacity']and raw[2:4]==b'\x13\0','precompiled record outside initialized first block'
    return dict(original=old,native=new,scope='Actual first LF_PRECOMP record in the current module first-block payload. The separately consumed DWORD signature is excluded from the initialized payload size; no unread block capacity or later record/heap equality claim.')


def compare_symbol_prefix(events,words,actual):
    old=parse(events,'packed-symbol-input',['directory','subtype','module','owner','input','requested','defined','bytes'],('owner','input'));new=actual['typed']
    assert words[0]==0x20d27 and old['input']==words[1]and new['input']==actual['address'],'symbol prefix actual caller/ABI differs'
    assert all(old[k]==new[k]for k in('directory','subtype','module','requested','defined','bytes')),'actual published symbol prefix differs'
    allocations=[line.split()for line in events.splitlines()if line.startswith('packed-symbol-allocation ')]
    matches=[r for r in allocations if int(r[6],16)==old['input']and int(r[4],16)==old['owner']];assert len(matches)==1,'missing actual symbol allocation lifetime'
    allocation=matches[0];assert int(allocation[8])==old['requested']==int(allocation[10])+int(allocation[12])-int(allocation[14]),'original symbol allocation request differs'
    for r in(old,new):assert r['subtype']==0x125 and 0<r['defined']<=r['requested']<=524288 and len(bytes.fromhex(r['bytes']))==r['defined']==words[2]
    return dict(original=old,native=new,original_allocation=allocation,scope='Actual first buffered-write source: live selected directory125 entry and module.symbols publication, real malloc request/return, exact defined packed symbol prefix. No uninitialized allocation tail or wider module heap equality claimed.')
