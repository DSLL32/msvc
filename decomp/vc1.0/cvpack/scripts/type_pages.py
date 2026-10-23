"""First global type-pointer page: exact malloc lifetime, zero defined slots."""
import hashlib,json,re
from state import BINARY
from objects import namespace


def required(config,case):
    return case=='hello-debug-dll'and any(r['name']=='save_type'and r['status']!='stub'for r in config['functions'])


def validate(config,pe):
    ref=config.get('type_page_observations')
    assert ref or not required(config,'hello-debug-dll'),'missing type page allocation proof'
    if not ref:return
    path=BINARY/ref['evidence'];assert hashlib.sha256(path.read_bytes()).hexdigest()==ref['evidence_sha256'],'stale type page proof'
    p=json.loads(path.read_text());assert p['schema']==1 and p['size']==2048 and p['root_count']==120 and p['defined_ranges']==[]
    assert p['frontier']=='append_buffer'and p['original_frontier']==0x22f40 and p['allocation_return']==0x199d7
    for source,digest in p['sources'].items():assert hashlib.sha256((BINARY/source).read_bytes()).hexdigest()==digest,'type page source changed'
    for filename in('data.json','walks.json'):assert hashlib.sha256((BINARY/filename).read_bytes()).hexdigest()==p[filename+'_sha256'],'type page ownership registry changed'
    for r in p['instructions']:assert hashlib.sha256(pe.read(r['start'],r['end']-r['start'])).hexdigest()==r['sha256'],'type page original creation/publication differs'
    data={r['name']:r for r in json.loads((BINARY/'data.json').read_text())}
    assert data['type_pages']['original']==0x37198 and data['type_pages']['size']==480 and data['type_pages']['kind']=='pointer-array','type page static root owner differs'
    assert data['next_type_index']['original']==0x37398 and data['next_type_index']['size']==2
    assert data['type_page_capacity']['original']==0x33924 and data['type_page_capacity']['size']==2
    assert pe.read(0x37198,480)==b'\0'*480,'type page initial roots are not null'
    lines=(BINARY/'src/types.c').read_text().splitlines();line=p['native_allocation_line']
    assert lines[line-2].strip()=='page = checked_alloc(0x800);'and lines[line-1].strip()=='page_index = ((int)next_type_index-0x1000)/512;','type page native allocation observation site differs'
    return p


def parse(events):
    entries=[];outputs=[]
    for line in events.splitlines():
        if line.startswith('type-page-allocation '):
            fields=line.split()[1:]
            assert fields[::2]==['base','size','index','capacity','owner','esp','before','roots_before'],'invalid type page creation event'
            entries.append({k:(v if k in('before','roots_before')else int(v,16 if k in('base','owner','esp')else 10))for k,v in zip(fields[::2],fields[1::2])})
        elif line.startswith('type-page-output '):
            fields=line.split()[1:];assert fields[::2]==['base','size','frontier','bytes'],'invalid type page output event'
            outputs.append({k:(v if k=='bytes'else int(v,10 if k=='size'else 16))for k,v in zip(fields[::2],fields[1::2])})
    return entries,outputs


def compare(config,pe,events,native,old_walks,new_walks,state,case,buffer_proof=None):
    p=validate(config,pe);entries,outputs=parse(events)
    if not(entries or outputs or native):
        assert not required(config,case),'missing mandatory type page lifetime observations'
        return
    assert p and case=='hello-debug-dll'and len(entries)==len(outputs)==len(native)==1,'missing/extra type page allocation lifetime'
    assert events.splitlines().count('type-page-rearm 000199d7 000199da')==1,'missing type page allocation rearm'
    a,b,post=entries[0],native[0],outputs[0];assert not b.get('error'),'native type page observer failed'
    assert a['size']==b['size']==post['size']==2048 and a['index']==b['index']==0x1000 and a['capacity']==b['capacity']==512
    assert post['base']==a['base']and ((post['frontier']==0x22f40 and b['frontier']=='append_buffer')or(post['frontier']in(0x262d0,0x1ad40,0x1af10,0x1ae40,0x19770,0x256f0,0x1e700,0x27b10,0x1ac80,0x1c640,0x1eac0,0x19ca0)and b['frontier']in('finish_recursive_types','find_named_type','find_global_type','find_local_type','record_derived_types','convert_global_data32','get_type_record','add_recursive_offset','intern_type','hash_recursive_type','commit_recursive_type','publish_recursive_type')and buffer_proof)), 'unproved type page frontier'
    if post['frontier']==0x22f40:
        words={int(m[1]):int(m[2],16)for m in re.finditer(r'^stack (\d+) ([0-9a-f]+)$',events,re.M)}
        fields=re.search(r'^registers (.*)$',events,re.M)[1].split();registers={k:int(v,16)for k,v in zip(fields[::2],fields[1::2])}
        assert words[0]==0x19a65 and words[1]==8 and a['esp']==registers['esp']+4,'type page allocation belongs to another caller frame'
        assert len(old_walks['local_type_records'])==len(new_walks['local_type_records'])==1
        old_table,new_table=old_walks['local_type_records'][0],new_walks['local_type_records'][0];offset=19*28
        assert a['owner']==registers['ebx']==old_table['address']+offset and b['owner']==new_table['address']+offset,'wrong type page parent record owner'
        assert registers['esi']==a['base']and registers['edi']==0 and registers['ebp']&0xffff==8,'wrong type page active slot/size'
        data_pointer=int.from_bytes(bytes.fromhex(old_table['bytes'])[offset:offset+4],'little')
        assert registers['edx']==data_pointer and registers['ecx']==0x3301c,'wrong pending type page record copy arguments'
        frozen=(BINARY/'cases/hello-debug-dll/input/msvc.pdb').read_bytes()
        assert hashlib.sha256(frozen).hexdigest()=='6421bc52a3c6b42958d229e92b0b4612dfb5335be933fe4ec995cd66fb30cd28','changed type page record input'
        for walks,table in((old_walks,old_table),(new_walks,new_table)):
            pointer=int.from_bytes(bytes.fromhex(table['bytes'])[offset:offset+4],'little')
            block=walks['current_type_server'][0]['objects']['type-storage-1'];delta=pointer-block['address']
            assert 12<=delta and delta+8<=12+4140,'pending type copy extends outside defined PDB payload'
            payload=bytes.fromhex(block['bytes']);record=payload[delta:delta+8]
            assert record==frozen[delta+52:delta+60]and record[:6]==bytes.fromhex('060001020100'),'pending type copy differs from its actual frozen record'
    else:
        assert buffer_proof['original']['entries'][0]['owner']==a['owner']and buffer_proof['native']['entries'][0]['owner']==b['owner'],'type page allocation parent differs'
        assert a['esp']==buffer_proof['original']['entries'][0]['esp']+4,'type page creation frame differs'

    memory={}
    for m in re.finditer(r'^memory ([0-9a-f]+) ([0-9a-f]+)$',events,re.M):memory.update((int(m[1],16)+i,v)for i,v in enumerate(bytes.fromhex(m[2])))
    old_roots=bytes(memory[0x37198+i]for i in range(480))
    new_roots=bytes.fromhex(b['roots_after']);assert len(new_roots)==480
    assert state['globals']['type_pages']==[dict(null=False,address=b['base'])]+[dict(null=True,address=0)for _ in range(119)],'native type page root inventory differs'
    for row,walks,after,roots in((a,old_walks,post['bytes'],old_roots),(b,new_walks,b['postbytes'],new_roots)):
        assert row['base']and row['base']%4==0
        assert bytes.fromhex(row['roots_before'])==b'\0'*480,'type page existed before current allocation'
        assert roots==row['base'].to_bytes(4,'little')+b'\0'*476,'type page root publication differs'
        before,after=bytes.fromhex(row['before']),bytes.fromhex(after)
        assert len(before)==len(after)==2048
        if post['frontier']==0x22f40:assert after==before,'type page has unproved writes before append returns'
        else:
            side=buffer_proof['original']if row is a else buffer_proof['native']
            slots=b''.join(r['result'].to_bytes(4,'little')for r in side['returns'])
            assert len(slots)in(12,24,32,104,140,280,284,288)and after[:len(slots)]==slots and after[len(slots):]==before[len(slots):],'type page pointer writes/untouched tail differ'
            for offset in range(0,len(slots),4):
                value=int.from_bytes(after[offset:offset+4],'little')
                assert value==side['returns'][offset//4]['result'],'type page slot lacks observed copy result'
        for address,size,name in namespace(walks):assert not(row['base']<address+size and address<row['base']+2048),'type page allocation overlaps another owner'
    return dict(original=a,native=b,defined_ranges=([[0,len(buffer_proof['original']['returns'])*4]]if buffer_proof else []),logical_owner='save_type/module3/type_page0/malloc199d2',original_namespace=dict(address=a['base'],bytes=post['bytes']),native_namespace=dict(address=b['base'],bytes=b['postbytes']))
