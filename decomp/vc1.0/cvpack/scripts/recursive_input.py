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
